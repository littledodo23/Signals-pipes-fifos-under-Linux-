#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Config config;

void init_default_config(void) {
    config.worker_pool_size = 4;
    config.max_idle_time = 60;
    strcpy(config.matrix_directory, "");
    config.use_custom_menu = 0;
    
    for (int i = 0; i < 15; i++) {
        config.menu_order[i] = i + 1;
    }
}

void load_config(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        printf("[CONFIG] No config file found, using defaults\n");
        return;
    }
    
    char line[512];
    
    if (fgets(line, sizeof(line), f)) {
        sscanf(line, "%d %d", &config.worker_pool_size, &config.max_idle_time);
    }
    
    if (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = 0;
        strncpy(config.matrix_directory, line, sizeof(config.matrix_directory) - 1);
    }
    
    if (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "CUSTOM_MENU:", 12) == 0) {
            config.use_custom_menu = 1;
            char *token = strtok(line + 12, ",");
            int idx = 0;
            while (token != NULL && idx < 15) {
                config.menu_order[idx++] = atoi(token);
                token = strtok(NULL, ",");
            }
        }
    }
    
    fclose(f);
    
    printf("[CONFIG] Loaded successfully:\n");
    printf("  - Worker Pool Size: %d\n", config.worker_pool_size);
    printf("  - Max Idle Time: %d seconds\n", config.max_idle_time);
    if (strlen(config.matrix_directory) > 0) {
        printf("  - Matrix Directory: %s\n", config.matrix_directory);
    }
}

Config* get_config(void) {
    return &config;
}
