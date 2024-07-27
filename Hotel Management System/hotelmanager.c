#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>

#define EARNINGS_FILE "earnings.txt"

// Structure to hold earnings information and flags
typedef struct {
    int total_tables;       // Total number of tables in the restaurant
    int earnings_flag;      // Table number for which new earnings have been received (0 for no new earnings)
    int earnings[10];       // Earnings from the latest set of customers
    int total_earnings;     // Total earnings of the hotel
    int total_wages;        // Total wages of waiters (40% of total earnings)
    int total_profit;       // Total profit made by the hotel (total earnings - total wages)
    int free[10];
} HotelManagerData;

typedef struct {
    int termination_flag;  // Flag to indicate whether to close the hotel or not
    int busy_flag;
} TerminationData;

// Function to create or open the earnings file and write earnings into it
void write_earnings_to_file(int earnings, int table_number) {
    FILE *earnings_fp = fopen(EARNINGS_FILE, "a"); // Open file in append mode
    if (earnings_fp == NULL) {
        perror("Error opening earnings file");
        exit(EXIT_FAILURE);
    }

    fprintf(earnings_fp, "Earnings from Table %d: %d INR\n", table_number, earnings);

    fclose(earnings_fp);
}

int main() {
    int total_tables;
    //---------------------------------------------------------------------------------
    // Generate key for the termination shared memory segment (ADMIN'S BLOCK CONNECTION)
    key_t termination_key = ftok("admin.c", 'a');
    if (termination_key == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }

    // Open the termination shared memory segment
    int termination_shmid = shmget(termination_key, sizeof(TerminationData), 0666);
    if (termination_shmid == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    // Attach the termination shared memory segment
    TerminationData *termination_data = (TerminationData *)shmat(termination_shmid, NULL, 0);
    if (termination_data == (void *)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }

    //---------------------------------------------------------------------------------
    // Generate key for the shared memory segment (HOTEL MANAGER's BLOCK)
    key_t key = ftok("hotelmanager.c", 'A');
    if (key == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }

    // Create shared memory segment
    int shmid = shmget(key, sizeof(HotelManagerData), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    // Attach shared memory segment
    HotelManagerData *hotel_manager_data = (HotelManagerData *)shmat(shmid, NULL, 0);
    if (hotel_manager_data == (void *)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }
    //---------------------------------------------------------------------------------
    // Prompt user for the total number of tables
    printf("\nEnter the Total Number of Tables at the Hotel: ");
    if (scanf("%d", &total_tables) != 1 || total_tables <= 0) {
        printf("Invalid input. Please enter a valid number of tables.\n");
        exit(EXIT_FAILURE);
    }
    printf("Hotel started.\n");
    hotel_manager_data->total_tables=total_tables;

    // Main loop to handle earnings processing
    while (1) {
        termination_data->busy_flag=0;

        if(termination_data->termination_flag == 1) {
            //printf("\nChecking the flags");
            for(int i=0; i<total_tables; i++) {
                if(hotel_manager_data->free[i]==2) {
                    //printf("\nfree[i]=2 for i= %d", i);
                    termination_data->busy_flag=1;
                    sleep(3);
                    break;
                }
            }
            if(termination_data->busy_flag!=1) {
                break;
            }

        }

        if (hotel_manager_data->earnings_flag != 0) {
            // Write earnings to file
            int flag=hotel_manager_data->earnings_flag;
            write_earnings_to_file(hotel_manager_data->earnings[flag-1], hotel_manager_data->earnings_flag);

            // Update total earnings
            hotel_manager_data->total_earnings += hotel_manager_data->earnings[flag-1];

            // Reset earnings and earnings flag
            hotel_manager_data->earnings[flag-1]=0;
            hotel_manager_data->earnings_flag = 0;
        }
        sleep(1);
    }


    if (hotel_manager_data->total_earnings > 0) {
        hotel_manager_data->total_wages = hotel_manager_data->total_earnings * 0.4;
        hotel_manager_data->total_profit = hotel_manager_data->total_earnings - hotel_manager_data->total_wages;

        // Display and write total earnings, total wages, and total profit
        printf("Total Earnings of Hotel: %d INR\n", hotel_manager_data->total_earnings);
        printf("Total Wages of Waiters: %d INR\n", hotel_manager_data->total_wages);
        printf("Total Profit: %d INR\n", hotel_manager_data->total_profit);

        // Write to the earnings file
        FILE *earnings_fp = fopen(EARNINGS_FILE, "a"); // Open file in append mode
        if (earnings_fp == NULL) {
            perror("Error opening earnings file");
            exit(EXIT_FAILURE);
        }
        fprintf(earnings_fp, "Total Earnings of Hotel: %d INR\n", hotel_manager_data->total_earnings);
        fprintf(earnings_fp, "Total Wages of Waiters: %d INR\n", hotel_manager_data->total_wages);
        fprintf(earnings_fp, "Total Profit: %d INR\n", hotel_manager_data->total_profit);
        fclose(earnings_fp);
    }
    printf("\n-------------Thank you for visiting the Hotel!-------------\n\n");


    
    // Detach shared memory segments
    if (shmdt(hotel_manager_data) == -1) { // HOTEL MANAGER's SHM DETACHED
        perror("shmdt");
        exit(EXIT_FAILURE);
    }

    if (shmdt(termination_data) == -1) { // ADMIN's SHM DETACHED
        perror("shmdt");
        exit(EXIT_FAILURE);
    }

    sleep(7);
    // Delete shared memory segments
    if (shmctl(shmid, IPC_RMID, NULL) == -1) { // HOTEL MANAGER's SHM DELETION
        perror("shmctl");
        exit(EXIT_FAILURE);
    }

    if (shmctl(termination_shmid, IPC_RMID, NULL) == -1) { // ADMIN's SHM DELETION
        perror("shmctl");
        exit(EXIT_FAILURE);
    }

    return 0;
}
