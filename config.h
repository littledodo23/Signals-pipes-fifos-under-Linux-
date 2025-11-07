#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
    int worker_pool_size;
    int max_idle_time;
    char matrix_directory[256];      // ✅ NEW: Matrix loading directory
    int menu_order[15];               // ✅ NEW: Custom menu order (optional)
    int use_custom_menu;              // ✅ NEW: Flag for custom menu
} Config;

void init_default_config(void);
void load_config(const char *filename);
Config* get_config(void);

#endif
