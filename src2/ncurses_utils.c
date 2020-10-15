#include <ncurses_utils.h>
#include <string.h>
#include <float.h>

WINDOW *menuWindow;
WINDOW *menuBoxWindow;

WINDOW *inputWindow;
WINDOW *inputBoxWindow;

WINDOW *messageWindow;
WINDOW *messageBoxWindow;

WINDOW *sensorsDataWindow;
WINDOW *sensorsDataBoxWindow;

int nextLineSensorsData = 0;

int init_screens()
{
    initscr();            // Init curses mode
    noecho();             // Don't need to see user input
    cbreak();             // Disable line buffering, gimme every thing
    keypad(stdscr, TRUE); // Gimme that spicy F button
    curs_set(0);          // Hide cursor
    refresh();

    int row, col;
    getmaxyx(stdscr, row, col);
    if (row < 24 || col < 120)
    {
        endwin();

        printf("Screen too small, min rows = 24 and min columns = 120\n\rGot rows = %d and columns = %d\n\rPlease resize your screen\n", row, col);

        return -1;
    }

    menuBoxWindow = create_newwin(LINES - 8, COLS / 2, 0, 0, true);
    menuWindow = create_newwin(LINES - 8, COLS / 2, 0, 0, false);

    inputBoxWindow = create_newwin(3, COLS / 2, LINES - 3, 0, true);
    inputWindow = create_newwin(3, COLS / 2, LINES - 3, 0, false);

    messageBoxWindow = create_newwin(5, COLS / 2, LINES - 8, 0, true);
    messageWindow = create_newwin(5, COLS / 2, LINES - 8, 0, false);

    sensorsDataBoxWindow = create_newwin(LINES, COLS % 2 ? COLS / 2 + 1 : COLS / 2, 0, COLS / 2, true);
    sensorsDataWindow = create_newwin(LINES, COLS % 2 ? COLS / 2 + 1 : COLS / 2, 0, COLS / 2, false);

    print_menu();
    print_messages("To start the app please select mode and histeresis", "* To set mode: F2 or F3 (see options above)", "* To set histeresis: F4");

    return 0;
}

void finish_screens(bool goodbye)
{
    delwin(menuBoxWindow);
    delwin(inputBoxWindow);
    delwin(messageBoxWindow);
    delwin(sensorsDataBoxWindow);

    delwin(menuWindow);
    delwin(inputWindow);
    delwin(messageWindow);
    delwin(sensorsDataWindow);

    if (goodbye)
    {
        messageGoodBye();
        getch();
    }
    endwin();
}

WINDOW *create_newwin(int height, int width, int starty, int startx, bool bbox)
{
    if (!bbox)
    {
        height -= 2;
        width -= 2;
        starty += 1;
        startx += 1;
    }

    WINDOW *local_win;

    local_win = newwin(height, width, starty, startx);

    if (bbox)
    {
        box(local_win, 0, 0);
    }

    wrefresh(local_win);

    return local_win;
}

void messageGoodBye()
{
    clear();
    char byeMsg[] = "Good bye! Have a nice day!";
    char exitMsg[] = "Press any button to exit";
    mvprintw(LINES / 2 - 1, COLS / 2 - strlen(byeMsg) / 2, byeMsg);
    mvprintw(LINES / 2, COLS / 2 - strlen(exitMsg) / 2, exitMsg);
}

void get_input_temperature(double *new_referencetemperature)
{
    echo();

    wclear(inputWindow);

    mvwprintw(inputWindow, 0, 0, "Type new reference temperature > ");

    wscanw(inputWindow, "%lf", new_referencetemperature);

    noecho();

    wclear(inputWindow);

    wrefresh(inputWindow);
}

void get_input_histeresis(double *new_histeresis)
{
    echo();

    wclear(inputWindow);

    mvwprintw(inputWindow, 0, 0, "Type new histeresis temperature > ");

    wscanw(inputWindow, "%lf", new_histeresis);

    noecho();

    wclear(inputWindow);

    wrefresh(inputWindow);
}

void print_menu()
{
    wclear(menuWindow);

    mvwprintw(menuWindow, 0, 0, "Option F1: exit program");
    mvwprintw(menuWindow, 2, 0, "Option F2: get reference temperature from potenciometer");
    mvwprintw(menuWindow, 3, 0, "Option F3: get reference temperature from keyboard input");
    mvwprintw(menuWindow, 4, 0, "Option F4: set histerese");

    wrefresh(menuWindow);
}

void print_messages(char message1[], char message2[], char message3[])
{
    wclear(messageWindow);

    mvwprintw(messageWindow, 0, 0, message1);
    mvwprintw(messageWindow, 1, 0, message2);
    mvwprintw(messageWindow, 2, 0, message3);

    wrefresh(messageWindow);
}

void update_messages(int mode, double referencetemperature, double histeresis)
{
    char str_referencetemperature[500] = "* To set mode: F2 or F3 (see options above)";
    char str_histeresis[500] = "* To set histeresis: F4";

    if (referencetemperature != DBL_MAX)
    {
        char newstr_reference_temperature[500] = "";
        sprintf(newstr_reference_temperature, "Reference temperature: %.2lf", referencetemperature);
        strcpy(str_referencetemperature, newstr_reference_temperature);
    }
    if (histeresis != DBL_MAX)
    {
        char newstr_histeresis[500] = "";
        sprintf(newstr_histeresis, "Histeresis: %.2lf", histeresis);
        strcpy(str_histeresis, newstr_histeresis);
    }

    if (mode == 0)
    {
        print_messages("To start the app please select mode and histeresis", str_referencetemperature, str_histeresis);
    }
    else if (mode == 1)
    {
        print_messages("Type: potenciometer", str_referencetemperature, str_histeresis);
    }
    else if (mode == 2)
    {
        print_messages("Type: keyboard input", str_referencetemperature, str_histeresis);
    }
}

void print_sensors_data(char str_to_print[])
{
    wmove(sensorsDataWindow, nextLineSensorsData, 0);
    wclrtoeol(sensorsDataWindow);
    wrefresh(sensorsDataWindow);

    mvwprintw(sensorsDataWindow, nextLineSensorsData, 0, str_to_print);
    wrefresh(sensorsDataWindow);

    if (nextLineSensorsData == LINES - 2)
    {
        nextLineSensorsData = 0;
        wmove(sensorsDataWindow, nextLineSensorsData, 0);
        wclrtoeol(sensorsDataWindow);
        wrefresh(sensorsDataWindow);
    }
    else
    {
        wmove(sensorsDataWindow, nextLineSensorsData + 1, 0);
        wclrtoeol(sensorsDataWindow);
        wrefresh(sensorsDataWindow);
        nextLineSensorsData++;
    }
}