#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

int main() {
    int num_voters;
    printf ("Enter the number of voters: ");
    if (scanf("%d", &num_voters) != 1 || num_voters < 3) { // Check if input isn't a valid integer or greater than or equal to 3
        printf("Number of voters must be greater than or equal to 3.\n");
        return 1;
    }

    int num_rounds;
    printf("Enter the number of rounds: ");
    if (scanf("%d", &num_rounds) != 1 || num_rounds < 1) { // Check if input isn't a valid integer or greater than or equal to 1
        printf("Number of rounds must be at least 1.\n");
        return 1;
    }
    printf("\n");

    // Judge creates N pipes    
    int pipes[num_voters][2];
    for (int i = 0; i < num_voters; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("Pipe failed");
            exit(1);
        }
    }

    // Judge creates N child processes
    int voter_id = -1;
    for (int i = 0; i < num_voters; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("Fork failed");
            exit(1);
        } else if (pid == 0) { // Child process: set incrementing voter_id
            voter_id = i;
            break;
        }
    }

    // Close all unused pipe ends in both judge and voter processes
    if (voter_id == -1) {
        // Judge process closes write ends of all pipes
        for (int i = 0; i < num_voters; i++) {
            close(pipes[i][1]);
        }
    }
    else {
        // Voter process closes read end of its own pipe and both ends of all other pipes
        for (int i = 0; i < num_voters; i++) {
            if (i == voter_id) {
                close(pipes[i][0]);
            } else {
                close(pipes[i][0]); 
                close(pipes[i][1]); 
            }
        }
        srand(getpid() + voter_id); // Seed random number generator using both process ID and voter ID for differnt values between voters and program runs
    }

    // Voting rounds
    for (int round = 1; round <= num_rounds; round++) {
        if (voter_id == -1) { // Judge process
            printf("------ Round %d ------\n", round);
            int yeses = 0;
            // Read votes from all voters
            for (int i = 0; i < num_voters; i++) {
                char vote_buffer[10];
                ssize_t bytes_read = read(pipes[i][0], vote_buffer, sizeof(vote_buffer) - 1);
                if (bytes_read > 0) {
                    vote_buffer[bytes_read] = '\0';
                    if (strcmp(vote_buffer, "YES") == 0) {
                        yeses++;
                    }
                }
            }
        
        // Determine and print the voting result
        if (yeses > num_voters / 2) {
                printf("--- Round %d: The voting result is a YES.\n\n", round);
            } else {
                printf("--- Round %d: The voting result is a NO.\n\n", round);
            }
        // Wait for judge to finish printing
        usleep(300000);
        }
        else { // Voter process
            int choice = rand() % 2; // Randomly choose 0 (NO) or 1 (YES)
            char* vote = (choice == 1) ? "YES" : "NO";

            // Print the vote explicitly 
            printf("Round %d: Voter %d votes: %s\n", round, voter_id, vote);

            // Send the vote to the judge through the pipe
            write(pipes[voter_id][1], vote, strlen(vote) + 1);
            
            // Wait before next round to not flood the pipe
            usleep(150000);
        }
    }

    if (voter_id == -1) {
        // Judge closes its read ends
        for (int i = 0; i < num_voters; i++) {
            close(pipes[i][0]);
        }
        // Wait for child processes to finish
        for (int i = 0; i < num_voters; i++) {
            wait(NULL);
        }
    }
    else {
        // Voter closes its write end and exits
        close(pipes[voter_id][1]);
        exit(0);
    }

    return 0;
}
