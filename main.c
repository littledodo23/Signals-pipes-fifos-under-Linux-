#include <stdio.h>
#include <stdlib.h>
#include "matrix.h"
#include "worker_pool.h"
#include "eigen.h"
#include "config.h"
#include "file_io.h"

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
    printf("10. Add 2 matrices\n");
    printf("11. Subtract 2 matrices\n");
    printf("12. Multiply 2 matrices\n");
    printf("13. Find determinant of a matrix\n");
    printf("14. Find eigenvalues & eigenvectors\n");
    printf("15. Exit\n");
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

    printf("\n=== ADDITION OPERATION ===\n");
    printf("Matrix: %s (%dx%d) + %s (%dx%d)\n\n", 
           m1->name, m1->rows, m1->cols, m2->name, m2->rows, m2->cols);

    // ⭐ 1. Multi-Process Version
    printf("[1/3] Running MULTI-PROCESS version (Pipes + Signals)...\n");
    double start_ipc = get_time_ms();
    Matrix *result_ipc = add_matrices_with_processes(m1, m2);
    double time_ipc = get_time_ms() - start_ipc;

    // ⭐ 2. Single-Threaded Version
    printf("[2/3] Running SINGLE-THREADED version...\n");
    double start_single = get_time_ms();
    Matrix *result_single = add_matrices_single(m1, m2);
    double time_single = get_time_ms() - start_single;

    // ⭐ 3. OpenMP Version
    printf("[3/3] Running OpenMP version (Shared Memory Parallelism)...\n");
    double start_omp = get_time_ms();
    Matrix *result_omp = add_matrices_openmp(m1, m2);
    double time_omp = get_time_ms() - start_omp;

    // Performance Comparison
    printf("\n=== PERFORMANCE COMPARISON ===\n");
    printf("Multi-process (Pipes+Signals): %.2f ms\n", time_ipc);
    printf("Single-threaded:               %.2f ms\n", time_single);
    printf("OpenMP (Shared Memory):        %.2f ms\n", time_omp);
    printf("─────────────────────────────────────\n");
    if (time_ipc > 0) {
        printf("Speedup (IPC vs Single):       %.2fx\n", time_single / time_ipc);
    }
    if (time_omp > 0) {
        printf("Speedup (OpenMP vs Single):    %.2fx\n", time_single / time_omp);
    }
    printf("\n💡 Note: OpenMP typically performs better for smaller matrices\n");
    printf("   while multi-process excels with larger workloads.\n");

    if (result_ipc) {
        printf("\nResult:\n");
        print_matrix(result_ipc);
        if (matrix_count < MAX_MATRICES) {
            matrices[matrix_count++] = result_ipc;
            printf("Result saved to memory as '%s'.\n", result_ipc->name);
        }
    }

    if (result_single) free_matrix(result_single);
    if (result_omp) free_matrix(result_omp);
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

    printf("\n=== SUBTRACTION OPERATION ===\n");
    printf("Matrix: %s (%dx%d) - %s (%dx%d)\n\n", 
           m1->name, m1->rows, m1->cols, m2->name, m2->rows, m2->cols);

    // ⭐ 1. Multi-Process Version
    printf("[1/3] Running MULTI-PROCESS version (Pipes + Signals)...\n");
    double start_ipc = get_time_ms();
    Matrix *result_ipc = subtract_matrices_with_processes(m1, m2);
    double time_ipc = get_time_ms() - start_ipc;

    // ⭐ 2. Single-Threaded Version
    printf("[2/3] Running SINGLE-THREADED version...\n");
    double start_single = get_time_ms();
    Matrix *result_single = subtract_matrices_single(m1, m2);
    double time_single = get_time_ms() - start_single;

    // ⭐ 3. OpenMP Version
    printf("[3/3] Running OpenMP version (Shared Memory Parallelism)...\n");
    double start_omp = get_time_ms();
    Matrix *result_omp = subtract_matrices_openmp(m1, m2);
    double time_omp = get_time_ms() - start_omp;

    // Performance Comparison
    printf("\n=== PERFORMANCE COMPARISON ===\n");
    printf("Multi-process (Pipes+Signals): %.2f ms\n", time_ipc);
    printf("Single-threaded:               %.2f ms\n", time_single);
    printf("OpenMP (Shared Memory):        %.2f ms\n", time_omp);
    printf("─────────────────────────────────────\n");
    if (time_ipc > 0) {
        printf("Speedup (IPC vs Single):       %.2fx\n", time_single / time_ipc);
    }
    if (time_omp > 0) {
        printf("Speedup (OpenMP vs Single):    %.2fx\n", time_single / time_omp);
    }

    if (result_ipc) {
        printf("\nResult:\n");
        print_matrix(result_ipc);
        if (matrix_count < MAX_MATRICES) {
            matrices[matrix_count++] = result_ipc;
            printf("Result saved to memory as '%s'.\n", result_ipc->name);
        }
    }

    if (result_single) free_matrix(result_single);
    if (result_omp) free_matrix(result_omp);
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

    if (m1->cols > MAX_VECTOR_SIZE) {
        printf("Error: Matrix dimension exceeds IPC buffer limit (%d).\n", MAX_VECTOR_SIZE);
        return;
    }

    printf("\n=== MULTIPLICATION OPERATION ===\n");
    printf("Matrix: %s (%dx%d) × %s (%dx%d)\n\n", 
           m1->name, m1->rows, m1->cols, m2->name, m2->rows, m2->cols);

    // ⭐ 1. Multi-Process Version
    printf("[1/3] Running MULTI-PROCESS version (Pipes + Signals)...\n");
    double start_ipc = get_time_ms();
    Matrix *result_ipc = multiply_matrices_with_processes(m1, m2);
    double time_ipc = get_time_ms() - start_ipc;

    // ⭐ 2. Single-Threaded Version
    printf("[2/3] Running SINGLE-THREADED version...\n");
    double start_single = get_time_ms();
    Matrix *result_single = multiply_matrices_single(m1, m2);
    double time_single = get_time_ms() - start_single;

    // ⭐ 3. OpenMP Version
    printf("[3/3] Running OpenMP version (Shared Memory Parallelism)...\n");
    double start_omp = get_time_ms();
    Matrix *result_omp = multiply_matrices_openmp(m1, m2);
    double time_omp = get_time_ms() - start_omp;

    // Performance Comparison
    printf("\n=== PERFORMANCE COMPARISON ===\n");
    printf("Multi-process (Pipes+Signals): %.2f ms\n", time_ipc);
    printf("Single-threaded:               %.2f ms\n", time_single);
    printf("OpenMP (Shared Memory):        %.2f ms\n", time_omp);
    printf("─────────────────────────────────────\n");
    if (time_ipc > 0) {
        printf("Speedup (IPC vs Single):       %.2fx\n", time_single / time_ipc);
    }
    if (time_omp > 0) {
        printf("Speedup (OpenMP vs Single):    %.2fx\n", time_single / time_omp);
    }

    if (result_ipc) {
        printf("\nResult:\n");
        print_matrix(result_ipc);
        if (matrix_count < MAX_MATRICES) {
            matrices[matrix_count++] = result_ipc;
            printf("Result saved to memory as '%s'.\n", result_ipc->name);
        }
    }

    if (result_single) free_matrix(result_single);
    if (result_omp) free_matrix(result_omp);
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

    // ⭐ 1. Multi-Process Version
    printf("[1/3] Running MULTI-PROCESS version (Pipes + Signals)...\n");
    double start_parallel = get_time_ms();
    double det_parallel = determinant_parallel(m);
    double time_parallel = get_time_ms() - start_parallel;

    // ⭐ 2. Single-Threaded Version
    printf("[2/3] Running SINGLE-THREADED version...\n");
    double start_single = get_time_ms();
    double det_single = determinant_single(m);
    double time_single = get_time_ms() - start_single;

    // ⭐ 3. OpenMP Version
    printf("[3/3] Running OpenMP version (Shared Memory Parallelism)...\n");
    double start_omp = get_time_ms();
    double det_omp = determinant_openmp(m);
    double time_omp = get_time_ms() - start_omp;

    // Performance Comparison
    printf("\n=== PERFORMANCE COMPARISON ===\n");
    printf("Multi-process (Pipes+Signals): %.2f ms\n", time_parallel);
    printf("Single-threaded:               %.2f ms\n", time_single);
    printf("OpenMP (Shared Memory):        %.2f ms\n", time_omp);
    printf("─────────────────────────────────────\n");
    if (time_parallel > 0) {
        printf("Speedup (IPC vs Single):       %.2fx\n", time_single / time_parallel);
    }
    if (time_omp > 0) {
        printf("Speedup (OpenMP vs Single):    %.2fx\n", time_single / time_omp);
    }

    printf("\n=== RESULTS ===\n");
    printf("Determinant (Multi-process): %.6f\n", det_parallel);
    printf("Determinant (Single-thread): %.6f\n", det_single);
    printf("Determinant (OpenMP):        %.6f\n", det_omp);
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

    double start_parallel = get_time_ms();
    EigenResult *result_parallel = compute_eigen_parallel(m, num_eigen);
    double time_parallel = get_time_ms() - start_parallel;

    double start_single = get_time_ms();
    EigenResult *result_single = compute_eigen_single(m, num_eigen);
    double time_single = get_time_ms() - start_single;

    printf("\n=== PERFORMANCE COMPARISON ===\n");
    printf("Parallel time:        %.2f ms\n", time_parallel);
    printf("Single-threaded time: %.2f ms\n", time_single);
    if (time_parallel > 0) {
        printf("Speedup factor:       %.2fx\n", time_single / time_parallel);
    }

    if (result_parallel) {
        printf("\n=== RESULTS ===");
        print_eigen_result(result_parallel, m->rows);
        free_eigen_result(result_parallel);
    }

    if (result_single) {
        free_eigen_result(result_single);
    }

    printf("\nNote: The dominant eigenvalue has a computed eigenvector.\n");
}

int main(int argc, char *argv[]) {
    printf("===========================================\n");
    printf(" Matrix Operations with Multi-Processing\n");
    printf(" Real-Time & Embedded Systems Project\n");
    printf("===========================================\n\n");

    // Load configuration file
    const char *config_file = (argc > 1) ? argv[1] : "matrix_config.txt";
    init_default_config();
    load_config(config_file);
    
    Config *cfg = get_config();

    setup_signal_handlers();

    // Initialize worker pool with configured size
    printf("\nInitializing system with %d workers...\n", cfg->worker_pool_size);
    init_worker_pool(cfg->worker_pool_size);
    max_idle_time = cfg->max_idle_time;

    // Load matrices from file (if provided as argument, else use default)
    const char *mat_file = (argc > 2) ? argv[2] : "matrix.txt";
    load_matrices_from_file(mat_file);

    int choice;
    while (1) {
        show_menu();
        choice = get_int_input("Enter your choice: ", 1, 15);

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
            case 15:
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
