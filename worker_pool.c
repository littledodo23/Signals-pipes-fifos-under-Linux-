#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <omp.h>
#include "worker_pool.h"
#include "matrix.h"

// ===== Global Variables =====
Worker *worker_pool = NULL;
int pool_size = 0;
int max_idle_time = 60; // seconds

// ===== Signal Handlers =====

void sigusr1_handler(int signo) {
    // Worker notification signal
    (void)signo; // Suppress unused warning
}

void sigchld_handler(int signo) {
    // Handle child process termination
    (void)signo;
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        // Mark worker as dead
        for (int i = 0; i < pool_size; i++) {
            if (worker_pool[i].pid == pid) {
                worker_pool[i].alive = 0;
                worker_pool[i].available = 0;
                printf("[INFO] Worker %d (PID %d) terminated\n", i, pid);
            }
        }
    }
}

void setup_signal_handlers() {
    signal(SIGUSR1, sigusr1_handler);
    signal(SIGCHLD, sigchld_handler);
    signal(SIGPIPE, SIG_IGN); // Ignore broken pipe
}

// ===== Timing Functions =====

double get_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000.0) + (tv.tv_usec / 1000.0);
}

// ===== Worker Process =====

void worker_process_loop(int input_fd, int output_fd) {
    WorkMessage msg;
    
    while (1) {
        // Read operation from parent
        ssize_t n = read(input_fd, &msg, sizeof(WorkMessage));
        if (n <= 0) {
            break; // Pipe closed or error
        }
        
        // Perform operation
        switch (msg.op_type) {
            case OP_ADD:
                msg.result = msg.operand1 + msg.operand2;
                break;
                
            case OP_SUBTRACT:
                msg.result = msg.operand1 - msg.operand2;
                break;
                
            case OP_MULTIPLY_ELEMENT: {
                // Dot product of row and column
                msg.result = 0.0;
                for (int i = 0; i < msg.row_size; i++) {
                    msg.result += msg.row_data[i] * msg.col_data[i];
                }
                break;
            }
                
            case OP_EXIT:
                close(input_fd);
                close(output_fd);
                exit(0);
                
            default:
                msg.result = 0.0;
        }
        
        // Send result back to parent
        write(output_fd, &msg, sizeof(WorkMessage));
    }
    
    close(input_fd);
    close(output_fd);
    exit(0);
}

// ===== Worker Pool Management =====

void init_worker_pool(int size) {
    pool_size = size;
    worker_pool = malloc(size * sizeof(Worker));
    
    printf("[INFO] Initializing worker pool with %d workers...\n", size);
    
    for (int i = 0; i < size; i++) {
        // Create pipes
        if (pipe(worker_pool[i].input_pipe) == -1 ||
            pipe(worker_pool[i].output_pipe) == -1) {
            perror("pipe");
            exit(1);
        }
        
        // Fork worker process
        pid_t pid = fork();
        
        if (pid < 0) {
            perror("fork");
            exit(1);
        }
        
        if (pid == 0) {
            // Child process
            close(worker_pool[i].input_pipe[1]);  // Close write end of input
            close(worker_pool[i].output_pipe[0]); // Close read end of output
            
            worker_process_loop(worker_pool[i].input_pipe[0],
                              worker_pool[i].output_pipe[1]);
            exit(0);
        }
        
        // Parent process
        close(worker_pool[i].input_pipe[0]);  // Close read end of input
        close(worker_pool[i].output_pipe[1]); // Close write end of output
        
        worker_pool[i].pid = pid;
        worker_pool[i].available = 1;
        worker_pool[i].alive = 1;
        worker_pool[i].last_used = time(NULL);
    }
    
    printf("[INFO] Worker pool initialized successfully\n");
}

Worker* get_available_worker() {
    for (int i = 0; i < pool_size; i++) {
        if (worker_pool[i].available && worker_pool[i].alive) {
            worker_pool[i].available = 0;
            worker_pool[i].last_used = time(NULL);
            return &worker_pool[i];
        }
    }
    return NULL;
}

void release_worker(Worker *w) {
    if (w) {
        w->available = 1;
        w->last_used = time(NULL);
    }
}

void age_workers() {
    time_t now = time(NULL);
    for (int i = 0; i < pool_size; i++) {
        if (worker_pool[i].alive && worker_pool[i].available) {
            if (now - worker_pool[i].last_used > max_idle_time) {
                WorkMessage msg = {.op_type = OP_EXIT};
                write(worker_pool[i].input_pipe[1], &msg, sizeof(WorkMessage));
                worker_pool[i].alive = 0;
                printf("[INFO] Aged out worker %d (idle for %ld seconds)\n",
                       i, now - worker_pool[i].last_used);
            }
        }
    }
}

void cleanup_worker_pool() {
    if (!worker_pool) return;
    
    printf("[INFO] Cleaning up worker pool...\n");
    
    for (int i = 0; i < pool_size; i++) {
        if (worker_pool[i].alive) {
            WorkMessage msg = {.op_type = OP_EXIT};
            write(worker_pool[i].input_pipe[1], &msg, sizeof(WorkMessage));
            close(worker_pool[i].input_pipe[1]);
            close(worker_pool[i].output_pipe[0]);
            
            // Wait for child to terminate
            waitpid(worker_pool[i].pid, NULL, 0);
        }
    }
    
    free(worker_pool);
    worker_pool = NULL;
    printf("[INFO] Worker pool cleaned up\n");
}

// ===== Matrix Addition (Parallel) =====

Matrix* add_matrices_parallel(Matrix *m1, Matrix *m2) {
    if (m1->rows != m2->rows || m1->cols != m2->cols) {
        printf("Error: Matrices must have same dimensions for addition\n");
        return NULL;
    }
    
    char result_name[50];
    snprintf(result_name, sizeof(result_name), "%s_plus_%s", m1->name, m2->name);
    Matrix *result = create_matrix(m1->rows, m1->cols, result_name);
    
    int total_elements = m1->rows * m1->cols;
    printf("[INFO] Adding matrices using %d workers for %d elements\n",
           pool_size, total_elements);
    
    double start_time = get_time_ms();
    
    // Send work to all workers
    int elements_processed = 0;
    
    for (int i = 0; i < m1->rows; i++) {
        for (int j = 0; j < m1->cols; j++) {
            Worker *w = get_available_worker();
            while (!w) {
                usleep(100); // Wait for available worker
                w = get_available_worker();
            }
            
            WorkMessage msg = {
                .op_type = OP_ADD,
                .operand1 = m1->data[i][j],
                .operand2 = m2->data[i][j]
            };
            
            write(w->input_pipe[1], &msg, sizeof(WorkMessage));
            read(w->output_pipe[0], &msg, sizeof(WorkMessage));
            
            result->data[i][j] = msg.result;
            release_worker(w);
            elements_processed++;
        }
    }
    
    double end_time = get_time_ms();
    printf("[TIMING] Parallel addition: %.2f ms\n", end_time - start_time);
    
    return result;
}

// ===== Matrix Subtraction (Parallel) =====

Matrix* subtract_matrices_parallel(Matrix *m1, Matrix *m2) {
    if (m1->rows != m2->rows || m1->cols != m2->cols) {
        printf("Error: Matrices must have same dimensions for subtraction\n");
        return NULL;
    }
    
    char result_name[50];
    snprintf(result_name, sizeof(result_name), "%s_minus_%s", m1->name, m2->name);
    Matrix *result = create_matrix(m1->rows, m1->cols, result_name);
    
    double start_time = get_time_ms();
    
    for (int i = 0; i < m1->rows; i++) {
        for (int j = 0; j < m1->cols; j++) {
            Worker *w = get_available_worker();
            while (!w) {
                usleep(100);
                w = get_available_worker();
            }
            
            WorkMessage msg = {
                .op_type = OP_SUBTRACT,
                .operand1 = m1->data[i][j],
                .operand2 = m2->data[i][j]
            };
            
            write(w->input_pipe[1], &msg, sizeof(WorkMessage));
            read(w->output_pipe[0], &msg, sizeof(WorkMessage));
            
            result->data[i][j] = msg.result;
            release_worker(w);
        }
    }
    
    double end_time = get_time_ms();
    printf("[TIMING] Parallel subtraction: %.2f ms\n", end_time - start_time);
    
    return result;
}

// ===== Matrix Multiplication (Parallel) =====

Matrix* multiply_matrices_parallel(Matrix *m1, Matrix *m2) {
    if (m1->cols != m2->rows) {
        printf("Error: Invalid dimensions for multiplication\n");
        return NULL;
    }
    
    char result_name[50];
    snprintf(result_name, sizeof(result_name), "%s_times_%s", m1->name, m2->name);
    Matrix *result = create_matrix(m1->rows, m2->cols, result_name);
    
    printf("[INFO] Multiplying matrices: %d workers for %d elements\n",
           pool_size, m1->rows * m2->cols);
    
    double start_time = get_time_ms();
    
    for (int i = 0; i < m1->rows; i++) {
        for (int j = 0; j < m2->cols; j++) {
            Worker *w = get_available_worker();
            while (!w) {
                usleep(100);
                w = get_available_worker();
            }
            
            WorkMessage msg = {
                .op_type = OP_MULTIPLY_ELEMENT,
                .row_size = m1->cols
            };
            
            // Copy row from m1
            for (int k = 0; k < m1->cols; k++) {
                msg.row_data[k] = m1->data[i][k];
            }
            
            // Copy column from m2
            for (int k = 0; k < m2->rows; k++) {
                msg.col_data[k] = m2->data[k][j];
            }
            
            write(w->input_pipe[1], &msg, sizeof(WorkMessage));
            read(w->output_pipe[0], &msg, sizeof(WorkMessage));
            
            result->data[i][j] = msg.result;
            release_worker(w);
        }
    }
    
    double end_time = get_time_ms();
    printf("[TIMING] Parallel multiplication: %.2f ms\n", end_time - start_time);
    
    return result;
}

// ===== Single-threaded versions for comparison =====

Matrix* add_matrices_single(Matrix *m1, Matrix *m2) {
    if (m1->rows != m2->rows || m1->cols != m2->cols) {
        return NULL;
    }
    
    char result_name[50];
    snprintf(result_name, sizeof(result_name), "%s_plus_%s_single", m1->name, m2->name);
    Matrix *result = create_matrix(m1->rows, m1->cols, result_name);
    
    double start_time = get_time_ms();
    
    for (int i = 0; i < m1->rows; i++) {
        for (int j = 0; j < m1->cols; j++) {
            result->data[i][j] = m1->data[i][j] + m2->data[i][j];
        }
    }
    
    double end_time = get_time_ms();
    printf("[TIMING] Single-threaded addition: %.2f ms\n", end_time - start_time);
    
    return result;
}

Matrix* subtract_matrices_single(Matrix *m1, Matrix *m2) {
    if (m1->rows != m2->rows || m1->cols != m2->cols) {
        return NULL;
    }
    
    char result_name[50];
    snprintf(result_name, sizeof(result_name), "%s_minus_%s_single", m1->name, m2->name);
    Matrix *result = create_matrix(m1->rows, m1->cols, result_name);
    
    double start_time = get_time_ms();
    
    for (int i = 0; i < m1->rows; i++) {
        for (int j = 0; j < m1->cols; j++) {
            result->data[i][j] = m1->data[i][j] - m2->data[i][j];
        }
    }
    
    double end_time = get_time_ms();
    printf("[TIMING] Single-threaded subtraction: %.2f ms\n", end_time - start_time);
    
    return result;
}

Matrix* multiply_matrices_single(Matrix *m1, Matrix *m2) {
    if (m1->cols != m2->rows) {
        return NULL;
    }
    
    char result_name[50];
    snprintf(result_name, sizeof(result_name), "%s_times_%s_single", m1->name, m2->name);
    Matrix *result = create_matrix(m1->rows, m2->cols, result_name);
    
    double start_time = get_time_ms();
    
    for (int i = 0; i < m1->rows; i++) {
        for (int j = 0; j < m2->cols; j++) {
            result->data[i][j] = 0.0;
            for (int k = 0; k < m1->cols; k++) {
                result->data[i][j] += m1->data[i][k] * m2->data[k][j];
            }
        }
    }
    
    double end_time = get_time_ms();
    printf("[TIMING] Single-threaded multiplication: %.2f ms\n", end_time - start_time);
    
    return result;
}

// ===== Determinant (Simple recursive - you can parallelize this more) =====

double determinant_single(Matrix *m) {
    if (m->rows != m->cols) {
        printf("Error: Matrix must be square\n");
        return 0.0;
    }
    
    int n = m->rows;
    
    if (n == 1) {
        return m->data[0][0];
    }
    
    if (n == 2) {
        return m->data[0][0] * m->data[1][1] - m->data[0][1] * m->data[1][0];
    }
    
    double det = 0.0;
    
    // Cofactor expansion along first row
    for (int j = 0; j < n; j++) {
        // Create submatrix
        Matrix *sub = create_matrix(n-1, n-1, "temp");
        
        for (int i = 1; i < n; i++) {
            int col_idx = 0;
            for (int k = 0; k < n; k++) {
                if (k != j) {
                    sub->data[i-1][col_idx++] = m->data[i][k];
                }
            }
        }
        
        double sign = (j % 2 == 0) ? 1.0 : -1.0;
        det += sign * m->data[0][j] * determinant_single(sub);
        
        free_matrix(sub);
    }
    
    return det;
}

double determinant_parallel(Matrix *m) {
    // For now, same as single (can be parallelized with OpenMP)
    double start_time = get_time_ms();
    double det = determinant_single(m);
    double end_time = get_time_ms();
    
    printf("[TIMING] Determinant calculation: %.2f ms\n", end_time - start_time);
    return det;
}
