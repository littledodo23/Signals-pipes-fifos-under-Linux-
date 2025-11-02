#include <stdio.h>
#include <stdlib.h>
#include "matrix.h"

void show_menu() {
    printf("\n========== Matrix Operations ==========\n");
    printf("1.  Enter a matrix\n");
    printf("2.  Display a matrix\n");
    printf("3.  Delete a matrix\n");
    printf("4.  Modify a matrix\n");
    printf("5.  Read a matrix from a file\n");
    printf("6.  Read a set of matrices from a folder\n");
    printf("7.  Save a matrix to a file\n");
    printf("8.  Save all matrices in memory to a folder\n");
    printf("9.  Display all matrices in memory\n");
    printf("15. Exit\n");
    printf("=======================================\n");
}

int main() {
    int choice;
    while (1) {
        show_menu();
        printf("Enter your choice: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1:  enter_matrix(); break;
            case 2:  display_matrix(); break;
            case 3:  delete_matrix(); break;
            case 4:  modify_matrix(); break;
            case 5:  read_matrix_from_file_option(); break;
            case 6:  read_matrices_from_folder_option(); break;
            case 7:  save_matrix_to_file_option(); break;
            case 8:  save_all_matrices_to_folder_option(); break;
            case 9:  display_all_matrices(); break;
            case 15:
                printf("Exiting...\n");
                exit(0);
            default:
                printf("Invalid choice!\n");
        }
    }
}
