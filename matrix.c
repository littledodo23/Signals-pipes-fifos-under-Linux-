#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "matrix.h"
#include <dirent.h>
#include <math.h>

// ===== Global Variables =====
Matrix *matrices[MAX_MATRICES];
int matrix_count = 0;

// ===== Helper Functions =====
Matrix *create_matrix(int rows, int cols, const char *name) {
    Matrix *m = malloc(sizeof(Matrix));
    if (!m) {
        printf("Memory allocation failed.\n");
        exit(1);
    }
    strcpy(m->name, name);
    m->rows = rows;
    m->cols = cols;

    m->data = malloc(rows * sizeof(double *));
    for (int i = 0; i < rows; i++) {
        m->data[i] = calloc(cols, sizeof(double));
    }
    return m;
}

void free_matrix(Matrix *m) {
    for (int i = 0; i < m->rows; i++) {
        free(m->data[i]);
    }
    free(m->data);
    free(m);
}

void print_matrix(Matrix *m) {
    printf("Matrix %s (%dx%d):\n", m->name, m->rows, m->cols);
    for (int i = 0; i < m->rows; i++) {
        for (int j = 0; j < m->cols; j++)
            printf("%8.2lf ", m->data[i][j]);
        printf("\n");
    }
}

// ===== Menu Options =====
void enter_matrix();
void display_matrix();
void delete_matrix();
void modify_matrix();
void display_all_matrices();
void read_matrix_from_file_option();
void save_matrix_to_file_option();
double determinant_parallel(Matrix *m);
void eigen_parallel(Matrix *m, double *eigenvalue, double *eigenvector);

// ===== SINGLE-THREADED Operations =====
Matrix* add_matrices_single(Matrix *a, Matrix *b) {
    if (a->rows != b->rows || a->cols != b->cols) 
        return NULL;
    char name[50]; 
    sprintf(name, "%s_plus_%s", a->name, b->name);
    Matrix *res = create_matrix(a->rows, a->cols, name);
    for (int i = 0; i < a->rows; i++)
        for (int j = 0; j < a->cols; j++)
            res->data[i][j] = a->data[i][j] + b->data[i][j];
    return res;
}

Matrix* subtract_matrices_single(Matrix *a, Matrix *b) {
    if (a->rows != b->rows || a->cols != b->cols) 
        return NULL;
    char name[50]; 
    sprintf(name, "%s_minus_%s", a->name, b->name);
    Matrix *res = create_matrix(a->rows, a->cols, name);
    for (int i = 0; i < a->rows; i++)
        for (int j = 0; j < a->cols; j++)
            res->data[i][j] = a->data[i][j] - b->data[i][j];
    return res;
}

Matrix* multiply_matrices_single(Matrix *a, Matrix *b) {
    if (a->cols != b->rows) 
        return NULL;
    char name[50]; 
    sprintf(name, "%s_mul_%s", a->name, b->name);
    Matrix *res = create_matrix(a->rows, b->cols, name);
    for (int i = 0; i < a->rows; i++)
        for (int j = 0; j < b->cols; j++) {
            double sum = 0;
            for (int k = 0; k < a->cols; k++)
                sum += a->data[i][k] * b->data[k][j];
            res->data[i][j] = sum;
        }
    return res;
}

// ===== PARALLEL Operations using fork + pipe =====
Matrix* add_matrices_parallel(Matrix *a, Matrix *b) {
    if (a->rows != b->rows || a->cols != b->cols) 
        return NULL;

    char name[50]; 
    sprintf(name, "%s_plus_%s", a->name, b->name);
    Matrix *res = create_matrix(a->rows, a->cols, name);

    int total = a->rows * a->cols;
    int (*pipefds)[2] = malloc(total * sizeof(int[2]));
    int idx = 0;

    for (int i = 0; i < a->rows; i++) {
        for (int j = 0; j < a->cols; j++) {
            pipe(pipefds[idx]);
            pid_t pid = fork();
            if (pid == 0) { // child
                close(pipefds[idx][0]);
                double val = a->data[i][j] + b->data[i][j];
                write(pipefds[idx][1], &val, sizeof(double));
                close(pipefds[idx][1]);
                exit(0);
            } else { // parent
                close(pipefds[idx][1]);
            }
            idx++;
        }
    }

    idx = 0;
    for (int i = 0; i < a->rows; i++) {
        for (int j = 0; j < a->cols; j++) {
            wait(NULL);
            read(pipefds[idx][0], &res->data[i][j], sizeof(double));
            close(pipefds[idx][0]);
            idx++;
        }
    }

    free(pipefds);
    return res;
}


Matrix* subtract_matrices_parallel(Matrix *a, Matrix *b) {
    if (a->rows != b->rows || a->cols != b->cols) 
        return NULL;

    char name[50]; 
    sprintf(name, "%s_minus_%s", a->name, b->name);
    Matrix *res = create_matrix(a->rows, a->cols, name);

    int total = a->rows * a->cols;
    int (*pipefds)[2] = malloc(total * sizeof(int[2]));
    int idx = 0;

    for (int i = 0; i < a->rows; i++) {
        for (int j = 0; j < a->cols; j++) {
            pipe(pipefds[idx]);
            pid_t pid = fork();
            if (pid == 0) { // child
                close(pipefds[idx][0]);
                double val = a->data[i][j] - b->data[i][j];
                write(pipefds[idx][1], &val, sizeof(double));
                close(pipefds[idx][1]);
                exit(0);
            } else { // parent
                close(pipefds[idx][1]);
            }
            idx++;
        }
    }

    idx = 0;
    for (int i = 0; i < a->rows; i++) {
        for (int j = 0; j < a->cols; j++) {
            wait(NULL);
            read(pipefds[idx][0], &res->data[i][j], sizeof(double));
            close(pipefds[idx][0]);
            idx++;
        }
    }

    free(pipefds);
    return res;
}


Matrix* multiply_matrices_parallel(Matrix *a, Matrix *b) {
    if (a->cols != b->rows) 
        return NULL;

    char name[50]; 
    sprintf(name, "%s_mul_%s", a->name, b->name);
    Matrix *res = create_matrix(a->rows, b->cols, name);

    int total_elements = a->rows * b->cols;
    int (*pipefds)[2] = malloc(total_elements * sizeof(int[2]));
    int idx = 0;

    for (int i = 0; i < a->rows; i++) {
        for (int j = 0; j < b->cols; j++) {
            pipe(pipefds[idx]);
            pid_t pid = fork();
            if (pid == 0) { // child
                close(pipefds[idx][0]);
                double sum = 0;
                for (int k = 0; k < a->cols; k++)
                    sum += a->data[i][k] * b->data[k][j];
                write(pipefds[idx][1], &sum, sizeof(double));
                close(pipefds[idx][1]);
                exit(0);
            } else { 
                close(pipefds[idx][1]);
            }
            idx++;
        }
    }

    // parent يقرأ النتائج
    idx = 0;
    for (int i = 0; i < a->rows; i++) {
        for (int j = 0; j < b->cols; j++) {
            wait(NULL);
            read(pipefds[idx][0], &res->data[i][j], sizeof(double));
            close(pipefds[idx][0]);
            idx++;
        }
    }

    free(pipefds);
    return res;
}

// ===== Menu Functions =====
void enter_matrix() {
    if (matrix_count >= MAX_MATRICES) {
        printf("Memory full! Cannot store more matrices.\n");
        return;
    }
    char name[50]; int rows, cols;
    printf("Enter matrix name: "); 
    scanf("%s", name);
    printf("Enter number of rows: "); 
    scanf("%d", &rows);
    printf("Enter number of columns: "); 
    scanf("%d", &cols);

    Matrix *m = create_matrix(rows, cols, name);
    printf("Enter elements row by row:\n");
    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++)
            scanf("%lf", &m->data[i][j]);
    matrices[matrix_count++] = m;
    printf("Matrix '%s' saved in memory.\n", name);
}

void display_matrix() {
    if (matrix_count == 0) { 
        printf("No matrices in memory.\n"); return; }
    for (int i = 0; i < matrix_count; i++)
        printf("%d. %s (%dx%d)\n", i+1, matrices[i]->name, matrices[i]->rows, matrices[i]->cols);
    int choice; 
    printf("Enter number to display: "); 
    scanf("%d", &choice);
    if (choice < 1 || choice > matrix_count) { 
        printf("Invalid choice.\n"); return; }
    print_matrix(matrices[choice-1]);
}

void delete_matrix() {
    if (matrix_count == 0) { 
        printf("No matrices to delete.\n"); 
        return; }
    for (int i = 0; i < matrix_count; i++) 
        printf("%d. %s\n", i+1, matrices[i]->name);
    int index; printf("Enter number of matrix to delete: "); 
    scanf("%d", &index);
    if (index < 1 || index > matrix_count) { 
        printf("Invalid choice.\n"); 
                                            return; }
    free_matrix(matrices[index-1]);
    for (int i = index-1; i < matrix_count-1; i++) matrices[i] = matrices[i+1];
    matrix_count--; 
    printf("Matrix deleted successfully.\n");
}

void modify_matrix() {
    if (matrix_count == 0) { 
        printf("No matrices to modify.\n"); 
        return; }
    for (int i = 0; i < matrix_count; i++) 
        printf("%d. %s\n", i+1, matrices[i]->name);
    int choice; 
    printf("Choose a matrix: "); 
    scanf("%d", &choice);
    if (choice < 1 || choice > matrix_count) 
        return;
    Matrix *m = matrices[choice-1]; int mode;
    printf("1. Modify full row\n2. Modify full column\n3. Modify one value\nChoice: "); 
    scanf("%d", &mode);
    if (mode==1){ int row; 
                 printf("Enter row index (1-%d): ", m->rows); 
                 scanf("%d",&row);
        for(int j=0;j<m->cols;j++){ 
            printf("New value [%d][%d]: ", row,j+1); 
            scanf("%lf",&m->data[row-1][j]);}}
    else if(mode==2){ int col; 
                     printf("Enter column index (1-%d): ", m->cols); 
                     scanf("%d",&col);
        for(int i=0;i<m->rows;i++){ 
            printf("New value [%d][%d]: ", i+1,col); 
            scanf("%lf",&m->data[i][col-1]);}}
    else if(mode==3){ int r,c; 
                     printf("Enter row and column (e.g., 2 3): "); 
                     scanf("%d %d",&r,&c); 
                     printf("New value: "); 
                     scanf("%lf",&m->data[r-1][c-1]);}
    printf("Matrix updated.\n");
}

void display_all_matrices() {
    if(matrix_count==0){
        printf("No matrices in memory.\n"); 
        return;}
    for(int i=0;i<matrix_count;i++) 
        print_matrix(matrices[i]);
}

void read_matrix_from_file_option() {
    if (matrix_count >= MAX_MATRICES) {
        printf("Memory full! Cannot load more matrices.\n");
        return;
    }

    char filename[100];
    printf("Enter file path (e.g., matrices/A.txt): ");
    scanf("%s", filename);

    FILE *f = fopen(filename, "r");
    if (!f) {
        perror("Error opening file");
        return;
    }

    char name[50];
    int rows, cols;
    if (fscanf(f, "%s %d %d", name, &rows, &cols) != 3) {
        printf("Invalid file format.\n");
        fclose(f);
        return;
    }

    Matrix *m = create_matrix(rows, cols, name);
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (fscanf(f, "%lf", &m->data[i][j]) != 1) {
                printf("Error reading matrix values.\n");
                fclose(f);
                free_matrix(m);
                return;
            }
        }
    }

    fclose(f);
    matrices[matrix_count++] = m;
    printf("Matrix '%s' loaded successfully from %s\n", name, filename);
}


void save_matrix_to_file_option() {
    if (matrix_count == 0) {
        printf("No matrices to save.\n");
        return;
    }

    printf("Select matrix to save:\n");
    for (int i = 0; i < matrix_count; i++)
        printf("%d. %s (%dx%d)\n", i + 1, matrices[i]->name, matrices[i]->rows, matrices[i]->cols);

    int choice;
    printf("Enter choice: ");
    scanf("%d", &choice);

    if (choice < 1 || choice > matrix_count) {
        printf("Invalid selection.\n");
        return;
    }

    char filename[100];
    printf("Enter filename to save (e.g., matrices/result.txt): ");
    scanf("%s", filename);

    FILE *f = fopen(filename, "w");
    if (!f) {
        perror("Error creating file");
        return;
    }

    Matrix *m = matrices[choice - 1];
    fprintf(f, "%s %d %d\n", m->name, m->rows, m->cols);
    for (int i = 0; i < m->rows; i++) {
        for (int j = 0; j < m->cols; j++)
            fprintf(f, "%.4lf ", m->data[i][j]);
        fprintf(f, "\n");
    }

    fclose(f);
    printf("Matrix '%s' saved successfully to %s\n", m->name, filename);
}


// ===== Determinant Calculation (Parallel using fork) =====
double determinant_parallel(Matrix *m) {
    if (m->rows != m->cols) {
        printf("Matrix must be square!\n");
        return 0.0;
    }

    int n = m->rows;

    // Base cases
    if (n == 1) return m->data[0][0];
    if (n == 2) return (m->data[0][0] * m->data[1][1]) - (m->data[0][1] * m->data[1][0]);

    double det = 0.0;
    int pipes[n][2];

    for (int i = 0; i < n; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("Pipe creation failed");
            exit(1);
        }

        pid_t pid = fork();
        if (pid == 0) { // Child process
            close(pipes[i][0]); // close read end

            // Build submatrix (minor)
            Matrix *minor = create_matrix(n - 1, n - 1, "minor");
            for (int r = 1; r < n; r++) {
                int c_index = 0;
                for (int c = 0; c < n; c++) {
                    if (c == i) continue;
                    minor->data[r - 1][c_index++] = m->data[r][c];
                }
            }

            double sub_det = determinant_parallel(minor);
            free_matrix(minor);

            double sign = (i % 2 == 0) ? 1.0 : -1.0;
            double part = sign * m->data[0][i] * sub_det;

            write(pipes[i][1], &part, sizeof(double));
            close(pipes[i][1]);
            kill(getppid(), SIGUSR1); // send signal to parent
            exit(0);
        }
        else if (pid < 0) {
            perror("Fork failed");
            exit(1);
        }
        close(pipes[i][1]);
    }

    // Parent collects results
    for (int i = 0; i < n; i++) {
        double val;
        read(pipes[i][0], &val, sizeof(double));
        det += val;
        close(pipes[i][0]);
        wait(NULL);
    }

    return det;
}



// ===== Parallel Power Iteration for largest eigenvalue =====
void eigen_parallel(Matrix *m, double *eigenvalue, double *eigenvector) {
    if (m->rows != m->cols) {
        printf("Matrix must be square!\n");
        return;
    }

    int n = m->rows;
    double tol = 1e-6;
    int max_iter = 1000;
    
    // Initialize eigenvector with 1's
    for (int i = 0; i < n; i++)
        eigenvector[i] = 1.0;

    for (int iter = 0; iter < max_iter; iter++) {
        double new_vector[n];
        int pipefds[n][2];

        // Parallel multiply: each child computes one row of M * vector
        for (int i = 0; i < n; i++) {
            pipe(pipefds[i]);
            pid_t pid = fork();
            if (pid == 0) { // child
                close(pipefds[i][0]);
                double sum = 0;
                for (int j = 0; j < n; j++)
                    sum += m->data[i][j] * eigenvector[j];
                write(pipefds[i][1], &sum, sizeof(double));
                close(pipefds[i][1]);
                exit(0);
            } else {
                close(pipefds[i][1]);
            }
        }

        // Parent reads results
        for (int i = 0; i < n; i++) {
            read(pipefds[i][0], &new_vector[i], sizeof(double));
            close(pipefds[i][0]);
            wait(NULL);
        }

        // Normalize new_vector
        double norm = 0;
        for (int i = 0; i < n; i++)
            norm += new_vector[i] * new_vector[i];
        norm = sqrt(norm);
        for (int i = 0; i < n; i++)
            new_vector[i] /= norm;

        // Check convergence
        double diff = 0;
        for (int i = 0; i < n; i++)
            diff += fabs(new_vector[i] - eigenvector[i]);
        if (diff < tol) break;

        // Copy new_vector to eigenvector
        for (int i = 0; i < n; i++)
            eigenvector[i] = new_vector[i];
    }

    // Compute corresponding eigenvalue: lambda = (v^T * M * v) / (v^T * v)
    double num = 0, denom = 0;
    for (int i = 0; i < n; i++) {
        double mv_i = 0;
        for (int j = 0; j < n; j++)
            mv_i += m->data[i][j] * eigenvector[j];
        num += eigenvector[i] * mv_i;
        denom += eigenvector[i] * eigenvector[i];
    }
    *eigenvalue = num / denom;
}





