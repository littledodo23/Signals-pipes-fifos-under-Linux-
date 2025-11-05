#ifndef WORKER_POOL_H
#define WORKER_POOL_H

#include <sys/types.h>
#include <time.h>
#include "matrix.h"

// ===== Worker Process Structure =====
typedef struct {
    pid_t pid;                  // Process ID
    int input_pipe[2];          // Parent writes, child reads
    int output_pipe[2];         // Child writes, parent reads
    time_t last_used;           // For aging mechanism
    int available;              // 1 if available, 0 if busy
    int alive;                  // 1 if process is alive
} Worker;

// ===== Operation Types =====
typedef enum {
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY_ELEMENT,
    OP_DETERMINANT_2X2,
    OP_MATRIX_VECTOR_MULTIPLY,
    OP_EXIT
} OperationType;

// ===== Message Structure for IPC =====
#define MAX_VECTOR_SIZE 2000
#define MAX_MATRIX_SIZE 100
typedef struct {
    OperationType op_type;
    double operand1;
    double operand2;
    double result;
    int row_size;
    int matrix_size;
    double row_data[MAX_VECTOR_SIZE];
    double col_data[MAX_VECTOR_SIZE];
    double matrix_data[MAX_MATRIX_SIZE][MAX_MATRIX_SIZE];
    double vector_data[MAX_VECTOR_SIZE];
} WorkMessage;

// ===== Global Pool =====
#define MAX_WORKERS 100
extern Worker *worker_pool;
extern int pool_size;
extern int max_idle_time;

// ===== Function Prototypes =====

// Pool management
void init_worker_pool(int size);
void cleanup_worker_pool();
Worker* get_available_worker();
void release_worker(Worker *w);
void age_workers();

// Worker process functions
void worker_process_loop(int input_fd, int output_fd);

// Process-based matrix operations (PRIMARY METHODS - using pipes/IPC)
Matrix* add_matrices_with_processes(Matrix *m1, Matrix *m2);
Matrix* subtract_matrices_with_processes(Matrix *m1, Matrix *m2);
Matrix* multiply_matrices_with_processes(Matrix *m1, Matrix *m2);
double determinant_with_processes(Matrix *m);
void compute_eigen_with_processes(Matrix *m, int num_eigenvalues, double *eigenvalues, double **eigenvectors);

// Single-threaded versions for comparison
Matrix* add_matrices_single(Matrix *m1, Matrix *m2);
Matrix* subtract_matrices_single(Matrix *m1, Matrix *m2);
Matrix* multiply_matrices_single(Matrix *m1, Matrix *m2);
double determinant_single(Matrix *m);
void compute_eigen_single(Matrix *m, int num_eigenvalues, double *eigenvalues, double **eigenvectors);

// Performance timing
double get_time_ms();

// Signal handlers
void setup_signal_handlers();
void sigusr1_handler(int signo);
void sigchld_handler(int signo);

#endif
