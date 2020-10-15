#ifndef _NCURSES_UTILS_H_
#define _NCURSES_UTILS_H_

#include <ncurses.h>

int init_screens();
void finish_screens(bool goodbye);
WINDOW *create_newwin(int height, int width, int starty, int startx, bool bbox);
void messageGoodBye();
void get_input_temperature(double *new_referencetemperature);
void get_input_histeresis(double *new_histeresis);
void print_menu();
void print_messages(char message1[], char message2[], char message3[]);
void update_messages(int mode, double referencetemperature, double histeresis);
void print_sensors_data(char str_to_print[]);

#endif /* _NCURSES_UTILS_H_ */