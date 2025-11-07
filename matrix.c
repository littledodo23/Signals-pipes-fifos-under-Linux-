#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "matrix.h"

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

void load_matrices_from_file(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        printf("Failed to open file: %s\n", filename);
        return;
    }

    while (!feof(fp)) {
        char name[50];
        int rows, cols;
        if (fscanf(fp, "%s %d %d", name, &rows, &cols) != 3) break;

        Matrix *m = create_matrix(rows, cols, name);
        for (int i = 0; i < rows; i++)
            for (int j = 0; j < cols; j++)
                fscanf(fp, "%lf", &m->data[i][j]);

        if (matrix_count < MAX_MATRICES)
            matrices[matrix_count++] = m;

        // Skip empty line
        int c;
        while ((c = fgetc(fp)) != EOF && c != '\n');
    }

    fclose(fp);
}

// ===== Menu Options =====

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

    printf("Available matrices:\n");
    for (int i = 0; i < matrix_count; i++)
        printf("%d. %s (%dx%d)\n", i + 1, matrices[i]->name,
               matrices[i]->rows, matrices[i]->cols);

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

    printf("Matrices in memory:\n");
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
    if (choice < 1 || choice > matrix_count) return;

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
