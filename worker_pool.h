#ifndef WORKER_POOL_H
#define WORKER_POOL_H

#include <sys/types.h>
#include <time.h>
#include "matrix.h"

// ===== Worker Process Structure =====
typedef struct {
    pid_t pid;
    int input_pipe[2];
    int output_pipe[2];
    time_t last_used;
    int available;
    int alive;
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

// ===== Message Structure =====
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

// ===== Functions =====
void init_worker_pool(int size);
void cleanup_worker_pool(void);
Worker* get_available_worker(void);
void release_worker(Worker *w);
void age_workers(void);

void worker_process_loop(int input_fd, int output_fd);

// Matrix operations using multiprocessing
Matrix* add_matrices_with_processes(Matrix *m1, Matrix *m2);
Matrix* subtract_matrices_with_processes(Matrix *m1, Matrix *m2);
Matrix* multiply_matrices_with_processes(Matrix *m1, Matrix *m2);
double determinant_with_processes(Matrix *m);

// Single-threaded versions
Matrix* add_matrices_single(Matrix *m1, Matrix *m2);
Matrix* subtract_matrices_single(Matrix *m1, Matrix *m2);
Matrix* multiply_matrices_single(Matrix *m1, Matrix *m2);
double determinant_single(Matrix *m);

// Timing + signal handlers
double get_time_ms(void);
void setup_signal_handlers(void);
void sigusr1_handler(int signo);
void sigchld_handler(int signo);

void compute_eigen_with_processes(Matrix *m, int num_eigenvalues, double *eigenvalues, double **eigenvectors);

#endif
