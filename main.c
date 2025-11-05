#include <stdio.h>
#include <stdlib.h>
#include "matrix.h"
#include "worker_pool.h"
#include "eigen.h"

void clear_input_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

int get_int_input(const char *prompt, int min, int max) {
    int value;
    char ch;
    while (1) {
        printf("%s", prompt);
        if (scanf("%d%c", &value, &ch) != 2 || ch != '\n') {
            printf("Invalid input. Please enter a number.\n");
            clear_input_buffer();
            continue;
        }
        if (value < min || value > max) {
            printf("Input out of range (%d - %d). Try again.\n", min, max);
            continue;
        }
        return value;
    }
}

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
    printf("10. Add 2 matrices (OpenMP parallel)\n");
    printf("11. Subtract 2 matrices (OpenMP parallel)\n");
    printf("12. Multiply 2 matrices (OpenMP parallel)\n");
    printf("13. Find determinant of a matrix\n");
    printf("14. Find eigenvalues & eigenvectors\n");
    printf("15. Benchmark operations (parallel vs single)\n");
    printf("16. Exit\n");
    printf("=======================================\n");
}

Matrix* select_matrix(const char *prompt) {
    if (matrix_count == 0) {
        printf("No matrices in memory.\n");
        return NULL;
    }

    printf("\n%s\n", prompt);
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
        printf("Error: Matrices must have same dimensions for addition.\n");
        return;
    }

    printf("\n--- Running PARALLEL addition ---\n");
    Matrix *result_parallel = add_matrices_parallel(m1, m2);

    printf("\n--- Running SINGLE-THREADED addition ---\n");
    Matrix *result_single = add_matrices_single(m1, m2);

    if (result_parallel) {
        printf("\nResult (parallel):\n");
        print_matrix(result_parallel);

        if (matrix_count < MAX_MATRICES) {
            matrices[matrix_count++] = result_parallel;
            printf("Result saved to memory as '%s'.\n", result_parallel->name);
        }
    }

    if (result_single) {
        free_matrix(result_single);
    }
}

void subtract_matrices_menu() {
    Matrix *m1 = select_matrix("Select first matrix (minuend):");
    if (!m1) return;

    Matrix *m2 = select_matrix("Select second matrix (subtrahend):");
    if (!m2) return;

    if (m1->rows != m2->rows || m1->cols != m2->cols) {
        printf("Error: Matrices must have same dimensions for subtraction.\n");
        return;
    }

    printf("\n--- Running PARALLEL subtraction ---\n");
    Matrix *result_parallel = subtract_matrices_parallel(m1, m2);

    printf("\n--- Running SINGLE-THREADED subtraction ---\n");
    Matrix *result_single = subtract_matrices_single(m1, m2);

    if (result_parallel) {
        printf("\nResult (parallel):\n");
        print_matrix(result_parallel);

        if (matrix_count < MAX_MATRICES) {
            matrices[matrix_count++] = result_parallel;
            printf("Result saved to memory as '%s'.\n", result_parallel->name);
        }
    }

    if (result_single) {
        free_matrix(result_single);
    }
}

void multiply_matrices_menu() {
    Matrix *m1 = select_matrix("Select first matrix:");
    if (!m1) return;

    Matrix *m2 = select_matrix("Select second matrix:");
    if (!m2) return;

    if (m1->cols != m2->rows) {
        printf("Error: Invalid dimensions for multiplication.\n");
        return;
    }

    printf("\n--- Running PARALLEL multiplication ---\n");
    Matrix *result_parallel = multiply_matrices_parallel(m1, m2);

    printf("\n--- Running SINGLE-THREADED multiplication ---\n");
    Matrix *result_single = multiply_matrices_single(m1, m2);

    if (result_parallel) {
        printf("\nResult (parallel):\n");
        print_matrix(result_parallel);

        if (matrix_count < MAX_MATRICES) {
            matrices[matrix_count++] = result_parallel;
            printf("Result saved to memory as '%s'.\n", result_parallel->name);
        }
    }

    if (result_single) {
        free_matrix(result_single);
    }
}

void determinant_menu() {
    Matrix *m = select_matrix("Select matrix for determinant:");
    if (!m) return;

    if (m->rows != m->cols) {
        printf("Error: Matrix must be square!\n");
        return;
    }

    printf("\n=== DETERMINANT CALCULATION ===\n");
    printf("Matrix: %s (%dx%d)\n\n", m->name, m->rows, m->cols);

    printf("--- Running PARALLEL determinant (OpenMP) ---\n");
    double det_parallel = determinant_parallel(m);

    printf("\n--- Running SINGLE-THREADED determinant ---\n");
    double det_single = determinant_single(m);

    printf("\n=== RESULTS ===\n");
    printf("Determinant (parallel):        %.6f\n", det_parallel);
    printf("Determinant (single-threaded): %.6f\n", det_single);
    printf("Difference:                    %.10f (should be ~0)\n", 
           det_parallel - det_single);
}

void eigenvalues_menu() {
    Matrix *m = select_matrix("Select matrix for eigenvalue computation:");
    if (!m) return;

    if (m->rows != m->cols) {
        printf("Error: Matrix must be square!\n");
        return;
    }

    printf("\n=== EIGENVALUE & EIGENVECTOR CALCULATION ===\n");
    printf("Matrix: %s (%dx%d)\n", m->name, m->rows, m->cols);
    
    int num_eigen = get_int_input("How many eigenvalues to compute? (1 to %d): ", 
                                   1, m->rows);

    printf("\n--- Running PARALLEL eigen computation (OpenMP) ---\n");
    EigenResult *result_parallel = compute_eigen_parallel(m, num_eigen);

    printf("\n--- Running SINGLE-THREADED eigen computation ---\n");
    EigenResult *result_single = compute_eigen_single(m, num_eigen);

    if (result_parallel) {
        printf("\n=== PARALLEL RESULTS ===");
        print_eigen_result(result_parallel, m->rows);
        free_eigen_result(result_parallel);
    }

    if (result_single) {
        printf("\n=== SINGLE-THREADED RESULTS ===");
        print_eigen_result(result_single, m->rows);
        free_eigen_result(result_single);
    }

    printf("\nNote: For large matrices, eigenvalue computation can be intensive.\n");
    printf("The dominant eigenvalue (largest magnitude) has a computed eigenvector.\n");
}

void benchmark_menu() {
    printf("\n=== BENCHMARK MODE ===\n");
    printf("This will compare parallel vs single-threaded performance.\n\n");

    Matrix *m1 = select_matrix("Select first matrix:");
    if (!m1) return;

    Matrix *m2 = select_matrix("Select second matrix:");
    if (!m2) return;

    if (m1->rows != m2->rows || m1->cols != m2->cols) {
        printf("Warning: Matrices have different dimensions.\n");
        printf("Only operations with compatible dimensions will be benchmarked.\n\n");
    }

    // Addition benchmark
    if (m1->rows == m2->rows && m1->cols == m2->cols) {
        printf("\n=== ADDITION BENCHMARK ===\n");
        Matrix *r1 = add_matrices_parallel(m1, m2);
        Matrix *r2 = add_matrices_single(m1, m2);
        if (r1) free_matrix(r1);
        if (r2) free_matrix(r2);
    }

    // Subtraction benchmark
    if (m1->rows == m2->rows && m1->cols == m2->cols) {
        printf("\n=== SUBTRACTION BENCHMARK ===\n");
        Matrix *r1 = subtract_matrices_parallel(m1, m2);
        Matrix *r2 = subtract_matrices_single(m1, m2);
        if (r1) free_matrix(r1);
        if (r2) free_matrix(r2);
    }

    // Multiplication benchmark
    if (m1->cols == m2->rows) {
        printf("\n=== MULTIPLICATION BENCHMARK ===\n");
        Matrix *r1 = multiply_matrices_parallel(m1, m2);
        Matrix *r2 = multiply_matrices_single(m1, m2);
        if (r1) free_matrix(r1);
        if (r2) free_matrix(r2);
    }

    // Determinant benchmarks (if square matrices)
    if (m1->rows == m1->cols) {
        printf("\n=== DETERMINANT BENCHMARK (Matrix 1: %s) ===\n", m1->name);
        double d1 = determinant_parallel(m1);
        double d2 = determinant_single(m1);
        printf("Determinant value: %.6f\n", d1);
    }

    if (m2->rows == m2->cols && m2 != m1) {
        printf("\n=== DETERMINANT BENCHMARK (Matrix 2: %s) ===\n", m2->name);
        double d1 = determinant_parallel(m2);
        double d2 = determinant_single(m2);
        printf("Determinant value: %.6f\n", d1);
    }

    // Eigenvalue benchmark (if square and not too large)
    if (m1->rows == m1->cols && m1->rows <= 10) {
        printf("\n=== EIGENVALUE BENCHMARK (Matrix 1: %s) ===\n", m1->name);
        printf("Computing dominant eigenvalue...\n");
        EigenResult *e1 = compute_eigen_parallel(m1, 1);
        EigenResult *e2 = compute_eigen_single(m1, 1);
        if (e1) {
            printf("Dominant eigenvalue: %.6f\n", e1->eigenvalues[0]);
            free_eigen_result(e1);
        }
        if (e2) free_eigen_result(e2);
    }

    if (m2->rows == m2->cols && m2 != m1 && m2->rows <= 10) {
        printf("\n=== EIGENVALUE BENCHMARK (Matrix 2: %s) ===\n", m2->name);
        printf("Computing dominant eigenvalue...\n");
        EigenResult *e1 = compute_eigen_parallel(m2, 1);
        EigenResult *e2 = compute_eigen_single(m2, 1);
        if (e1) {
            printf("Dominant eigenvalue: %.6f\n", e1->eigenvalues[0]);
            free_eigen_result(e1);
        }
        if (e2) free_eigen_result(e2);
    }

    printf("\n=== BENCHMARK COMPLETE ===\n");
}

int main() {
    int choice;

    printf("===========================================\n");
    printf(" Matrix Operations with Multi-Processing\n");
    printf(" Real-Time & Embedded Systems Project\n");
    printf("===========================================\n\n");

    setup_signal_handlers();

    int pool_sz;
    pool_sz = get_int_input("Enter worker pool size (recommended: 4-8): ", 1, MAX_WORKERS);
    init_worker_pool(pool_sz);

    while (1) {
        show_menu();
        choice = get_int_input("Enter your choice: ", 1, 16);

        switch (choice) {
            case 1: enter_matrix(); break;
            case 2: display_matrix(); break;
            case 3: delete_matrix(); break;
            case 4: modify_matrix(); break;
            case 5: read_matrix_from_file_option(); break;
            case 6: read_matrices_from_folder_option(); break;
            case 7: save_matrix_to_file_option(); break;
            case 8: save_all_matrices_to_folder_option(); break;
            case 9: display_all_matrices(); break;
            case 10: add_matrices_menu(); break;
            case 11: subtract_matrices_menu(); break;
            case 12: multiply_matrices_menu(); break;
            case 13: determinant_menu(); break;
            case 14: eigenvalues_menu(); break;
            case 15: benchmark_menu(); break;
            case 16:
                printf("\nCleaning up worker pool...\n");
                cleanup_worker_pool();

                printf("Freeing matrices...\n");
                for (int i = 0; i < matrix_count; i++) {
                    free_matrix(matrices[i]);
                }

                printf("Goodbye!\n");
                exit(0);
            default:
                printf("Invalid choice. Please try again.\n");
        }

        age_workers();
    }

    return 0;
}
