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
#include <math.h>
#include <omp.h>
#include "worker_pool.h"
#include "matrix.h"

// ===== Global Variables =====
Worker *worker_pool = NULL;
int pool_size = 0;
int max_idle_time = 60;

// ===== Signal Handlers =====

void sigusr1_handler(int signo) {
    (void)signo;
}

void sigchld_handler(int signo) {
    (void)signo;
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
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
    signal(SIGPIPE, SIG_IGN);
}

// ===== Timing Functions =====

double get_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000.0) + (tv.tv_usec / 1000.0);
}

// ===== Helper Functions =====

double determinant_2x2(double a, double b, double c, double d) {
    return a * d - b * c;
}

double vector_norm(double *v, int n) {
    double sum = 0.0;
    #pragma omp parallel for reduction(+:sum)
    for (int i = 0; i < n; i++) {
        sum += v[i] * v[i];
    }
    return sqrt(sum);
}

void normalize_vector(double *v, int n) {
    double norm = vector_norm(v, n);
    if (norm > 1e-10) {
        #pragma omp parallel for
        for (int i = 0; i < n; i++) {
            v[i] /= norm;
        }
    }
}

// ===== Worker Process =====

void worker_process_loop(int input_fd, int output_fd) {
    WorkMessage msg;
    
    while (1) {
        ssize_t n = read(input_fd, &msg, sizeof(WorkMessage));
        if (n <= 0) break;
        
        switch (msg.op_type) {
            case OP_ADD:
                msg.result = msg.operand1 + msg.operand2;
                break;
                
            case OP_SUBTRACT:
                msg.result = msg.operand1 - msg.operand2;
                break;
                
            case OP_MULTIPLY_ELEMENT:
                msg.result = 0.0;
                for (int i = 0; i < msg.row_size; i++) {
                    msg.result += msg.row_data[i] * msg.col_data[i];
                }
                break;
                
            case OP_DETERMINANT_2X2:
                msg.result = determinant_2x2(msg.matrix_data[0][0], msg.matrix_data[0][1],
                                            msg.matrix_data[1][0], msg.matrix_data[1][1]);
                break;
                
            case OP_MATRIX_VECTOR_MULTIPLY:
                for (int i = 0; i < msg.matrix_size; i++) {
                    msg.row_data[i] = 0.0;
                    for (int j = 0; j < msg.matrix_size; j++) {
                        msg.row_data[i] += msg.matrix_data[i][j] * msg.vector_data[j];
                    }
                }
                break;
                
            case OP_EXIT:
                close(input_fd);
                close(output_fd);
                exit(0);
                
            default:
                msg.result = 0.0;
        }
        
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
        if (pipe(worker_pool[i].input_pipe) == -1 ||
            pipe(worker_pool[i].output_pipe) == -1) {
            perror("pipe");
            exit(1);
        }
        
        pid_t pid = fork();
        
        if (pid < 0) {
            perror("fork");
            exit(1);
        }
        
        if (pid == 0) {
            close(worker_pool[i].input_pipe[1]);
            close(worker_pool[i].output_pipe[0]);
            
            worker_process_loop(worker_pool[i].input_pipe[0],
                              worker_pool[i].output_pipe[1]);
            exit(0);
        }
        
        close(worker_pool[i].input_pipe[0]);
        close(worker_pool[i].output_pipe[1]);
        
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
            waitpid(worker_pool[i].pid, NULL, 0);
        }
    }
    
    free(worker_pool);
    worker_pool = NULL;
    printf("[INFO] Worker pool cleaned up\n");
}

// ===== REQUIRED: PROCESS-BASED OPERATIONS (with optional OpenMP in loops) =====

Matrix* add_matrices_with_processes(Matrix *m1, Matrix *m2) {
    if (m1->rows != m2->rows || m1->cols != m2->cols) {
        printf("Error: Matrices must have same dimensions\n");
        return NULL;
    }
    
    char result_name[50];
    snprintf(result_name, sizeof(result_name), "%s_plus_%s", m1->name, m2->name);
    Matrix *result = create_matrix(m1->rows, m1->cols, result_name);
    
    int total_elements = m1->rows * m1->cols;
    printf("[INFO] Using %d worker processes for %d elements\n", pool_size, total_elements);
    
    // Send work (OpenMP optional optimization for the sending loop)
    int work_idx = 0;
    #pragma omp parallel for collapse(2) if(total_elements > 1000)
    for (int i = 0; i < m1->rows; i++) {
        for (int j = 0; j < m1->cols; j++) {
            int local_idx;
            #pragma omp critical
            {
                local_idx = work_idx++;
            }
            int worker_id = local_idx % pool_size;
            Worker *w = &worker_pool[worker_id];
            
            WorkMessage msg = {
                .op_type = OP_ADD,
                .operand1 = m1->data[i][j],
                .operand2 = m2->data[i][j]
            };
            
            #pragma omp critical
            {
                write(w->input_pipe[1], &msg, sizeof(WorkMessage));
            }
        }
    }
    
    // Collect results
    work_idx = 0;
    for (int i = 0; i < m1->rows; i++) {
        for (int j = 0; j < m1->cols; j++) {
            int worker_id = work_idx % pool_size;
            Worker *w = &worker_pool[worker_id];
            
            WorkMessage msg;
            read(w->output_pipe[0], &msg, sizeof(WorkMessage));
            result->data[i][j] = msg.result;
            work_idx++;
        }
    }
    
    return result;
}

Matrix* subtract_matrices_with_processes(Matrix *m1, Matrix *m2) {
    if (m1->rows != m2->rows || m1->cols != m2->cols) {
        printf("Error: Matrices must have same dimensions\n");
        return NULL;
    }
    
    char result_name[50];
    snprintf(result_name, sizeof(result_name), "%s_minus_%s", m1->name, m2->name);
    Matrix *result = create_matrix(m1->rows, m1->cols, result_name);
    
    int work_idx = 0;
    for (int i = 0; i < m1->rows; i++) {
        for (int j = 0; j < m1->cols; j++) {
            int worker_id = work_idx % pool_size;
            Worker *w = &worker_pool[worker_id];
            
            WorkMessage msg = {
                .op_type = OP_SUBTRACT,
                .operand1 = m1->data[i][j],
                .operand2 = m2->data[i][j]
            };
            
            write(w->input_pipe[1], &msg, sizeof(WorkMessage));
            work_idx++;
        }
    }
    
    work_idx = 0;
    for (int i = 0; i < m1->rows; i++) {
        for (int j = 0; j < m1->cols; j++) {
            int worker_id = work_idx % pool_size;
            Worker *w = &worker_pool[worker_id];
            
            WorkMessage msg;
            read(w->output_pipe[0], &msg, sizeof(WorkMessage));
            result->data[i][j] = msg.result;
            work_idx++;
        }
    }
    
    return result;
}

Matrix* multiply_matrices_with_processes(Matrix *m1, Matrix *m2) {
    if (m1->cols != m2->rows) {
        printf("Error: Invalid dimensions for multiplication\n");
        return NULL;
    }
    
    if (m1->cols > MAX_VECTOR_SIZE) {
        printf("Error: Matrix too large for IPC (max dimension: %d)\n", MAX_VECTOR_SIZE);
        return NULL;
    }
    
    char result_name[50];
    snprintf(result_name, sizeof(result_name), "%s_times_%s", m1->name, m2->name);
    Matrix *result = create_matrix(m1->rows, m2->cols, result_name);
    
    printf("[INFO] Using worker processes for %dx%d result matrix\n", m1->rows, m2->cols);
    
    int work_idx = 0;
    for (int i = 0; i < m1->rows; i++) {
        for (int j = 0; j < m2->cols; j++) {
            int worker_id = work_idx % pool_size;
            Worker *w = &worker_pool[worker_id];
            
            WorkMessage msg = {
                .op_type = OP_MULTIPLY_ELEMENT,
                .row_size = m1->cols
            };
            
            for (int k = 0; k < m1->cols; k++) {
                msg.row_data[k] = m1->data[i][k];
            }
            
            for (int k = 0; k < m2->rows; k++) {
                msg.col_data[k] = m2->data[k][j];
            }
            
            write(w->input_pipe[1], &msg, sizeof(WorkMessage));
            work_idx++;
        }
    }
    
    work_idx = 0;
    for (int i = 0; i < m1->rows; i++) {
        for (int j = 0; j < m2->cols; j++) {
            int worker_id = work_idx % pool_size;
            Worker *w = &worker_pool[worker_id];
            
            WorkMessage msg;
            read(w->output_pipe[0], &msg, sizeof(WorkMessage));
            result->data[i][j] = msg.result;
            work_idx++;
        }
    }
    
    return result;
}

// ===== DETERMINANT WITH PROCESSES (with OpenMP optimization in recursive calls) =====

double determinant_recursive_processes(Matrix *m) {
    if (m->rows != m->cols) return 0.0;
    
    int n = m->rows;
    
    if (n == 1) {
        return m->data[0][0];
    }
    
    if (n == 2) {
        int worker_id = 0;
        Worker *w = &worker_pool[worker_id];
        
        WorkMessage msg = {.op_type = OP_DETERMINANT_2X2};
        msg.matrix_data[0][0] = m->data[0][0];
        msg.matrix_data[0][1] = m->data[0][1];
        msg.matrix_data[1][0] = m->data[1][0];
        msg.matrix_data[1][1] = m->data[1][1];
        
        write(w->input_pipe[1], &msg, sizeof(WorkMessage));
        read(w->output_pipe[0], &msg, sizeof(WorkMessage));
        
        return msg.result;
    }
    
    double det = 0.0;
    
    // OPTIONAL: OpenMP to parallelize cofactor expansion for larger matrices
    #pragma omp parallel for reduction(+:det) if(n > 5)
    for (int j = 0; j < n; j++) {
        Matrix *sub = create_matrix(n-1, n-1, "temp_sub");
        
        for (int i = 1; i < n; i++) {
            int col_idx = 0;
            for (int k = 0; k < n; k++) {
                if (k != j) {
                    sub->data[i-1][col_idx++] = m->data[i][k];
                }
            }
        }
        
        double sign = (j % 2 == 0) ? 1.0 : -1.0;
        det += sign * m->data[0][j] * determinant_recursive_processes(sub);
        
        free_matrix(sub);
    }
    
    return det;
}

double determinant_with_processes(Matrix *m) {
    if (m->rows != m->cols) {
        printf("Error: Matrix must be square\n");
        return 0.0;
    }
    
    return determinant_recursive_processes(m);
}

// ===== EIGENVALUES WITH PROCESSES (with OpenMP optimization) =====

void compute_eigen_with_processes(Matrix *m, int num_eigenvalues, double *eigenvalues, double **eigenvectors) {
    if (m->rows != m->cols || m->rows > MAX_MATRIX_SIZE) {
        printf("Error: Invalid matrix for eigenvalue computation\n");
        return;
    }
    
    int n = m->rows;
    double *v = malloc(n * sizeof(double));
    double *v_new = malloc(n * sizeof(double));
    
    // Initialize with simple vector
    #pragma omp parallel for
    for (int i = 0; i < n; i++) {
        v[i] = 1.0;
    }
    normalize_vector(v, n);
    
    int max_iterations = 1000;
    double tolerance = 1e-6;
    
    for (int iter = 0; iter < max_iterations; iter++) {
        // Matrix-vector multiply using processes
        int work_idx = 0;
        for (int i = 0; i < n; i++) {
            int worker_id = work_idx % pool_size;
            Worker *w = &worker_pool[worker_id];
            
            WorkMessage msg = {
                .op_type = OP_MATRIX_VECTOR_MULTIPLY,
                .matrix_size = n
            };
            
            for (int j = 0; j < n; j++) {
                msg.matrix_data[i][j] = m->data[i][j];
                msg.vector_data[j] = v[j];
            }
            
            write(w->input_pipe[1], &msg, sizeof(WorkMessage));
            work_idx++;
        }
        
        // Collect results
        work_idx = 0;
        for (int i = 0; i < n; i++) {
            int worker_id = work_idx % pool_size;
            Worker *w = &worker_pool[worker_id];
            
            WorkMessage msg;
            read(w->output_pipe[0], &msg, sizeof(WorkMessage));
            v_new[i] = msg.row_data[i];
            work_idx++;
        }
        
        // Compute eigenvalue (OPTIONAL: OpenMP for dot product)
        double lambda = 0.0;
        #pragma omp parallel for reduction(+:lambda)
        for (int i = 0; i < n; i++) {
            lambda += v_new[i] * v[i];
        }
        
        normalize_vector(v_new, n);
        
        // Check convergence
        double diff = 0.0;
        for (int i = 0; i < n; i++) {
            diff += fabs(v_new[i] - v[i]);
        }
        
        if (diff < tolerance) {
            eigenvalues[0] = lambda;
            for (int i = 0; i < n; i++) {
                eigenvectors[0][i] = v_new[i];
            }
            break;
        }
        
        for (int i = 0; i < n; i++) {
            v[i] = v_new[i];
        }
        
        if (iter == max_iterations - 1) {
            eigenvalues[0] = lambda;
            for (int i = 0; i < n; i++) {
                eigenvectors[0][i] = v[i];
            }
        }
    }
    
    free(v);
    free(v_new);
}

// ===== SINGLE-THREADED VERSIONS FOR COMPARISON =====

Matrix* add_matrices_single(Matrix *m1, Matrix *m2) {
    if (m1->rows != m2->rows || m1->cols != m2->cols) return NULL;
    
    char result_name[50];
    snprintf(result_name, sizeof(result_name), "%s_plus_%s_single", m1->name, m2->name);
    Matrix *result = create_matrix(m1->rows, m1->cols, result_name);
    
    for (int i = 0; i < m1->rows; i++) {
        for (int j = 0; j < m1->cols; j++) {
            result->data[i][j] = m1->data[i][j] + m2->data[i][j];
        }
    }
    
    return result;
}

Matrix* subtract_matrices_single(Matrix *m1, Matrix *m2) {
    if (m1->rows != m2->rows || m1->cols != m2->cols) return NULL;
    
    char result_name[50];
    snprintf(result_name, sizeof(result_name), "%s_minus_%s_single", m1->name, m2->name);
    Matrix *result = create_matrix(m1->rows, m1->cols, result_name);
    
    for (int i = 0; i < m1->rows; i++) {
        for (int j = 0; j < m1->cols; j++) {
            result->data[i][j] = m1->data[i][j] - m2->data[i][j];
        }
    }
    
    return result;
}

Matrix* multiply_matrices_single(Matrix *m1, Matrix *m2) {
    if (m1->cols != m2->rows) return NULL;
    
    char result_name[50];
    snprintf(result_name, sizeof(result_name), "%s_times_%s_single", m1->name, m2->name);
    Matrix *result = create_matrix(m1->rows, m2->cols, result_name);
    
    for (int i = 0; i < m1->rows; i++) {
        for (int j = 0; j < m2->cols; j++) {
            result->data[i][j] = 0.0;
            for (int k = 0; k < m1->cols; k++) {
                result->data[i][j] += m1->data[i][k] * m2->data[k][j];
            }
        }
    }
    
    return result;
}

double determinant_recursive_single(Matrix *m) {
    if (m->rows != m->cols) return 0.0;
    
    int n = m->rows;
    
    if (n == 1) return m->data[0][0];
    
    if (n == 2) {
        return m->data[0][0] * m->data[1][1] - m->data[0][1] * m->data[1][0];
    }
    
    double det = 0.0;
    
    for (int j = 0; j < n; j++) {
        Matrix *sub = create_matrix(n-1, n-1, "temp_sub");
        
        for (int i = 1; i < n; i++) {
            int col_idx = 0;
            for (int k = 0; k < n; k++) {
                if (k != j) {
                    sub->data[i-1][col_idx++] = m->data[i][k];
                }
            }
        }
        
        double sign = (j % 2 == 0) ? 1.0 : -1.0;
        det += sign * m->data[0][j] * determinant_recursive_single(sub);
        
        free_matrix(sub);
    }
    
    return det;
}

double determinant_single(Matrix *m) {
    if (m->rows != m->cols) {
        printf("Error: Matrix must be square\n");
        return 0.0;
    }
    
    return determinant_recursive_single(m);
}

void compute_eigen_single(Matrix *m, int num_eigenvalues, double *eigenvalues, double **eigenvectors) {
    if (m->rows != m->cols) {
        printf("Error: Matrix must be square\n");
        return;
    }
    
    int n = m->rows;
    double *v = malloc(n * sizeof(double));
    double *v_new = malloc(n * sizeof(double));
    
    for (int i = 0; i < n; i++) {
        v[i] = 1.0;
    }
    normalize_vector(v, n);
    
    int max_iterations = 1000;
    double tolerance = 1e-6;
    
    for (int iter = 0; iter < max_iterations; iter++) {
        for (int i = 0; i < n; i++) {
            v_new[i] = 0.0;
            for (int j = 0; j < n; j++) {
                v_new[i] += m->data[i][j] * v[j];
            }
        }
        
        double lambda = 0.0;
        for (int i = 0; i < n; i++) {
            lambda += v_new[i] * v[i];
        }
        
        normalize_vector(v_new, n);
        
        double diff = 0.0;
        for (int i = 0; i < n; i++) {
            diff += fabs(v_new[i] - v[i]);
        }
        
        if (diff < tolerance) {
            eigenvalues[0] = lambda;
            for (int i = 0; i < n; i++) {
                eigenvectors[0][i] = v_new[i];
            }
            break;
        }
        
        for (int i = 0; i < n; i++) {
            v[i] = v_new[i];
        }
        
        if (iter == max_iterations - 1) {
            eigenvalues[0] = lambda;
            for (int i = 0; i < n; i++) {
                eigenvectors[0][i] = v[i];
            }
        }
    }
    
    free(v);
    free(v_new);
}
