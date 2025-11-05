#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <omp.h>
#include "eigen.h"
#include "matrix.h"
#include "worker_pool.h"

// ===== Helper Functions =====

// Compute vector norm (Euclidean)
double vector_norm(double *v, int n) {
    double sum = 0.0;
    for (int i = 0; i < n; i++) {
        sum += v[i] * v[i];
    }
    return sqrt(sum);
}

// Normalize a vector
void normalize_vector(double *v, int n) {
    double norm = vector_norm(v, n);
    if (norm > 1e-10) {
        for (int i = 0; i < n; i++) {
            v[i] /= norm;
        }
    }
}

// Matrix-vector multiplication: result = M * v
void matrix_vector_multiply(Matrix *m, double *v, double *result) {
    for (int i = 0; i < m->rows; i++) {
        result[i] = 0.0;
        for (int j = 0; j < m->cols; j++) {
            result[i] += m->data[i][j] * v[j];
        }
    }
}

// Parallel matrix-vector multiplication using OpenMP
void matrix_vector_multiply_parallel(Matrix *m, double *v, double *result) {
    #pragma omp parallel for
    for (int i = 0; i < m->rows; i++) {
        result[i] = 0.0;
        for (int j = 0; j < m->cols; j++) {
            result[i] += m->data[i][j] * v[j];
        }
    }
}

// Dot product of two vectors
double dot_product(double *v1, double *v2, int n) {
    double sum = 0.0;
    for (int i = 0; i < n; i++) {
        sum += v1[i] * v2[i];
    }
    return sum;
}

// Copy vector
void copy_vector(double *src, double *dst, int n) {
    for (int i = 0; i < n; i++) {
        dst[i] = src[i];
    }
}

// ===== Power Iteration Method =====
// Finds the dominant (largest magnitude) eigenvalue and eigenvector

int power_iteration_single(Matrix *m, double *eigenvalue, double *eigenvector, 
                           int max_iterations, double tolerance) {
    if (m->rows != m->cols) {
        printf("Error: Matrix must be square\n");
        return 0;
    }
    
    int n = m->rows;
    double *v = malloc(n * sizeof(double));
    double *v_new = malloc(n * sizeof(double));
    
    // Initialize with random vector
    for (int i = 0; i < n; i++) {
        v[i] = 1.0; // Simple initialization
    }
    normalize_vector(v, n);
    
    double lambda = 0.0;
    double lambda_old = 0.0;
    
    for (int iter = 0; iter < max_iterations; iter++) {
        // v_new = A * v
        matrix_vector_multiply(m, v, v_new);
        
        // Compute eigenvalue (Rayleigh quotient)
        lambda = dot_product(v_new, v, n);
        
        // Normalize
        normalize_vector(v_new, n);
        
        // Check convergence
        if (iter > 0 && fabs(lambda - lambda_old) < tolerance) {
            copy_vector(v_new, eigenvector, n);
            *eigenvalue = lambda;
            free(v);
            free(v_new);
            return 1; // Success
        }
        
        lambda_old = lambda;
        copy_vector(v_new, v, n);
    }
    
    // Max iterations reached
    copy_vector(v, eigenvector, n);
    *eigenvalue = lambda;
    free(v);
    free(v_new);
    return 1;
}

int power_iteration_parallel(Matrix *m, double *eigenvalue, double *eigenvector,
                             int max_iterations, double tolerance) {
    if (m->rows != m->cols) {
        printf("Error: Matrix must be square\n");
        return 0;
    }
    
    int n = m->rows;
    double *v = malloc(n * sizeof(double));
    double *v_new = malloc(n * sizeof(double));
    
    // Initialize with random vector
    for (int i = 0; i < n; i++) {
        v[i] = 1.0;
    }
    normalize_vector(v, n);
    
    double lambda = 0.0;
    double lambda_old = 0.0;
    
    for (int iter = 0; iter < max_iterations; iter++) {
        // v_new = A * v (parallel)
        matrix_vector_multiply_parallel(m, v, v_new);
        
        // Compute eigenvalue (Rayleigh quotient)
        lambda = dot_product(v_new, v, n);
        
        // Normalize
        normalize_vector(v_new, n);
        
        // Check convergence
        if (iter > 0 && fabs(lambda - lambda_old) < tolerance) {
            copy_vector(v_new, eigenvector, n);
            *eigenvalue = lambda;
            free(v);
            free(v_new);
            return 1;
        }
        
        lambda_old = lambda;
        copy_vector(v_new, v, n);
    }
    
    copy_vector(v, eigenvector, n);
    *eigenvalue = lambda;
    free(v);
    free(v_new);
    return 1;
}

// ===== QR Algorithm for Multiple Eigenvalues =====
// Simplified QR iteration (finds all eigenvalues but not eigenvectors efficiently)

void qr_decomposition_simple(Matrix *m, Matrix *Q, Matrix *R) {
    int n = m->rows;
    
    // Initialize Q as identity, R as copy of m
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            Q->data[i][j] = (i == j) ? 1.0 : 0.0;
            R->data[i][j] = m->data[i][j];
        }
    }
    
    // Gram-Schmidt process
    for (int j = 0; j < n; j++) {
        double *col = malloc(n * sizeof(double));
        
        // Get column j from original matrix
        for (int i = 0; i < n; i++) {
            col[i] = m->data[i][j];
        }
        
        // Orthogonalize against previous columns
        for (int k = 0; k < j; k++) {
            double dot = 0.0;
            for (int i = 0; i < n; i++) {
                dot += m->data[i][j] * Q->data[i][k];
            }
            for (int i = 0; i < n; i++) {
                col[i] -= dot * Q->data[i][k];
            }
        }
        
        // Normalize
        double norm = vector_norm(col, n);
        if (norm > 1e-10) {
            for (int i = 0; i < n; i++) {
                Q->data[i][j] = col[i] / norm;
            }
        }
        
        free(col);
    }
    
    // Compute R = Q^T * A
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            R->data[i][j] = 0.0;
            for (int k = 0; k < n; k++) {
                R->data[i][j] += Q->data[k][i] * m->data[k][j];
            }
        }
    }
}

int qr_algorithm_eigenvalues(Matrix *m, double *eigenvalues, int max_iterations, double tolerance) {
    if (m->rows != m->cols) {
        printf("Error: Matrix must be square\n");
        return 0;
    }
    
    int n = m->rows;
    
    // Create working copy of matrix
    Matrix *A = create_matrix(n, n, "temp_A");
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            A->data[i][j] = m->data[i][j];
        }
    }
    
    Matrix *Q = create_matrix(n, n, "temp_Q");
    Matrix *R = create_matrix(n, n, "temp_R");
    
    for (int iter = 0; iter < max_iterations; iter++) {
        // QR decomposition
        qr_decomposition_simple(A, Q, R);
        
        // A = R * Q
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                double sum = 0.0;
                for (int k = 0; k < n; k++) {
                    sum += R->data[i][k] * Q->data[k][j];
                }
                A->data[i][j] = sum;
            }
        }
        
        // Check for convergence (off-diagonal elements should be small)
        double off_diag_sum = 0.0;
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                if (i != j) {
                    off_diag_sum += fabs(A->data[i][j]);
                }
            }
        }
        
        if (off_diag_sum < tolerance) {
            break;
        }
    }
    
    // Extract eigenvalues from diagonal
    for (int i = 0; i < n; i++) {
        eigenvalues[i] = A->data[i][i];
    }
    
    free_matrix(A);
    free_matrix(Q);
    free_matrix(R);
    
    return 1;
}

// ===== Find All Eigenvalues and Eigenvectors =====

EigenResult* compute_eigen_single(Matrix *m, int num_eigenvalues) {
    if (m->rows != m->cols) {
        printf("Error: Matrix must be square\n");
        return NULL;
    }
    
    int n = m->rows;
    if (num_eigenvalues > n) {
        num_eigenvalues = n;
    }
    
    EigenResult *result = malloc(sizeof(EigenResult));
    result->num_eigenvalues = num_eigenvalues;
    result->eigenvalues = malloc(num_eigenvalues * sizeof(double));
    result->eigenvectors = malloc(num_eigenvalues * sizeof(double*));
    for (int i = 0; i < num_eigenvalues; i++) {
        result->eigenvectors[i] = malloc(n * sizeof(double));
    }
    
    double start_time = get_time_ms();
    
    // Use power iteration for dominant eigenvalue
    if (num_eigenvalues >= 1) {
        power_iteration_single(m, &result->eigenvalues[0], 
                              result->eigenvectors[0], 1000, 1e-6);
    }
    
    // For additional eigenvalues, use QR algorithm
    if (num_eigenvalues > 1) {
        double *all_eigenvalues = malloc(n * sizeof(double));
        qr_algorithm_eigenvalues(m, all_eigenvalues, 1000, 1e-6);
        
        // Copy the requested number of eigenvalues
        for (int i = 1; i < num_eigenvalues; i++) {
            result->eigenvalues[i] = all_eigenvalues[i];
            
            // For simplicity, set eigenvectors to zero (full eigenvector computation is complex)
            // In a complete implementation, you'd use inverse iteration here
            for (int j = 0; j < n; j++) {
                result->eigenvectors[i][j] = 0.0;
            }
        }
        
        free(all_eigenvalues);
    }
    
    double end_time = get_time_ms();
    printf("[TIMING] Single-threaded eigen computation: %.2f ms\n", end_time - start_time);
    
    return result;
}

EigenResult* compute_eigen_parallel(Matrix *m, int num_eigenvalues) {
    if (m->rows != m->cols) {
        printf("Error: Matrix must be square\n");
        return NULL;
    }
    
    int n = m->rows;
    if (num_eigenvalues > n) {
        num_eigenvalues = n;
    }
    
    EigenResult *result = malloc(sizeof(EigenResult));
    result->num_eigenvalues = num_eigenvalues;
    result->eigenvalues = malloc(num_eigenvalues * sizeof(double));
    result->eigenvectors = malloc(num_eigenvalues * sizeof(double*));
    for (int i = 0; i < num_eigenvalues; i++) {
        result->eigenvectors[i] = malloc(n * sizeof(double));
    }
    
    double start_time = get_time_ms();
    
    // Use parallel power iteration for dominant eigenvalue
    if (num_eigenvalues >= 1) {
        power_iteration_parallel(m, &result->eigenvalues[0],
                                result->eigenvectors[0], 1000, 1e-6);
    }
    
    // For additional eigenvalues, use QR algorithm
    if (num_eigenvalues > 1) {
        double *all_eigenvalues = malloc(n * sizeof(double));
        qr_algorithm_eigenvalues(m, all_eigenvalues, 1000, 1e-6);
        
        for (int i = 1; i < num_eigenvalues; i++) {
            result->eigenvalues[i] = all_eigenvalues[i];
            for (int j = 0; j < n; j++) {
                result->eigenvectors[i][j] = 0.0;
            }
        }
        
        free(all_eigenvalues);
    }
    
    double end_time = get_time_ms();
    printf("[TIMING] Parallel eigen computation (OpenMP): %.2f ms\n", end_time - start_time);
    
    return result;
}

void free_eigen_result(EigenResult *result) {
    if (!result) return;
    
    for (int i = 0; i < result->num_eigenvalues; i++) {
        free(result->eigenvectors[i]);
    }
    free(result->eigenvectors);
    free(result->eigenvalues);
    free(result);
}

void print_eigen_result(EigenResult *result, int matrix_size) {
    if (!result) return;
    
    printf("\n=== EIGENVALUE RESULTS ===\n");
    for (int i = 0; i < result->num_eigenvalues; i++) {
        printf("\nEigenvalue %d: %.6f\n", i + 1, result->eigenvalues[i]);
        
        // Check if eigenvector is computed (non-zero)
        int has_eigenvector = 0;
        for (int j = 0; j < matrix_size; j++) {
            if (fabs(result->eigenvectors[i][j]) > 1e-10) {
                has_eigenvector = 1;
                break;
            }
        }
        
        if (has_eigenvector) {
            printf("Eigenvector %d: [", i + 1);
            for (int j = 0; j < matrix_size; j++) {
                printf("%.4f", result->eigenvectors[i][j]);
                if (j < matrix_size - 1) printf(", ");
            }
            printf("]\n");
        } else {
            printf("Eigenvector %d: [Not computed - use power iteration for individual vectors]\n", i + 1);
        }
    }
    printf("\n");
}
