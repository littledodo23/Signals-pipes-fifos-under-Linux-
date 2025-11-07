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
#include <fcntl.h>
#include <sys/stat.h>

// ===== Global Variables =====
Worker *worker_pool = NULL;
int pool_size = 0;
#define STATUS_FIFO "/tmp/matrix_status_fifo"
static int status_fifo_fd = -1;
static pid_t monitor_pid = -1;
int max_idle_time = 60;
volatile sig_atomic_t workers_completed = 0;

// ===== Signal Handlers =====
void sigusr1_handler(int signo) {
    (void)signo;
    workers_completed++;
}

void sigchld_handler(int signo) {
    (void)signo;
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        workers_completed++;
    }
}

void setup_signal_handlers(void) {
    struct sigaction sa_usr1;
    sa_usr1.sa_handler = sigusr1_handler;
    sigemptyset(&sa_usr1.sa_mask);
    sa_usr1.sa_flags = 0;
    sigaction(SIGUSR1, &sa_usr1, NULL);
    
    struct sigaction sa_chld;
    sa_chld.sa_handler = sigchld_handler;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa_chld, NULL);
    
    signal(SIGPIPE, SIG_IGN);
}

double get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000.0) + (tv.tv_usec / 1000.0);
}

// ===== Helper Functions =====
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

// ===== Worker Process Loop =====
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
                msg.result = msg.matrix_data[0][0] * msg.matrix_data[1][1] - 
                            msg.matrix_data[0][1] * msg.matrix_data[1][0];
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
        kill(getppid(), SIGUSR1);
    }
    
    close(input_fd);
    close(output_fd);
    exit(0);
}

// ===== FIFO IMPLEMENTATION =====
void init_status_fifo(void) {
    unlink(STATUS_FIFO);
    
    if (mkfifo(STATUS_FIFO, 0666) == -1) {
        perror("[FIFO] mkfifo failed");
        return;
    }
    
    printf("[FIFO] Status FIFO created at: %s\n", STATUS_FIFO);
}

void send_status_via_fifo(const char *status_msg) {
    if (status_fifo_fd == -1) {
        status_fifo_fd = open(STATUS_FIFO, O_WRONLY | O_NONBLOCK);
        if (status_fifo_fd == -1) {
            return;
        }
    }
    
    StatusMessage msg;
    strncpy(msg.status, status_msg, sizeof(msg.status) - 1);
    msg.status[sizeof(msg.status) - 1] = '\0';
    msg.worker_count = pool_size;
    
    msg.active_workers = 0;
    for (int i = 0; i < pool_size; i++) {
        if (worker_pool && worker_pool[i].alive && !worker_pool[i].available) {
            msg.active_workers++;
        }
    }
    
    msg.timestamp = get_time_ms();
    
    ssize_t written = write(status_fifo_fd, &msg, sizeof(StatusMessage));
    if (written == -1 && errno != EAGAIN) {
        close(status_fifo_fd);
        status_fifo_fd = -1;
    }
}

void cleanup_status_fifo(void) {
    if (status_fifo_fd != -1) {
        close(status_fifo_fd);
        status_fifo_fd = -1;
    }
    unlink(STATUS_FIFO);
    printf("[FIFO] Status FIFO cleaned up\n");
}

void monitor_status_fifo_background(void) {
    monitor_pid = fork();
    
    if (monitor_pid == 0) {
        printf("[FIFO MONITOR] Started (PID: %d)\n", getpid());
        
        int fd = open(STATUS_FIFO, O_RDONLY);
        if (fd == -1) {
            perror("[FIFO MONITOR] open failed");
            exit(1);
        }
        
        StatusMessage msg;
        while (1) {
            ssize_t n = read(fd, &msg, sizeof(StatusMessage));
            if (n == sizeof(StatusMessage)) {
                printf("\n[FIFO MONITOR] Status Update:\n");
                printf("  Status: %s\n", msg.status);
                printf("  Workers: %d/%d active\n", msg.active_workers, msg.worker_count);
                printf("  Timestamp: %.2f ms\n\n", msg.timestamp);
            } else if (n == 0) {
                close(fd);
                fd = open(STATUS_FIFO, O_RDONLY);
            }
        }
        
        close(fd);
        exit(0);
    }
    
    usleep(100000);
}

// ===== Worker Pool Management =====
void init_worker_pool(int size) {
    pool_size = size;
    worker_pool = malloc(size * sizeof(Worker));
    
    printf("[INFO] Initializing worker pool with %d workers...\n", size);
    
    init_status_fifo();
    monitor_status_fifo_background();
    
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
    send_status_via_fifo("POOL_READY");
}

Worker* get_available_worker(void) {
    for (int i = 0; i < pool_size; i++) {
        if (worker_pool[i].alive && worker_pool[i].available) {
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

void age_workers(void) {
    time_t now = time(NULL);
    for (int i = 0; i < pool_size; i++) {
        if (worker_pool[i].alive && worker_pool[i].available) {
            if (now - worker_pool[i].last_used > max_idle_time) {
                WorkMessage msg = {.op_type = OP_EXIT};
                write(worker_pool[i].input_pipe[1], &msg, sizeof(WorkMessage));
                worker_pool[i].alive = 0;
                printf("[INFO] Aged out worker %d (idle for %ld seconds)\n",
                       i, (long)(now - worker_pool[i].last_used));
            }
        }
    }
}

void cleanup_worker_pool(void) {
    if (!worker_pool) return;
    
    printf("[INFO] Cleaning up worker pool...\n");
    send_status_via_fifo("POOL_SHUTDOWN");
    
    for (int i = 0; i < pool_size; i++) {
        if (worker_pool[i].alive) {
            WorkMessage msg = {.op_type = OP_EXIT};
            write(worker_pool[i].input_pipe[1], &msg, sizeof(WorkMessage));
            close(worker_pool[i].input_pipe[1]);
            close(worker_pool[i].output_pipe[0]);
            waitpid(worker_pool[i].pid, NULL, 0);
        }
    }
    
    if (monitor_pid > 0) {
        kill(monitor_pid, SIGTERM);
        waitpid(monitor_pid, NULL, 0);
    }
    cleanup_status_fifo();
    
    free(worker_pool);
    worker_pool = NULL;
    printf("[INFO] Worker pool cleaned up\n");
}

Matrix* add_matrices_with_processes(Matrix *m1, Matrix *m2) {
    if (m1->rows != m2->rows || m1->cols != m2->cols) {
        printf("Error: Matrices must have same dimensions\n");
        return NULL;
    }
    
    char result_name[128];
    snprintf(result_name, sizeof(result_name), "%s_plus_%s", m1->name, m2->name);
    Matrix *result = create_matrix(m1->rows, m1->cols, result_name);
    
    int total_elements = m1->rows * m1->cols;
    printf("[INFO] Creating %d child processes\n", total_elements);
    
    pid_t *pids = malloc(total_elements * sizeof(pid_t));
    int (*pipes)[2] = malloc(total_elements * sizeof(int[2]));
    
    workers_completed = 0;
    send_status_via_fifo("ADD_OPERATION_START");
    
    int idx = 0;
    for (int i = 0; i < m1->rows; i++) {
        for (int j = 0; j < m1->cols; j++) {
            if (pipe(pipes[idx]) == -1) {
                perror("pipe");
                exit(1);
            }
            
            pid_t pid = fork();
            if (pid < 0) {
                perror("fork");
                exit(1);
            }
            
            if (pid == 0) {
                close(pipes[idx][0]);
                double result_val = m1->data[i][j] + m2->data[i][j];
                write(pipes[idx][1], &result_val, sizeof(double));
                close(pipes[idx][1]);
                kill(getppid(), SIGUSR1);
                exit(0);
            }
            
            close(pipes[idx][1]);
            pids[idx] = pid;
            idx++;
        }
    }
    
    idx = 0;
    for (int i = 0; i < m1->rows; i++) {
        for (int j = 0; j < m1->cols; j++) {
            double result_val;
            ssize_t n = read(pipes[idx][0], &result_val, sizeof(double));
            if (n > 0) {
                result->data[i][j] = result_val;
            }
            close(pipes[idx][0]);
            waitpid(pids[idx], NULL, 0);
            idx++;
        }
    }
    
    printf("[INFO] All %d processes completed\n", total_elements);
    
    free(pids);
    free(pipes);
    send_status_via_fifo("ADD_OPERATION_COMPLETE");
    
    return result;
}

Matrix* subtract_matrices_with_processes(Matrix *m1, Matrix *m2) {
    if (m1->rows != m2->rows || m1->cols != m2->cols) {
        printf("Error: Matrices must have same dimensions\n");
        return NULL;
    }
    
    char result_name[128];
    snprintf(result_name, sizeof(result_name), "%s_minus_%s", m1->name, m2->name);
    Matrix *result = create_matrix(m1->rows, m1->cols, result_name);
    
    int total_elements = m1->rows * m1->cols;
    
    pid_t *pids = malloc(total_elements * sizeof(pid_t));
    int (*pipes)[2] = malloc(total_elements * sizeof(int[2]));
    
    workers_completed = 0;
    send_status_via_fifo("SUBTRACT_OPERATION_START");
    
    int idx = 0;
    for (int i = 0; i < m1->rows; i++) {
        for (int j = 0; j < m1->cols; j++) {
            if (pipe(pipes[idx]) == -1) {
                perror("pipe");
                exit(1);
            }
            
            pid_t pid = fork();
            if (pid < 0) {
                perror("fork");
                exit(1);
            }
            
            if (pid == 0) {
                close(pipes[idx][0]);
                double result_val = m1->data[i][j] - m2->data[i][j];
                write(pipes[idx][1], &result_val, sizeof(double));
                close(pipes[idx][1]);
                kill(getppid(), SIGUSR1);
                exit(0);
            }
            
            close(pipes[idx][1]);
            pids[idx] = pid;
            idx++;
        }
    }
    
    idx = 0;
    for (int i = 0; i < m1->rows; i++) {
        for (int j = 0; j < m1->cols; j++) {
            double result_val;
            read(pipes[idx][0], &result_val, sizeof(double));
            result->data[i][j] = result_val;
            close(pipes[idx][0]);
            waitpid(pids[idx], NULL, 0);
            idx++;
        }
    }
    
    free(pids);
    free(pipes);
    send_status_via_fifo("SUBTRACT_OPERATION_COMPLETE");
    
    return result;
}

Matrix* multiply_matrices_with_processes(Matrix *m1, Matrix *m2) {
    if (m1->cols != m2->rows) {
        printf("Error: Invalid dimensions\n");
        return NULL;
    }
    
    char result_name[128];
    snprintf(result_name, sizeof(result_name), "%s_times_%s", m1->name, m2->name);
    Matrix *result = create_matrix(m1->rows, m2->cols, result_name);
    
    int total_processes = m1->rows * m2->cols;
    pid_t *pids = malloc(total_processes * sizeof(pid_t));
    int (*pipes)[2] = malloc(total_processes * sizeof(int[2]));
    
    workers_completed = 0;
    send_status_via_fifo("MULTIPLY_OPERATION_START");
    int idx = 0;
    
    for (int i = 0; i < m1->rows; i++) {
        for (int j = 0; j < m2->cols; j++) {
            if (pipe(pipes[idx]) == -1) {
                perror("pipe");
                exit(1);
            }
            
            pid_t pid = fork();
            if (pid < 0) {
                perror("fork");
                exit(1);
            }
            
            if (pid == 0) {
                close(pipes[idx][0]);
                
                double result_val = 0.0;
                for (int k = 0; k < m1->cols; k++) {
                    result_val += m1->data[i][k] * m2->data[k][j];
                }
                
                write(pipes[idx][1], &result_val, sizeof(double));
                close(pipes[idx][1]);
                kill(getppid(), SIGUSR1);
                exit(0);
            }
            
            close(pipes[idx][1]);
            pids[idx] = pid;
            idx++;
        }
    }
    
    idx = 0;
    for (int i = 0; i < m1->rows; i++) {
        for (int j = 0; j < m2->cols; j++) {
            double result_val;
            read(pipes[idx][0], &result_val, sizeof(double));
            result->data[i][j] = result_val;
            close(pipes[idx][0]);
            waitpid(pids[idx], NULL, 0);
            idx++;
        }
    }
    
    free(pids);
    free(pipes);
    send_status_via_fifo("MULTIPLY_OPERATION_COMPLETE");
    return result;
}

double determinant_recursive_processes(Matrix *m) {
    if (m->rows != m->cols) return 0.0;
    
    int n = m->rows;
    
    if (n == 1) return m->data[0][0];
    if (n == 2) return m->data[0][0] * m->data[1][1] - m->data[0][1] * m->data[1][0];
    
    int num_processes = n;
    pid_t *pids = malloc(num_processes * sizeof(pid_t));
    int (*pipes)[2] = malloc(num_processes * sizeof(int[2]));
    
    workers_completed = 0;
    
    for (int j = 0; j < n; j++) {
        if (pipe(pipes[j]) == -1) {
            perror("pipe");
            exit(1);
        }
        
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(1);
        }
        
        if (pid == 0) {
            close(pipes[j][0]);
            
            Matrix *sub = create_matrix(n-1, n-1, "temp_sub");
            for (int i = 1; i < n; i++) {
                int col_idx = 0;
                for (int k = 0; k < n; k++) {
                    if (k != j) {
                        sub->data[i-1][col_idx++] = m->data[i][k];
                    }
                }
            }
            
            double sub_det = determinant_recursive_processes(sub);
            double sign = (j % 2 == 0) ? 1.0 : -1.0;
            double cofactor = sign * m->data[0][j] * sub_det;
            
            write(pipes[j][1], &cofactor, sizeof(double));
            close(pipes[j][1]);
            
            free_matrix(sub);
            kill(getppid(), SIGUSR1);
            exit(0);
        }
        
        close(pipes[j][1]);
        pids[j] = pid;
    }
    
    double det = 0.0;
    for (int j = 0; j < n; j++) {
        double cofactor;
        read(pipes[j][0], &cofactor, sizeof(double));
        det += cofactor;
        close(pipes[j][0]);
        waitpid(pids[j], NULL, 0);
    }
    
    free(pids);
    free(pipes);
    
    return det;
}

double determinant_with_processes(Matrix *m) {
    if (m->rows != m->cols) {
        printf("Error: Matrix must be square\n");
        return 0.0;
    }
    
    return determinant_recursive_processes(m);
}

double determinant_parallel(Matrix *m) {
    return determinant_with_processes(m);
}

void compute_eigen_with_processes(Matrix *m, int num_eigenvalues, double *eigenvalues, double **eigenvectors) {
    (void)num_eigenvalues;
    if (m->rows != m->cols) {
        printf("Error: Invalid matrix\n");
        return;
    }
    
    int n = m->rows;
    
    double *v = malloc(n * sizeof(double));
    double *v_new = malloc(n * sizeof(double));
    
    #pragma omp parallel for
    for (int i = 0; i < n; i++) {
        v[i] = 1.0;
    }
    normalize_vector(v, n);
    
    int max_iterations = 1000;
    double tolerance = 1e-6;
    
    for (int iter = 0; iter < max_iterations; iter++) {
        pid_t *pids = malloc(n * sizeof(pid_t));
        int (*pipes)[2] = malloc(n * sizeof(int[2]));
        
        workers_completed = 0;
        
        for (int i = 0; i < n; i++) {
            if (pipe(pipes[i]) == -1) {
                perror("pipe");
                exit(1);
            }
            
            pid_t pid = fork();
            if (pid < 0) {
                perror("fork");
                exit(1);
            }
            
            if (pid == 0) {
                close(pipes[i][0]);
                
                double row_result = 0.0;
                
                #pragma omp parallel for reduction(+:row_result)
                for (int j = 0; j < n; j++) {
                    row_result += m->data[i][j] * v[j];
                }
                
                write(pipes[i][1], &row_result, sizeof(double));
                close(pipes[i][1]);
                
                kill(getppid(), SIGUSR1);
                exit(0);
            }
            
            close(pipes[i][1]);
            pids[i] = pid;
        }
        
        for (int i = 0; i < n; i++) {
            read(pipes[i][0], &v_new[i], sizeof(double));
            close(pipes[i][0]);
            waitpid(pids[i], NULL, 0);
        }
        
        free(pids);
        free(pipes);
        
        double lambda = 0.0;
        #pragma omp parallel for reduction(+:lambda)
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

Matrix* add_matrices_single(Matrix *m1, Matrix *m2) {
    if (m1->rows != m2->rows || m1->cols != m2->cols) return NULL;
    
    char result_name[128];
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
    
    char result_name[128];
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
    
    char result_name[128];
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

double determinant_single(Matrix *m) {
    if (m->rows != m->cols) return 0.0;
    int n = m->rows;
    
    if (n == 1) return m->data[0][0];
    if (n == 2) return m->data[0][0] * m->data[1][1] - m->data[0][1] * m->data[1][0];
    
    double det = 0.0;
    for (int j = 0; j < n; j++) {
        Matrix *sub = create_matrix(n-1, n-1, "temp_sub");
        for (int i = 1; i < n; i++) {
            int col_idx = 0;
            for (int k = 0; k < n; k++) {
                if (k != j) sub->data[i-1][col_idx++] = m->data[i][k];
            }
        }
        double sign = (j % 2 == 0) ? 1.0 : -1.0;
        det += sign * m->data[0][j] * determinant_single(sub);
        free_matrix(sub);
    }
    return det;
}
