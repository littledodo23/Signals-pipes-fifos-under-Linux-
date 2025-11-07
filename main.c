#include <stdio.h>
#include <stdlib.h>
#include <math.h>
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
    Config *cfg = get_config();
    
    printf("\n========== Matrix Operations ==========\n");
    
    if (cfg->use_custom_menu) {
        printf("[Using Custom Menu Order]\n");
        const char *menu_items[] = {
            "Enter a matrix",
            "Display a matrix",
            "Delete a matrix",
            "Modify a matrix",
            "Read a matrix from a file",
            "Read a set of matrices from a folder",
            "Save a matrix to a file",
            "Save all matrices in memory to a folder",
            "Display all matrices in memory",
            "Add 2 matrices",
            "Subtract 2 matrices",
            "Multiply 2 matrices",
            "Find determinant of a matrix",
            "Find eigenvalues & eigenvectors",
            "Exit"
        };
        
        for (int i = 0; i < 15; i++) {
            int item_num = cfg->menu_order[i];
            if (item_num >= 1 && item_num <= 15) {
                printf("%d.  %s\n", i + 1, menu_items[item_num - 1]);
            }
        }
    } else {
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
    }
    
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

    printf("\n=== ADDITION OPERATION - 3-WAY COMPARISON ===\n");
    
    // Method 1: Worker Pool (Reusing persistent processes)
    printf("\n[1] Using WORKER POOL (persistent processes)...\n");
    double start_pool = get_time_ms();
    Matrix *result_pool = add_matrices_with_pool(m1, m2);
    double time_pool = get_time_ms() - start_pool;

    // Method 2: Fork-based (creating new processes)
    printf("\n[2] Using FORK (new processes per element)...\n");
    double start_fork = get_time_ms();
    Matrix *result_fork = add_matrices_with_processes(m1, m2);
    double time_fork = get_time_ms() - start_fork;
    
    // Method 3: OpenMP (threading)
    printf("\n[3] Using OpenMP (threading)...\n");
    double start_omp = get_time_ms();
    Matrix *result_omp = add_matrices_openmp(m1, m2);
    double time_omp = get_time_ms() - start_omp;

    // Method 4: Single-threaded
    printf("\n[4] Using Single-threaded...\n");
    double start_single = get_time_ms();
    Matrix *result_single = add_matrices_single(m1, m2);
    double time_single = get_time_ms() - start_single;

    printf("\n=== PERFORMANCE COMPARISON ===\n");
    printf("Worker Pool time:     %.2f ms  (Speedup: %.2fx)\n", time_pool, time_single / time_pool);
    printf("Fork-based time:      %.2f ms  (Speedup: %.2fx)\n", time_fork, time_single / time_fork);
    printf("OpenMP time:          %.2f ms  (Speedup: %.2fx)\n", time_omp, time_single / time_omp);
    printf("Single-threaded time: %.2f ms  (Baseline)\n", time_single);
    
    printf("\n=== WINNER ===\n");
    double best_time = time_pool;
    const char *best_method = "Worker Pool";
    
    if (time_fork < best_time) {
        best_time = time_fork;
        best_method = "Fork-based";
    }
    if (time_omp < best_time) {
        best_time = time_omp;
        best_method = "OpenMP";
    }
    
    printf("Fastest method: %s (%.2f ms)\n", best_method, best_time);

    if (result_pool) {
        printf("\nResult:\n");
        print_matrix(result_pool);
        if (matrix_count < MAX_MATRICES) {
            matrices[matrix_count++] = result_pool;
            printf("Result saved to memory as '%s'.\n", result_pool->name);
        }
    }

    if (result_fork) free_matrix(result_fork);
    if (result_omp) free_matrix(result_omp);
    if (result_single) free_matrix(result_single);
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

    printf("\n=== SUBTRACTION OPERATION - 3-WAY COMPARISON ===\n");

    // Fork-based
    printf("\n[1] Using FORK (new processes)...\n");
    double start_fork = get_time_ms();
    Matrix *result_fork = subtract_matrices_with_processes(m1, m2);
    double time_fork = get_time_ms() - start_fork;
    
    // OpenMP
    printf("\n[2] Using OpenMP...\n");
    double start_omp = get_time_ms();
    Matrix *result_omp = subtract_matrices_openmp(m1, m2);
    double time_omp = get_time_ms() - start_omp;

    // Single-threaded
    printf("\n[3] Using Single-threaded...\n");
    double start_single = get_time_ms();
    Matrix *result_single = subtract_matrices_single(m1, m2);
    double time_single = get_time_ms() - start_single;

    printf("\n=== PERFORMANCE COMPARISON ===\n");
    printf("Fork-based time:      %.2f ms  (Speedup: %.2fx)\n", time_fork, time_single / time_fork);
    printf("OpenMP time:          %.2f ms  (Speedup: %.2fx)\n", time_omp, time_single / time_omp);
    printf("Single-threaded time: %.2f ms  (Baseline)\n", time_single);

    if (result_fork) {
        printf("\nResult:\n");
        print_matrix(result_fork);
        if (matrix_count < MAX_MATRICES) {
            matrices[matrix_count++] = result_fork;
            printf("Result saved to memory as '%s'.\n", result_fork->name);
        }
    }

    if (result_omp) free_matrix(result_omp);
    if (result_single) free_matrix(result_single);
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

    printf("\n=== MULTIPLICATION OPERATION - 3-WAY COMPARISON ===\n");

    // Fork-based
    printf("\n[1] Using FORK (new processes)...\n");
    double start_fork = get_time_ms();
    Matrix *result_fork = multiply_matrices_with_processes(m1, m2);
    double time_fork = get_time_ms() - start_fork;
    
    // OpenMP
    printf("\n[2] Using OpenMP...\n");
    double start_omp = get_time_ms();
    Matrix *result_omp = multiply_matrices_openmp(m1, m2);
    double time_omp = get_time_ms() - start_omp;

    // Single-threaded
    printf("\n[3] Using Single-threaded...\n");
    double start_single = get_time_ms();
    Matrix *result_single = multiply_matrices_single(m1, m2);
    double time_single = get_time_ms() - start_single;

    printf("\n=== PERFORMANCE COMPARISON ===\n");
    printf("Fork-based time:      %.2f ms  (Speedup: %.2fx)\n", time_fork, time_single / time_fork);
    printf("OpenMP time:          %.2f ms  (Speedup: %.2fx)\n", time_omp, time_single / time_omp);
    printf("Single-threaded time: %.2f ms  (Baseline)\n", time_single);

    if (result_fork) {
        printf("\nResult:\n");
        print_matrix(result_fork);
        if (matrix_count < MAX_MATRICES) {
            matrices[matrix_count++] = result_fork;
            printf("Result saved to memory as '%s'.\n", result_fork->name);
        }
    }

    if (result_omp) free_matrix(result_omp);
    if (result_single) free_matrix(result_single);
}

void determinant_menu() {
    Matrix *m = select_matrix("Select matrix for determinant:");
    if (!m) return;

    if (m->rows != m->cols) {
        printf("Error: Matrix must be square!\n");
        return;
    }

    printf("\n=== DETERMINANT CALCULATION - 3-WAY COMPARISON ===\n");
    printf("Matrix: %s (%dx%d)\n\n", m->name, m->rows, m->cols);

    // Multi-process
    printf("[1] Using Multi-processing (fork)...\n");
    double start_mp = get_time_ms();
    double det_mp = determinant_parallel(m);
    double time_mp = get_time_ms() - start_mp;

    // OpenMP
    printf("[2] Using OpenMP...\n");
    double start_omp = get_time_ms();
    double det_omp = determinant_openmp(m);
    double time_omp = get_time_ms() - start_omp;

    // Single-threaded
    printf("[3] Using Single-threaded...\n");
    double start_single = get_time_ms();
    double det_single = determinant_single(m);
    double time_single = get_time_ms() - start_single;

    printf("\n=== PERFORMANCE COMPARISON ===\n");
    printf("Multi-process time:   %.2f ms  (Speedup: %.2fx)\n", time_mp, time_single / time_mp);
    printf("OpenMP time:          %.2f ms  (Speedup: %.2fx)\n", time_omp, time_single / time_omp);
    printf("Single-threaded time: %.2f ms  (Baseline)\n", time_single);
    
    printf("\n=== VERIFICATION ===\n");
    printf("Multi-process result: %.6f\n", det_mp);
    printf("OpenMP result:        %.6f\n", det_omp);
    printf("Single-thread result: %.6f\n", det_single);
    
    if (fabs(det_mp - det_single) < 1e-6 && fabs(det_omp - det_single) < 1e-6) {
        printf("✅ All methods agree!\n");
    } else {
        printf("⚠️  Warning: Results differ!\n");
    }

    printf("\n=== FINAL RESULT ===\n");
    printf("Determinant: %.6f\n", det_mp);
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

    printf("\n=== 3-WAY COMPARISON ===\n");

    // Multi-processing
    printf("\n[1] Using Multi-processing (fork + pipes)...\n");
    double *eigenvalues_mp = malloc(num_eigen * sizeof(double));
    double **eigenvectors_mp = malloc(num_eigen * sizeof(double*));
    for (int i = 0; i < num_eigen; i++) {
        eigenvectors_mp[i] = malloc(m->rows * sizeof(double));
    }
    
    send_status_via_fifo("EIGEN_MP_START");
    double start_mp = get_time_ms();
    compute_eigen_with_processes(m, num_eigen, eigenvalues_mp, eigenvectors_mp);
    double time_mp = get_time_ms() - start_mp;
    send_status_via_fifo("EIGEN_MP_COMPLETE");

    // OpenMP
    printf("\n[2] Using OpenMP (threading)...\n");
    double start_omp = get_time_ms();
    EigenResult *result_omp = compute_eigen_parallel(m, num_eigen);
    double time_omp = get_time_ms() - start_omp;

    // Single-threaded
    printf("\n[3] Using Single-threaded...\n");
    double start_single = get_time_ms();
    EigenResult *result_single = compute_eigen_single(m, num_eigen);
    double time_single = get_time_ms() - start_single;

    printf("\n=== PERFORMANCE COMPARISON ===\n");
    printf("Multi-process time:   %.2f ms  (Speedup: %.2fx)\n", time_mp, time_single / time_mp);
    printf("OpenMP time:          %.2f ms  (Speedup: %.2fx)\n", time_omp, time_single / time_omp);
    printf("Single-threaded time: %.2f ms  (Baseline)\n", time_single);

    printf("\n=== RESULTS (Multi-Processing) ===\n");
    for (int i = 0; i < num_eigen; i++) {
        printf("\nEigenvalue %d: %.6f\n", i + 1, eigenvalues_mp[i]);
        
        int has_vector = 0;
        for (int j = 0; j < m->rows; j++) {
            if (fabs(eigenvectors_mp[i][j]) > 1e-10) {
                has_vector = 1;
                break;
            }
        }
        
        if (has_vector) {
            printf("Eigenvector %d: [", i + 1);
            for (int j = 0; j < m->rows; j++) {
                printf("%.4f", eigenvectors_mp[i][j]);
                if (j < m->rows - 1) printf(", ");
            }
            printf("]\n");
        }
    }

    // Cleanup
    for (int i = 0; i < num_eigen; i++) {
        free(eigenvectors_mp[i]);
    }
    free(eigenvectors_mp);
    free(eigenvalues_mp);
    
    if (result_omp) free_eigen_result(result_omp);
    if (result_single) free_eigen_result(result_single);
}

int main(int argc, char *argv[]) {
    printf("===========================================\n");
    printf(" Matrix Operations with Multi-Processing\n");
    printf(" Real-Time & Embedded Systems Project\n");
    printf(" ✅ OPTIMIZED VERSION WITH 3-WAY COMPARISON\n");
    printf("===========================================\n\n");

    const char *config_file = (argc > 1) ? argv[1] : "matrix_config.txt";
    init_default_config();
    load_config(config_file);
    
    Config *cfg = get_config();

    setup_signal_handlers();

    printf("\nInitializing system with %d workers...\n", cfg->worker_pool_size);
    init_worker_pool(cfg->worker_pool_size);
    max_idle_time = cfg->max_idle_time;

    if (strlen(cfg->matrix_directory) > 0) {
        printf("\n[AUTO-LOAD] Loading matrices from: %s\n", cfg->matrix_directory);
        read_matrices_from_folder(cfg->matrix_directory);
    } else {
        const char *mat_file = (argc > 2) ? argv[2] : "matrix.txt";
        printf("\n[AUTO-LOAD] Loading matrices from file: %s\n", mat_file);
        load_matrices_from_file(mat_file);
    }

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
