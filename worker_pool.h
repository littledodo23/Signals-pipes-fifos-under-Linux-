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

// ✅ FIFO Status Message
typedef struct {
    char status[64];
    int worker_count;
    int active_workers;
    double timestamp;
} StatusMessage;

// ===== Global Pool =====
#define MAX_WORKERS 100
extern Worker *worker_pool;
extern int pool_size;
extern int max_idle_time;

// ===== Worker Pool Management =====
void init_worker_pool(int size);
void cleanup_worker_pool(void);
Worker* get_available_worker(void);
void release_worker(Worker *w);
void age_workers(void);
void worker_process_loop(int input_fd, int output_fd);

// ===== Matrix Operations - Fork-based (New Processes) =====
Matrix* add_matrices_with_processes(Matrix *m1, Matrix *m2);
Matrix* subtract_matrices_with_processes(Matrix *m1, Matrix *m2);
Matrix* multiply_matrices_with_processes(Matrix *m1, Matrix *m2);
double determinant_with_processes(Matrix *m);

// ===== ✅ ADDED: Matrix Operations - Worker Pool (Persistent Processes) =====
Matrix* add_matrices_with_pool(Matrix *m1, Matrix *m2);

// ===== ✅ ADDED: Matrix Operations - OpenMP (Threading) =====
Matrix* add_matrices_openmp(Matrix *m1, Matrix *m2);
Matrix* subtract_matrices_openmp(Matrix *m1, Matrix *m2);
Matrix* multiply_matrices_openmp(Matrix *m1, Matrix *m2);
double determinant_openmp(Matrix *m);

// ===== Single-threaded Versions (Baseline) =====
Matrix* add_matrices_single(Matrix *m1, Matrix *m2);
Matrix* subtract_matrices_single(Matrix *m1, Matrix *m2);
Matrix* multiply_matrices_single(Matrix *m1, Matrix *m2);
double determinant_single(Matrix *m);

// ===== Eigenvalue Computation =====
void compute_eigen_with_processes(Matrix *m, int num_eigenvalues, double *eigenvalues, double **eigenvectors);

// ===== Helper Functions =====
double determinant_parallel(Matrix *m);
double get_time_ms(void);
void setup_signal_handlers(void);
void sigusr1_handler(int signo);
void sigchld_handler(int signo);

// ===== FIFO Functions =====
void init_status_fifo(void);
void send_status_via_fifo(const char *status_msg);
void cleanup_status_fifo(void);
void monitor_status_fifo_background(void);

#endif
