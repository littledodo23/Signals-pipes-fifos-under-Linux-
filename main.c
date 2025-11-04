#include <stdio.h>
#include <stdlib.h>
#include "matrix.h"
#include "worker_pool.h"

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

Matrix* select_matrix(const char *prompt) {
    if (matrix_count == 0) {
        printf("No matrices in memory!\n");
        return NULL;
    }
    
    printf("\n%s\n", prompt);
    printf("Available matrices:\n");
    for (int i = 0; i < matrix_count; i++) {
        printf("%d. %s (%dx%d)\n", i + 1, matrices[i]->name,
               matrices[i]->rows, matrices[i]->cols);
    }
    
    int choice;
    printf("Enter choice: ");
    scanf("%d", &choice);
    
    if (choice < 1 || choice > matrix_count) {
        printf("Invalid choice!\n");
        return NULL;
    }
    
    return matrices[choice - 1];
}

void add_matrices_menu() {
    Matrix *m1 = select_matrix("Select first matrix to add:");
    if (!m1) return;
    
    Matrix *m2 = select_matrix("Select second matrix to add:");
    if (!m2) return;
    
    printf("\n--- Running PARALLEL addition ---\n");
    Matrix *result_parallel = add_matrices_parallel(m1, m2);
    
    printf("\n--- Running SINGLE-THREADED addition ---\n");
    Matrix *result_single = add_matrices_single(m1, m2);
    
    if (result_parallel) {
        printf("\nResult (parallel):\n");
        print_matrix(result_parallel);
        
        if (matrix_count < MAX_MATRICES) {
            matrices[matrix_count++] = result_parallel;
            printf("Result saved to memory as '%s'\n", result_parallel->name);
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
    
    printf("\n--- Running PARALLEL subtraction ---\n");
    Matrix *result_parallel = subtract_matrices_parallel(m1, m2);
    
    printf("\n--- Running SINGLE-THREADED subtraction ---\n");
    Matrix *result_single = subtract_matrices_single(m1, m2);
    
    if (result_parallel) {
        printf("\nResult (parallel):\n");
        print_matrix(result_parallel);
        
        if (matrix_count < MAX_MATRICES) {
            matrices[matrix_count++] = result_parallel;
            printf("Result saved to memory as '%s'\n", result_parallel->name);
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
    
    printf("\n--- Running PARALLEL multiplication ---\n");
    Matrix *result_parallel = multiply_matrices_parallel(m1, m2);
    
    printf("\n--- Running SINGLE-THREADED multiplication ---\n");
    Matrix *result_single = multiply_matrices_single(m1, m2);
    
    if (result_parallel) {
        printf("\nResult (parallel):\n");
        print_matrix(result_parallel);
        
        if (matrix_count < MAX_MATRICES) {
            matrices[matrix_count++] = result_parallel;
            printf("Result saved to memory as '%s'\n", result_parallel->name);
        }
    }
    
    if (result_single) {
        free_matrix(result_single);
    }
}

void eigen_menu() {
    Matrix *m = select_matrix("Select matrix for eigenvalue/vector computation:");
    if (!m) return;

    int n = m->rows;
    if (m->rows != m->cols) {
        printf("Error: Matrix must be square!\n");
        return;
    }

    double eigenvector[n];
    double eigenvalue;

    eigen_parallel(m, &eigenvalue, eigenvector);

    printf("\nLargest eigenvalue: %.6lf\n", eigenvalue);
    printf("Corresponding eigenvector:\n");
    for (int i = 0; i < n; i++)
        printf("%8.4lf\n", eigenvector[i]);
}

void determinant_menu() {
    Matrix *m = select_matrix("Select matrix for determinant:");
    if (!m) return;
    
    if (m->rows != m->cols) {
        printf("Error: Matrix must be square!\n");
        return;
    }
    
    printf("\nCalculating determinant...\n");
    double det = determinant_parallel(m);
    printf("\nDeterminant of %s = %.4f\n", m->name, det);
}

void benchmark_menu() {
    printf("\n=== BENCHMARK MODE ===\n");
    printf("This will compare parallel vs single-threaded performance\n\n");
    
    Matrix *m1 = select_matrix("Select first matrix:");
    if (!m1) return;
    
    Matrix *m2 = select_matrix("Select second matrix:");
    if (!m2) return;
    
    if (m1->rows != m2->rows || m1->cols != m2->cols) {
        printf("Matrices must have same dimensions for this benchmark\n");
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
    
    printf("╔════════════════════════════════════════════╗\n");
    printf("║  Matrix Operations with Multi-Processing  ║\n");
    printf("║  Real-Time & Embedded Systems Project     ║\n");
    printf("╚════════════════════════════════════════════╝\n\n");
    
    // Setup signal handlers
    setup_signal_handlers();
    
    // Initialize worker pool
    printf("Enter worker pool size (recommended: 4-8): ");
    int pool_sz;
    scanf("%d", &pool_sz);
    
    if (pool_sz < 1) pool_sz = 4;
    if (pool_sz > MAX_WORKERS) pool_sz = MAX_WORKERS;
    
    init_worker_pool(pool_sz);
    
    while (1) {
        show_menu();
        printf("Enter your choice: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1:  
                enter_matrix(); 
                break;
                
            case 2:  
                display_matrix(); 
                break;
                
            case 3:  
                delete_matrix(); 
                break;
                
            case 4:  
                modify_matrix(); 
                break;
                
            case 5:  
                read_matrix_from_file_option(); 
                break;
                
            case 6:  
                read_matrices_from_folder_option(); 
                break;
                
            case 7:  
                save_matrix_to_file_option(); 
                break;
                
            case 8:  
                save_all_matrices_to_folder_option(); 
                break;
                
            case 9:  
                display_all_matrices(); 
                break;
                
            case 10:
                add_matrices_menu();
                break;
                
            case 11:
                subtract_matrices_menu();
                break;
                
            case 12:
                multiply_matrices_menu();
                break;
                
            case 13:
                determinant_menu();
                break;
                
            case 14:
                benchmark_menu();
                break;

                
            case 15:
                printf("\n Cleaning up worker pool...\n");
                cleanup_worker_pool();
                
                printf("  Freeing matrices...\n");
                for (int i = 0; i < matrix_count; i++) {
                    free_matrix(matrices[i]);
                }
                
                printf(" Goodbye!\n");
                exit(0);
                
            default:
                printf("  Invalid choice! Please try again.\n");
        }
        
        // Periodically age out idle workers
        age_workers();
    }
    
    return 0;
}
