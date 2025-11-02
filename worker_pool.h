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
    OP_DETERMINANT,
    OP_EXIT
} OperationType;

// ===== Message Structure for IPC =====
typedef struct {
    OperationType op_type;
    double operand1;
    double operand2;
    double result;
    int row_size;               // For multiplication
    double row_data[100];       // Row vector
    double col_data[100];       // Column vector
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

// Matrix operations with multiprocessing
Matrix* add_matrices_parallel(Matrix *m1, Matrix *m2);
Matrix* subtract_matrices_parallel(Matrix *m1, Matrix *m2);
Matrix* multiply_matrices_parallel(Matrix *m1, Matrix *m2);
double determinant_parallel(Matrix *m);

// Single-threaded versions for comparison
Matrix* add_matrices_single(Matrix *m1, Matrix *m2);
Matrix* subtract_matrices_single(Matrix *m1, Matrix *m2);
Matrix* multiply_matrices_single(Matrix *m1, Matrix *m2);
double determinant_single(Matrix *m);

// Performance timing
double get_time_ms();
void benchmark_operation(const char *op_name, Matrix *m1, Matrix *m2);

// Signal handlers
void setup_signal_handlers();
void sigusr1_handler(int signo);
void sigchld_handler(int signo);

#endif
