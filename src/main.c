#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <pthread.h>

#include <window.h>
#include <menu.h>

void *watch_userinput(void *args);
void *watch_sensordata(void *args);

typedef struct inpWindows{
    WINDOW *inputWindow;
    WINDOW *messageWindow;
} inpWindows;

int main() {
    int row, col;

    initscr(); // Init curses mode
    noecho(); // Don't need to see user input
    cbreak(); // Disable line buffering, gimme every thing
    keypad(stdscr, TRUE); // Gimme that F button
    curs_set(0); // Hide cursor
	refresh();

    getmaxyx(stdscr, row, col);
    if(row < 24 || col < 120){
        endwin();
        
        printf("Screen too small, min rows = 24 and min columns = 120\n\rGot rows = %d and columns = %d\n\rPlease resize your screen\n", row, col);
        
        return -1;
    }
    
    WINDOW *menuWindow;
    WINDOW *inputWindow;
    WINDOW *messageWindow;
    WINDOW *sensorsDataWindow;

	menuWindow = create_newwin(LINES - 8, COLS / 2, 0, 0);
    inputWindow = create_newwin(3, COLS / 2, LINES - 3, 0);
    messageWindow = create_newwin(5, COLS / 2, LINES - 8, 0);
    sensorsDataWindow = create_newwin(LINES, COLS % 2 ? COLS / 2 + 1 : COLS / 2, 0, COLS / 2);

    print_menuoptions(menuWindow);

    show_message(messageWindow, "To start the app please select mode and histeresis", "To set mode: F2 or F3 (see options above)", "To set histeresis: F4");
    
    pthread_t thread_userinput;

    inpWindows inpWindow;
    inpWindow.inputWindow = inputWindow;
    inpWindow.messageWindow = messageWindow;

    if(pthread_create(&thread_userinput, NULL, watch_userinput, (void *) &inpWindow)){
        endwin();
        
        printf("Fail to create user input thread\n");
        
        return -2;
    }

    pthread_t thread_sensordata;

    if(pthread_create(&thread_sensordata, NULL, watch_sensordata, (void *) &sensorsDataWindow)){
        endwin();
        
        printf("Fail to create sensor data thread\n");
        
        return -3;
    }

    pthread_join(thread_userinput, NULL);
    pthread_join(thread_sensordata, NULL);

    msg_goodbye();
    
    delwin(menuWindow);
    delwin(sensorsDataWindow);
    delwin(inputWindow);
    delwin(messageWindow);

    getch(); // Wait input to exit

    endwin(); // Need to stop curses mode
    return 0;
}

void *watch_userinput(void *args){
    inpWindows *inpWindow = (inpWindows*) args;
    WINDOW *inputWindow = inpWindow->inputWindow;
    WINDOW *messageWindow = inpWindow->messageWindow;

    int menuOption;
    char str_histeresis[50] = "";
    char str_referencetemperature[50] = "";
    while((menuOption = getch()) != KEY_F(1)){
        if(menuOption == KEY_F(2)){
            show_message(messageWindow, "Type: potenciometer", "Reference temperature: 2.2", "Not implemented yet");
        }
        else if(menuOption == KEY_F(3)){
            float new_referencetemperature;

            echo();

            clear_window(inputWindow);

            mvwprintw(inputWindow, 1, 1, "Type new reference temperature > ");

            wscanw(inputWindow, "%f", &new_referencetemperature);

            noecho();

            clear_window(inputWindow);

            strcpy(str_referencetemperature, "Reference temperature: ");
            char buff[20];

            gcvt(new_referencetemperature, 20, buff);
            strcat(str_referencetemperature, buff);
            strcat(str_referencetemperature, " oC");

            if(strcmp(str_histeresis, "")){
                show_message(messageWindow, "Type: keyboard input", str_referencetemperature, str_histeresis);
            }
        }
        else if(menuOption == KEY_F(4)){
            float new_histeresis;

            echo();

            clear_window(inputWindow);

            mvwprintw(inputWindow, 1, 1, "Type new histeresis temperature > ");

            wscanw(inputWindow, "%f", &new_histeresis);

            noecho();

            clear_window(inputWindow);

            strcpy(str_histeresis, "Histeresis: ");
            char buff[20];

            gcvt(new_histeresis, 20, buff);
            strcat(str_histeresis, buff);
            strcat(str_histeresis, " oC");

            if(strcmp(str_referencetemperature, "")){
                show_message(messageWindow, "Type: keyboard input", str_referencetemperature, str_histeresis);
            }
        }
    }

    return 0;
}

void *watch_sensordata(void *args){

    return 0;
}