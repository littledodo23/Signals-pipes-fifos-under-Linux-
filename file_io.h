#ifndef FILE_IO_H
#define FILE_IO_H

#include "matrix.h"

// ✅ FIXED: Added all missing declarations
Matrix *read_matrix_from_file(const char *filename);
void save_matrix_to_file(Matrix *m, const char *filename);
void read_matrices_from_folder(const char *foldername);
void save_all_matrices_to_folder(const char *foldername);
void load_matrices_from_file(const char *filename);

// Menu wrapper functions
void read_matrix_from_file_option(void);
void read_matrices_from_folder_option(void);
void save_matrix_to_file_option(void);
void save_all_matrices_to_folder_option(void);

// Helper
void print_cwd_debug(void);

#endif
