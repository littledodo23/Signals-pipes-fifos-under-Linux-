#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "matrix.h"
#include "worker_pool.h"

// ===== Safe Input Helpers =====

int get_int_input(const char *prompt, int min, int max) {
    char line[64];
    int value;
    while (1) {
        printf("%s", prompt);

        if (!fgets(line, sizeof(line), stdin)) {
            printf("Input error. Try again.\n");
            continue;
        }

        if (sscanf(line, "%d", &value) != 1) {
            printf("Invalid input. Please enter a number.\n");
            continue;
        }

        if (min != max && (value < min || value > max)) {
            printf("Please enter a number between %d and %d.\n", min, max);
            continue;
        }

        return value;
    }
}

void get_string_input(const char *prompt, char *buffer, int size) {
    while (1) {
        printf("%s", prompt);
        if (!fgets(buffer, size, stdin)) {
            printf("Input error. Try again.\n");
            continue;
        }

        buffer[strcspn(buffer, "\n")] = 0; // remove newline

        if (strlen(buffer) == 0) {
            printf("Input cannot be empty. Try again.\n");
            continue;
        }

        return;
    }
}

// ===== Menu =====

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
    printf("10. Add 2 matrices (multi-process)\n");
    printf("11. Subtract 2 matrices (multi-process)\n");
    printf("12. Multiply 2 matrices (multi-process)\n");
    printf("13. Find determinant of a matrix\n");
    printf("14. Benchmark operations (parallel vs single)\n");
    printf("15. Exit\n");
    printf("=======================================\n");
}

// ===== Selection and Operation Menus =====

Matrix* select_matrix(const char *prompt) {
    if (matrix_count == 0) {
        printf("No matrices in memory.\n");
        return NULL;
    }

    printf("\n%s\n", prompt);
    printf("Available matrices:\n");
    for (int i = 0; i < matrix_count; i++) {
        printf("%d. %s (%dx%d)\n", i + 1, matrices[i]->name,
               matrices[i]->rows, matrices[i]->cols);
    }

    int choice = get_int_input("Enter choice: ", 1, matrix_count);
    return matrices[choice - 1];
}

void add_matrices_menu() {
    Matrix *m1 = select_matrix("Select first matrix to add:");
    if (!m1) return;

    Matrix *m2 = select_matrix("Select second matrix to add:");
    if (!m2) return;

    if (m1->rows != m2->rows || m1->cols != m2->cols) {
        printf("Error: Matrices must have the same dimensions for addition.\n");
        return;
    }

    printf("\nRunning parallel addition...\n");
    Matrix *result_parallel = add_matrices_parallel(m1, m2);
    if (result_parallel) {
        printf("\nResult (parallel):\n");
        print_matrix(result_parallel);
        if (matrix_count < MAX_MATRICES) {
            matrices[matrix_count++] = result_parallel;
            printf("Result saved to memory as '%s'\n", result_parallel->name);
        }
    }
}

void subtract_matrices_menu() {
    Matrix *m1 = select_matrix("Select first matrix (minuend):");
    if (!m1) return;

    Matrix *m2 = select_matrix("Select second matrix (subtrahend):");
    if (!m2) return;

    if (m1->rows != m2->rows || m1->cols != m2->cols) {
        printf("Error: Matrices must have the same dimensions for subtraction.\n");
        return;
    }

    printf("\nRunning parallel subtraction...\n");
    Matrix *result_parallel = subtract_matrices_parallel(m1, m2);
    if (result_parallel) {
        printf("\nResult (parallel):\n");
        print_matrix(result_parallel);
        if (matrix_count < MAX_MATRICES) {
            matrices[matrix_count++] = result_parallel;
            printf("Result saved to memory as '%s'\n", result_parallel->name);
        }
    }
}

void multiply_matrices_menu() {
    Matrix *m1 = select_matrix("Select first matrix:");
    if (!m1) return;

    Matrix *m2 = select_matrix("Select second matrix:");
    if (!m2) return;

    if (m1->cols != m2->rows) {
        printf("Error: Columns of the first matrix must equal rows of the second.\n");
        return;
    }

    printf("\nRunning parallel multiplication...\n");
    Matrix *result_parallel = multiply_matrices_parallel(m1, m2);
    if (result_parallel) {
        printf("\nResult (parallel):\n");
        print_matrix(result_parallel);
        if (matrix_count < MAX_MATRICES) {
            matrices[matrix_count++] = result_parallel;
            printf("Result saved to memory as '%s'\n", result_parallel->name);
        }
    }
}

void determinant_menu() {
    Matrix *m = select_matrix("Select matrix for determinant:");
    if (!m) return;

    if (m->rows != m->cols) {
        printf("Error: Matrix must be square.\n");
        return;
    }

    printf("Calculating determinant...\n");
    double det = determinant_parallel(m);
    printf("Determinant of %s = %.4f\n", m->name, det);
}

void eigen_menu() {
    Matrix *m = select_matrix("Select matrix for eigenvalue/vector computation:");
    if (!m) return;

    if (m->rows != m->cols) {
        printf("Error: Matrix must be square.\n");
        return;
    }

    int n = m->rows;
    double eigenvector[n];
    double eigenvalue;

    eigen_parallel(m, &eigenvalue, eigenvector);

    printf("\nLargest eigenvalue: %.6lf\n", eigenvalue);
    printf("Corresponding eigenvector:\n");
    for (int i = 0; i < n; i++)
        printf("%8.4lf\n", eigenvector[i]);
}

void benchmark_menu() {
    printf("\n=== BENCHMARK MODE ===\n");
    printf("This will compare parallel vs single-threaded performance.\n");

    Matrix *m1 = select_matrix("Select first matrix:");
    if (!m1) return;

    Matrix *m2 = select_matrix("Select second matrix:");
    if (!m2) return;

    if (m1->rows != m2->rows || m1->cols != m2->cols) {
        printf("Matrices must have the same dimensions for benchmark.\n");
        return;
    }

    printf("\n=== ADDITION BENCHMARK ===\n");
    Matrix *r1 = add_matrices_parallel(m1, m2);
    Matrix *r2 = add_matrices_single(m1, m2);
    if (r1) free_matrix(r1);
    if (r2) free_matrix(r2);

    printf("\n=== SUBTRACTION BENCHMARK ===\n");
    r1 = subtract_matrices_parallel(m1, m2);
    r2 = subtract_matrices_single(m1, m2);
    if (r1) free_matrix(r1);
    if (r2) free_matrix(r2);

    if (m1->cols == m2->rows) {
        printf("\n=== MULTIPLICATION BENCHMARK ===\n");
        r1 = multiply_matrices_parallel(m1, m2);
        r2 = multiply_matrices_single(m1, m2);
        if (r1) free_matrix(r1);
        if (r2) free_matrix(r2);
    }

    printf("\n=== BENCHMARK COMPLETE ===\n");
}

int main() {
    int choice;

    printf("Matrix Operations with Multi-Processing\n");
    printf("Real-Time & Embedded Systems Project\n\n");

    setup_signal_handlers();

    int pool_sz = get_int_input("Enter worker pool size (recommended 4-8): ", 1, MAX_WORKERS);
    init_worker_pool(pool_sz);

    while (1) {
        show_menu();
        choice = get_int_input("Enter your choice: ", 1, 15);

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
            case 10: add_matrices_menu(); break;
            case 11: subtract_matrices_menu(); break;
            case 12: multiply_matrices_menu(); break;
            case 13: determinant_menu(); break;
            case 14: benchmark_menu(); break;
            case 15:
                printf("\nCleaning up worker pool...\n");
                cleanup_worker_pool();
                for (int i = 0; i < matrix_count; i++)
                    free_matrix(matrices[i]);
                printf("Goodbye.\n");
                exit(0);
            default:
                printf("Invalid choice. Try again.\n");
        }

        age_workers();
    }

    return 0;
}
