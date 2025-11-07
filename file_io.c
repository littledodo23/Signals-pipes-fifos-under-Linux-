#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include "matrix.h"
#include "file_io.h"

#ifdef _WIN32
#include <direct.h>  // for _mkdir, _getcwd
#define MKDIR(dir) _mkdir(dir)
#define GETCWD _getcwd
#else
#include <unistd.h>  // for getcwd, mkdir
#define MKDIR(dir) mkdir(dir, 0777)
#define GETCWD getcwd
#endif

// ===============================
// Helper: print current directory
// ===============================
void print_cwd_debug() {
    char cwd[512];
    if (GETCWD(cwd, sizeof(cwd)) != NULL)
        printf("[DEBUG] Current working directory: %s\n", cwd);
    else
        perror("[DEBUG] getcwd() failed");
}

// ===============================
// Read a single matrix from file
// ===============================
Matrix *read_matrix_from_file(const char *filename) {
    print_cwd_debug();

    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("[ERROR] Opening file for reading failed");
        return NULL;
    }

    char name[50];
    int rows, cols;
    if (fscanf(fp, "%s %d %d", name, &rows, &cols) != 3) {
        fprintf(stderr, "[ERROR] Invalid file format in %s\n", filename);
        fclose(fp);
        return NULL;
    }

    Matrix *m = create_matrix(rows, cols, name);
    if (!m) {
        fprintf(stderr, "[ERROR] Memory allocation failed for matrix %s\n", name);
        fclose(fp);
        return NULL;
    }

    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++)
            fscanf(fp, "%lf", &m->data[i][j]);

    fclose(fp);
    printf("✅ Matrix '%s' loaded successfully from %s\n", m->name, filename);
    return m;
}

// ===============================
// Save a single matrix to file
// ===============================
void save_matrix_to_file(Matrix *m, const char *filename) {
 //   printf("\n[DEBUG] Trying to save matrix '%s' to file: %s\n", m->name, filename);

    FILE *fp = fopen(filename, "w");
    if (!fp) {
        perror("[ERROR] Could not open file for writing");
        return;
    }

    int header = fprintf(fp, "%s %d %d\n", m->name, m->rows, m->cols);

    int total_written = 0;
    for (int i = 0; i < m->rows; i++) {
        for (int j = 0; j < m->cols; j++) {
            int n = fprintf(fp, "%.2lf ", m->data[i][j]);
            total_written += n;
        }
        fprintf(fp, "\n");
    }



    if (fclose(fp) == 0)
        printf(" File '%s' closed successfully.\n", filename);
    else
        perror("[ERROR] fclose() failed");

    printf(" Matrix '%s' saved to %s\n", m->name, filename);
}



// ========================================
// Read all .txt matrices from a folder
// ========================================
void read_matrices_from_folder(const char *foldername) {
    DIR *dir = opendir(foldername);
    if (!dir) {
        perror("[ERROR] Opening folder failed");
        return;
    }

    struct dirent *entry;
    int count = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (strstr(entry->d_name, ".txt")) {
            char path[256];
            snprintf(path, sizeof(path), "%s/%s", foldername, entry->d_name);
            Matrix *m = read_matrix_from_file(path);
            if (m && matrix_count < MAX_MATRICES) {
                matrices[matrix_count++] = m;
                count++;
            }
        }
    }

    closedir(dir);
    printf("✅ %d matrices loaded from folder: %s\n", count, foldername);
}

// ==========================================
// Save all matrices in memory to a folder
// ==========================================
void save_all_matrices_to_folder(const char *foldername) {
    // Try to create the folder (ignore if exists)
    if (MKDIR(foldername) == 0)
        printf("[DEBUG] Folder '%s' created.\n", foldername);
    else if (errno == EEXIST)
        printf("[DEBUG] Folder '%s' already exists.\n", foldername);
    else
        perror("[WARNING] Could not create folder (may still exist)");

    for (int i = 0; i < matrix_count; i++) {
        char filename[256];
        snprintf(filename, sizeof(filename), "%s/%s.txt", foldername, matrices[i]->name);
        save_matrix_to_file(matrices[i], filename);
    }

    printf("✅ All matrices saved to folder: %s\n", foldername);
}

// =================================
// Menu helper wrappers
// =================================

void read_matrix_from_file_option() {
    char filename[100];
    printf("Enter filename: ");
    scanf("%99s", filename);
    Matrix *m = read_matrix_from_file(filename);
    if (m && matrix_count < MAX_MATRICES)
        matrices[matrix_count++] = m;
}

void read_matrices_from_folder_option() {
    char folder[100];
    printf("Enter folder path: ");
    scanf("%99s", folder);
    read_matrices_from_folder(folder);
}

void save_matrix_to_file_option() {
    if (matrix_count == 0) {
        printf("⚠️ No matrices to save.\n");
        return;
    }

    for (int i = 0; i < matrix_count; i++)
        printf("%d. %s\n", i + 1, matrices[i]->name);

    int ch;
    printf("Choose matrix: ");
    scanf("%d", &ch);

    if (ch < 1 || ch > matrix_count) {
        printf("Invalid selection.\n");
        return;
    }

    char filename[100];
    printf("Enter filename: ");
    scanf("%99s", filename);

    save_matrix_to_file(matrices[ch - 1], filename);
}

void save_all_matrices_to_folder_option() {
    char folder[100];
    printf("Enter folder name: ");
    scanf("%99s", folder);
    save_all_matrices_to_folder(folder);
}
