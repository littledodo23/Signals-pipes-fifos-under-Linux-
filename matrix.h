#ifndef MATRIX_H
#define MATRIX_H

// ===== Matrix Structure =====
typedef struct {
    char name[50];
    int rows;
    int cols;
    double **data;
} Matrix;

// ===== Global Storage =====
#define MAX_MATRICES 50
extern Matrix *matrices[MAX_MATRICES];
extern int matrix_count;

// ===== Basic Matrix Functions =====
Matrix *create_matrix(int rows, int cols, const char *name);
void free_matrix(Matrix *m);
void print_matrix(Matrix *m);

// ===== File Operations =====
Matrix *read_matrix_from_file(const char *filename);
void save_matrix_to_file(Matrix *m, const char *filename);
void read_matrices_from_folder(const char *foldername);
void save_all_matrices_to_folder(const char *foldername);

// ===== Menu Operations (1–9) =====
void enter_matrix();
void display_matrix();
void delete_matrix();
void modify_matrix();
void read_matrix_from_file_option();
void read_matrices_from_folder_option();
void save_matrix_to_file_option();
void save_all_matrices_to_folder_option();
void display_all_matrices();

#endif
