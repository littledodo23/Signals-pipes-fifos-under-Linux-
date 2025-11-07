#ifndef EIGEN_H
#define EIGEN_H

#include "matrix.h"
#include "worker_pool.h"

// ===== Eigen Result Structure =====
typedef struct {
    int num_eigenvalues;
    double *eigenvalues;
    double **eigenvectors;
} EigenResult;

// ===== Power Iteration (for dominant eigenvalue/eigenvector) =====
int power_iteration_single(Matrix *m, double *eigenvalue, double *eigenvector,
                           int max_iterations, double tolerance);

int power_iteration_parallel(Matrix *m, double *eigenvalue, double *eigenvector,
                             int max_iterations, double tolerance);

// ===== QR Algorithm (for all eigenvalues) =====
int qr_algorithm_eigenvalues(Matrix *m, double *eigenvalues, 
                             int max_iterations, double tolerance);

// ===== Complete Eigen Computation =====
EigenResult* compute_eigen_single(Matrix *m, int num_eigenvalues);
EigenResult* compute_eigen_parallel(Matrix *m, int num_eigenvalues);

// ===== Helper Functions =====
void free_eigen_result(EigenResult *result);
void print_eigen_result(EigenResult *result, int matrix_size);

// Vector operations
double vector_norm(double *v, int n);
void normalize_vector(double *v, int n);
void matrix_vector_multiply(Matrix *m, double *v, double *result);  // ✅ FIXED: douable -> double
void matrix_vector_multiply_parallel(Matrix *m, double *v, double *result);
double dot_product(double *v1, double *v2, int n);
void copy_vector(double *src, double *dst, int n);

#endif
