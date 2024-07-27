#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>


// Structure to hold termination flag
typedef struct {
    int termination_flag;  // Flag to indicate whether to close the hotel or not
    int busy_flag;
} TerminationData;

int main() {
    // Generate key for the shared memory segment
    
    key_t key = ftok("admin.c", 'a');
    if (key == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }

    // Create shared memory segment for termination flag
    int termination_shmid = shmget(key, sizeof(TerminationData), IPC_CREAT | 0666);
    if (termination_shmid == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }
    
    // Attach shared memory segment
    TerminationData *termination_data = (TerminationData *)shmat(termination_shmid, NULL, 0);
    if (termination_data == (void *)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    char choice;
    do {
        // Prompt user whether to close the hotel or not
        printf("\nDo you want to close the hotel? Enter Y for Yes and N for No: ");
        scanf(" %c", &choice);
        

        if (choice == 'Y' || choice == 'y') {
            // Set termination flag
            termination_data->termination_flag=1;
            printf("\nChecking if all tables are free... \n");
            sleep(3);
            if(termination_data->busy_flag==1) {
                printf("Tables occupied, can't close.\n");
                termination_data->termination_flag=0;
                continue;
            }
            else {
                printf("\n------------Hotel Closed------------\n\n");
                break;
            }
        } else if (choice == 'N' || choice == 'n') {
            // Keep running as usual
            continue;
        } else {
            printf("Invalid input. Please enter Y or N.\n");
        }
    } while (1);

    // Detach shared memory segment
    if (shmdt(termination_data) == -1) {
        perror("shmdt");
        exit(EXIT_FAILURE);
    }

    // Delete shared memory segment
    //shmctl(shm_id, IPC_RMID, NULL); 

    return 0;
}
