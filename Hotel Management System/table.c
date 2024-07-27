#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/shm.h>

#define MENU_FILE "menu.txt"

typedef struct {
    int table_number;
    int num_customers;
    int orders[5][10]; // Array to hold orders for each customer
    int total_bill;
    int order_status; // Flag to indicate the status of the order and bill
                      // -2: Default
                      // -1: Terminate
                      // 0: Customers on table
                      // 1: Order given 
                      // 2: Bill Calculated and customers leave. (No Customers on table)
} TableData;

int isSerialValid(int serial) {
    FILE *menu_fp = fopen(MENU_FILE, "r");
    if (menu_fp == NULL) {
        perror("Error opening menu file");
        exit(EXIT_FAILURE);
    }
    
    int valid = 0;
    int serial_number;
    char line[100];
    
    while (fgets(line, sizeof(line), menu_fp) != NULL) {
        if (sscanf(line, "%d", &serial_number) == 1 && serial_number == serial) {
            valid = 1;
            break;
        }
    }
    
    fclose(menu_fp);
    return valid;
}

int main() {
    int num_customers;
    TableData *table_data;
    int shmid;
    key_t key;
    int table_number;
    printf("\nEnter Table Number: ");
    if (scanf("%d", &table_number) != 1) {
        printf("Invalid input. Please enter a valid number.\n");
        exit(EXIT_FAILURE);
    }

    // Generate key using table number
    key = ftok("table.c", table_number);
    if (key == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }

    // Create shared memory segment
    shmid = shmget(key, sizeof(TableData), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    // Attach shared memory segment
    table_data = (TableData *)shmat(shmid, NULL, 0);
    if (table_data == (void *)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);

    }
    table_data->table_number = table_number;
    table_data->order_status=-2;
    printf("Waiting for waiter...\n");
    while (table_data->order_status==-2) {
        sleep(3); // Check every 3 seconds if the waiter is connected or no to know if the table number is valid
        if(table_data->order_status==-1) {
            printf("\n---This table number doesn't exist. Exiting---\n\n");
            exit(EXIT_FAILURE);
        }
    }

    table_data->total_bill=0;
    

    while (1) {
        // Ask user if they wish to seat a new set of customers
        printf("Enter the number of customers to seat at the table (-1 to terminate): ");
        if (scanf("%d", &num_customers) != 1) {
            printf("Invalid input. Please enter a valid number.\n");
            continue;
        }

        if (num_customers == -1) {
            // Set flag to terminate the table process
            table_data->order_status = -1;
            break;
        } else if (num_customers < 1 || num_customers > 5) {
            printf("Invalid input. Please enter a number between 1 and 5 or -1 to terminate.\n");
            continue;
        } else {
            // Reset order status
            table_data->order_status = 0;
            table_data->num_customers = num_customers;

            // Display menu
            FILE *menu_fp = fopen(MENU_FILE, "r");
            if (menu_fp == NULL) {
                perror("Error opening menu file");
                exit(EXIT_FAILURE);
            }

            printf("Menu:\n");
            char line[100];
            while (fgets(line, sizeof(line), menu_fp) != NULL) {
                printf("%s", line);
            }
            fclose(menu_fp);

            // Creating pipes for communication between table process and each customer process
            int pipes[num_customers][2];
            for (int i = 0; i < num_customers; i++) {
                if (pipe(pipes[i]) == -1) {
                    perror("pipe");
                    exit(EXIT_FAILURE);
                }
            }

            // Forking and executing child processes sequentially
            for (int i = 0; i < num_customers; i++) {
                pid_t pid = fork();
                if (pid == -1) {
                    perror("fork");
                    exit(EXIT_FAILURE);
                } else if (pid == 0) {  // Child process (customer)
                    // Close read end of pipe
                    close(pipes[i][0]);

                    printf("Customer %d's order:\n", getpid());

                    int orders[10]; // Array to hold orders for each customer
                    int order_index = 0;
                    int order;
                    do {
                        printf("Enter the serial number of the item to order from the menu. Enter -1 when done: ");
                        scanf("%d", &order);
                        if (order!=-1 && !isSerialValid(order))  {
                            printf("Invalid input or serial number. Please enter a valid serial number.\n");
                            continue;
                        }
                        if (order != -1) {
                            orders[order_index++] = order;
                        }
                    } while (order != -1);

                    // Send orders back via the pipe
                    write(pipes[i][1], orders, order_index * sizeof(int));

                    // Close write end of pipe
                    close(pipes[i][1]);
                    exit(EXIT_SUCCESS);

                } else { // Parent process (table)
                    // Close write end of pipe
                    close(pipes[i][1]);

                    // Read orders from pipe and store them
                    int orders[10]; // Array to hold orders for each customer
                    int order_index = 0;
                    ssize_t bytes_read;
                    while ((bytes_read = read(pipes[i][0], orders, sizeof(orders))) > 0) {
                        // Copy orders to table_data
                        for (int j = 0; j < bytes_read / sizeof(int); j++) {
                            table_data->orders[i][order_index++] = orders[j];
                        }
                    }

                    // Close read end of pipe
                    close(pipes[i][0]);

                    // Wait for child process to finish
                    wait(NULL);
                }
            }
            table_data->order_status = 1; // Orders are given but bill isn't calculated
            printf("Waiting for waiter...\n");
            
            while (table_data->order_status != 2) {
                if(table_data->order_status==-1) {
                    exit(EXIT_FAILURE);
                }
                sleep(3); // Check every 3 seconds if the bill is calculated
            }
            
            // Print total bill
            printf("Total Bill for Table: %d\n", table_data->total_bill);
            printf("\n----Table Vacant---\n\n");
            // Reset orders array
            for (int i = 0; i < table_data->num_customers; i++) {
                for (int j = 0; j < 10; j++) {
                    table_data->orders[i][j] = 0;
                }
            }
            // Reset Bill
            table_data->total_bill=0;
        }
    }

    // Detach and delete shared memory segment
    if (shmdt(table_data) == -1) {
        perror("shmdt");
        exit(EXIT_FAILURE);
    }

    return 0;
}
