#include "config.h"
#include <stdio.h>

static Config config;

void init_default_config(void) {
    config.worker_pool_size = 4;
    config.max_idle_time = 60;
}

void load_config(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) return; // use defaults if file not found
    fscanf(f, "%d %d", &config.worker_pool_size, &config.max_idle_time);
    fclose(f);
}

Config* get_config(void) {
    return &config;
}
