#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>
void keyboard_handler(void);

extern int is_script_running;
extern volatile char last_pressed_key;
char get_last_key(void);

#define KEY_ESC   27
#define KEY_UP    17
#define KEY_DOWN  18
#define KEY_LEFT  19
#define KEY_RIGHT 20

extern int is_editor_active;

#endif /* KEYBOARD_H */
