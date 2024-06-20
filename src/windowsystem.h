#ifndef WINDOWSYSTEM_H
#define WINDOWSYSTEM_H

#include <stdbool.h>

int init_window_system(void);
bool update_active_window(void);
char *get_window_title(void);
char *get_window_name(void);
char *get_window_instance(void);

#endif

