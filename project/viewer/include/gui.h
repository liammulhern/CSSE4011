#ifndef GUI_H_
#define GUI_H_

#define GUI_THREAD_STACK_SIZE 4096
#define GUI_THREAD_PRIORITY 5

int gui_init();

void gui_thread(void);

void gui_draw_localisation_position(double col, double row);

#endif // GUI_H_
