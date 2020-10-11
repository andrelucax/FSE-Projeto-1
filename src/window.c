#include <string.h>

#include <window.h>

WINDOW *create_newwin(int height, int width, int starty, int startx){
    WINDOW *local_win;

	local_win = newwin(height, width, starty, startx);

	box(local_win, 0, 0);

	wrefresh(local_win);

	return local_win;
}

void clear_window(WINDOW *window){
    wclear(window);

    box(window, 0, 0);

    wrefresh(window);
}

void msg_goodbye(){
    clear();
    char byeMsg[] = "Good bye! Have a nice day!";
    char exitMsg[] = "Press any button to exit";
    mvprintw(LINES / 2 - 1, COLS / 2 - strlen(byeMsg) / 2, byeMsg);
    mvprintw(LINES / 2, COLS / 2 - strlen(exitMsg) / 2, exitMsg);
}