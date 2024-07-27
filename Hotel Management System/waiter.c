#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>


#define MENU_FILE "menu.txt"

typedef struct {
    int table_number;
    int num_customers;
    int orders[5][10]; // Array to hold orders for each customer
    int total_bill;
    int order_status; // Flag to indicate the status of orders
} TableData;

// Structure to hold earnings information and flags
typedef struct {
    int total_tables;       // Total number of tables in the restaurant
    int earnings_flag;      // Table number for which new earnings have been received (0 for no new earnings)
    int earnings[10];       // Earnings from the latest set of customers
    int total_earnings;     // Total earnings of the hotel
    int total_wages;        // Total wages of waiters (40% of total earnings)
    int total_profit;       // Total profit made by the hotel (total earnings - total wages)
    int free[10];           // If the waiter is free then 1 else it is 2
} HotelManagerData;




int getPrice(int item_number) {
    FILE *menu_fp = fopen(MENU_FILE, "r");
    if (menu_fp == NULL) {
        perror("Error opening menu file");
        exit(EXIT_FAILURE);
    }

    char line[100];
    int price = 0;
    while (fgets(line, sizeof(line), menu_fp) != NULL) {
        int menu_item;
        if (sscanf(line, "%d", &menu_item) == 1 && menu_item == item_number) {
            // Extract price from the line
            sscanf(line, "%*d. %*[^0-9]%d", &price);
            break;
        }
    }

    fclose(menu_fp);
    return price;
}

int main() {
    int waiter_id;
    TableData *table_data;
    HotelManagerData *hotel_manager_data;
    int shmid1;
    int shmid2;
    key_t key1;
    key_t key2;

 
    // Prompt for waiter ID
    printf("\nEnter Waiter ID: ");
    if (scanf("%d", &waiter_id) != 1) {
        printf("Invalid input. Please enter a valid Waiter ID.\n");
        exit(EXIT_FAILURE);
    }


    //------------------------Connecting to Table SHM------------------------------------
    // Generate key using ftok()
    key1 = ftok("table.c", waiter_id);
    if (key1 == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }

    // Get the shared memory segment ID
    shmid1 = shmget(key1, sizeof(TableData), 0666);
    if (shmid1 == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    // Attach shared memory segment
    table_data = (TableData *)shmat(shmid1, NULL, 0);
    if (table_data == (void *)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }
    //------------------------------------------------------------------------------------

    //------------------------Connecting to Hotel Manager SHM-----------------------------
    // Generate key using ftok()
    key2 = ftok("hotelmanager.c", 'A');
    if (key2 == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }

    // Get the shared memory segment ID
    shmid2 = shmget(key2, sizeof(HotelManagerData), 0666);
    if (shmid2 == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    // Attach shared memory segment
    hotel_manager_data = (HotelManagerData *)shmat(shmid2, NULL, 0);
    if (hotel_manager_data == (void *)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }
    //------------------------------------------------------------------------------------



    if(table_data->table_number>hotel_manager_data->total_tables) {
        printf("\n---This table number doesn't exist. Exiting--- \n\n");
        table_data->order_status=-1;

        // Detach and Delete from SHM (Table SHM)
        if (shmdt(table_data) == -1) {
            perror("shmdt");
            exit(EXIT_FAILURE);
        }
        if (shmctl(shmid1, IPC_RMID, NULL) == -1) {
            perror("shmctl");
            exit(EXIT_FAILURE);
        }
        exit(EXIT_FAILURE);


    }
    table_data->order_status=2;
    hotel_manager_data->free[waiter_id-1]=2;



    // Loop until order_status flag is set to -1
    while (table_data->order_status != -1) {

        // Check if the order is given but the bill isn't calculated
        if (table_data->order_status == 1) {
            // Print previously written customer orders
            for (int i = 0; i < table_data->num_customers; i++) {
                int total_price = 0;
                printf("Customer %d's Orders:\n", i + 1);
                for (int j = 0; j < 10; j++) {
                    if (table_data->orders[i][j] != 0) {
                        int item_price = getPrice(table_data->orders[i][j]);
                        printf("Item %d: Price %d\n", table_data->orders[i][j], item_price);
                        total_price += item_price;
                    } else {
                        break;
                    }
                }
                printf("Total Bill for Customer %d: %d\n", i + 1, total_price);
                table_data->total_bill += total_price; // Update total bill for the table
            }
            printf("Total Bill for Table: %d\n", table_data->total_bill);

            // Sending the earnings to hotel manager
            hotel_manager_data->earnings_flag=waiter_id;
            hotel_manager_data->earnings[waiter_id-1]=table_data->total_bill;
            // Notify the table that bill calculation is done
            table_data->order_status = 2;
        }

        // Wait for the next update
        sleep(3);
    }
    hotel_manager_data->free[waiter_id-1]=1;
    printf("\n---Waiter Free---\n\n");
    // Detach from shared memory segment (Hotel Manager SHM)
    if (shmdt(hotel_manager_data) == -1) {
        perror("shmdt");
        exit(EXIT_FAILURE);
    }

    // Detach from shared memory segment (Table SHM)
    if (shmdt(table_data) == -1) {
        perror("shmdt");
        exit(EXIT_FAILURE);
    }

    // Delete from SHM (Table SHM)
    if (shmctl(shmid1, IPC_RMID, NULL) == -1) {
        perror("shmctl");
        exit(EXIT_FAILURE);
    }


    return 0;
}
