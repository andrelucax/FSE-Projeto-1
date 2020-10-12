#include <menu.h>
#include <window.h>

void print_menuoptions(WINDOW *menuWindow){
    mvwprintw(menuWindow, 1, 1, "Option F1: exit program");
    mvwprintw(menuWindow, 3, 1, "Option F2: get reference temperature from potenciometer");
    mvwprintw(menuWindow, 4, 1, "Option F3: get reference temperature from keyboard input");
    mvwprintw(menuWindow, 5, 1, "Option F4: set histerese");

    wrefresh(menuWindow);
}

void show_message(WINDOW *messageWindow, char message1[], char message2[], char message3[]){
    clear_window(messageWindow);

    mvwprintw(messageWindow, 1, 1, message1);
    mvwprintw(messageWindow, 2, 1, message2);
    mvwprintw(messageWindow, 3, 1, message3);

    wrefresh(messageWindow);
}