#include <ncurses.h>

WINDOW *create_newwin(int height, int width, int starty, int startx);
void msg_goodbye();
void clear_window(WINDOW *window);