#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
    int worker_pool_size;
    int max_idle_time;
} Config;

void init_default_config(void);
void load_config(const char *filename);
Config* get_config(void);

#endif
