#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "matrix.h"

Matrix *matrices[MAX_MATRICES];
int matrix_count = 0;

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

void enter_matrix() {
    if (matrix_count >= MAX_MATRICES) {
        printf("Memory full! Cannot store more matrices.\n");
        return;
    }

    char name[50];
    int rows, cols;
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
        printf("No matrices in memory.\n");
        return;
    }
    for (int i = 0; i < matrix_count; i++)
        printf("%d. %s (%dx%d)\n", i + 1, matrices[i]->name, matrices[i]->rows, matrices[i]->cols);
    int choice;
    printf("Enter number to display: ");
    scanf("%d", &choice);
    if (choice < 1 || choice > matrix_count) {
        printf("Invalid choice.\n");
        return;
    }
    print_matrix(matrices[choice - 1]);
}

void delete_matrix() {
    if (matrix_count == 0) {
        printf("No matrices to delete.\n");
        return;
    }
    for (int i = 0; i < matrix_count; i++)
        printf("%d. %s\n", i + 1, matrices[i]->name);
    int index;
    printf("Enter number of matrix to delete: ");
    scanf("%d", &index);
    if (index < 1 || index > matrix_count) {
        printf("Invalid choice.\n");
        return;
    }
    free_matrix(matrices[index - 1]);
    for (int i = index - 1; i < matrix_count - 1; i++)
        matrices[i] = matrices[i + 1];
    matrix_count--;
    printf("Matrix deleted successfully.\n");
}

void modify_matrix() {
    if (matrix_count == 0) {
        printf("No matrices to modify.\n");
        return;
    }
    for (int i = 0; i < matrix_count; i++)
        printf("%d. %s\n", i + 1, matrices[i]->name);
    int choice;
    printf("Choose a matrix: ");
    scanf("%d", &choice);
    if (choice < 1 || choice > matrix_count)
        return;

    Matrix *m = matrices[choice - 1];
    int mode;
    printf("1. Modify full row\n2. Modify full column\n3. Modify one value\nChoice: ");
    scanf("%d", &mode);

    if (mode == 1) {
        int row;
        printf("Enter row index (1-%d): ", m->rows);
        scanf("%d", &row);
        for (int j = 0; j < m->cols; j++) {
            printf("New value [%d][%d]: ", row, j + 1);
            scanf("%lf", &m->data[row - 1][j]);
        }
    } else if (mode == 2) {
        int col;
        printf("Enter column index (1-%d): ", m->cols);
        scanf("%d", &col);
        for (int i = 0; i < m->rows; i++) {
            printf("New value [%d][%d]: ", i + 1, col);
            scanf("%lf", &m->data[i][col - 1]);
        }
    } else if (mode == 3) {
        int r, c;
        printf("Enter row and column (e.g., 2 3): ");
        scanf("%d %d", &r, &c);
        printf("New value: ");
        scanf("%lf", &m->data[r - 1][c - 1]);
    }

    printf("Matrix updated.\n");
}

void display_all_matrices() {
    if (matrix_count == 0) {
        printf("No matrices in memory.\n");
        return;
    }
    for (int i = 0; i < matrix_count; i++)
        print_matrix(matrices[i]);
}

void read_matrix_from_file_option() {
    if (matrix_count >= MAX_MATRICES) {
        printf("Memory full! Cannot load more matrices.\n");
        return;
    }

    char filename[100];
    printf("Enter file path: ");
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
    printf("Enter filename to save: ");
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

double determinant_parallel(Matrix *m) {
    if (m->rows != m->cols) {
        printf("Matrix must be square!\n");
        return 0.0;
    }

    int n = m->rows;
    if (n == 1) return m->data[0][0];
    if (n == 2) return m->data[0][0] * m->data[1][1] - m->data[0][1] * m->data[1][0];

    double det = 0.0;
    for (int i = 0; i < n; i++) {
        Matrix *minor = create_matrix(n - 1, n - 1, "minor");
        for (int r = 1; r < n; r++) {
            int c_idx = 0;
            for (int c = 0; c < n; c++) {
                if (c == i) continue;
                minor->data[r - 1][c_idx++] = m->data[r][c];
            }
        }
        double sign = (i % 2 == 0) ? 1.0 : -1.0;
        det += sign * m->data[0][i] * determinant_parallel(minor);
        free_matrix(minor);
    }
    return det;
}

void eigen_parallel(Matrix *m, double *eigenvalue, double *eigenvector) {
    if (m->rows != m->cols) {
        printf("Matrix must be square!\n");
        return;
    }

    int n = m->rows;
    double tol = 1e-6;
    int max_iter = 1000;
    for (int i = 0; i < n; i++)
        eigenvector[i] = 1.0;

    for (int iter = 0; iter < max_iter; iter++) {
        double new_vector[n];
        for (int i = 0; i < n; i++) {
            double sum = 0;
            for (int j = 0; j < n; j++)
                sum += m->data[i][j] * eigenvector[j];
            new_vector[i] = sum;
        }

        double norm = 0;
        for (int i = 0; i < n; i++)
            norm += new_vector[i] * new_vector[i];
        norm = sqrt(norm);
        for (int i = 0; i < n; i++)
            new_vector[i] /= norm;

        double diff = 0;
        for (int i = 0; i < n; i++)
            diff += fabs(new_vector[i] - eigenvector[i]);
        if (diff < tol) break;

        for (int i = 0; i < n; i++)
            eigenvector[i] = new_vector[i];
    }

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
