#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <bcm2835.h>
#include <time.h>
#include<signal.h>

#include <window.h>
#include <menu.h>
#include <uart_utils.h>
#include <linux_userspace.c>
#include <control_lcd_16x2.h>

// Threads
pthread_t thread_userinput;
pthread_t thread_sensordata;
pthread_t thread_potenciometer;

int mode = 0; // Potenciometer or keyboard input
int logIts = 0; // need to log in value 3 and reset to 0

float referencetemperature;
float tempIn;
float tempEx;
float potenciometerVal;
float g_histeresis;

bool canStart = false;

struct bme280_dev dev;
int8_t rslt = BME280_OK;

// Strings
char str_histeresis[50] = "";
char str_referencetemperature[50] = "";

void onExit(bool waitUI);
void sigHandler(int signal);

// Threads functions
void *watch_userinput(void *args);
void *watch_sensordata(void *args);
void *watch_potenciometer(void *args);
void handleData(float tempWanted, float oscValue, float temp);

// To pass 2 args to userInputThread
typedef struct inpWindows{
    WINDOW *inputWindow;
    WINDOW *messageWindow;
} inpWindows;

int main() {
    signal(SIGKILL, sigHandler);
    signal(SIGSTOP, sigHandler);
    signal(SIGINT, sigHandler);
    signal(SIGTERM, sigHandler);

    if (!bcm2835_init()){
        printf("Error on bcm2835\n");
        exit(1);
    };

    bcm2835_gpio_fsel(RPI_V2_GPIO_P1_18, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(RPI_V2_GPIO_P1_16, BCM2835_GPIO_FSEL_OUTP);

    lcd_init();

    struct identifier id;

    char path[] = "/dev/i2c-1";

    if ((id.fd = open(path, O_RDWR)) < 0)
    {
        fprintf(stderr, "Failed to open the i2c bus %s\n", path);
        exit(2);
    }

    id.dev_addr = BME280_I2C_ADDR_PRIM;

    if (ioctl(id.fd, I2C_SLAVE, id.dev_addr) < 0)
    {
        fprintf(stderr, "Failed to acquire bus access and/or talk to slave.\n");
        exit(3);
    }

    dev.intf = BME280_I2C_INTF;
    dev.read = user_i2c_read;
    dev.write = user_i2c_write;
    dev.delay_us = user_delay_us;

    dev.intf_ptr = &id;

    rslt = bme280_init(&dev);
    if (rslt != BME280_OK)
    {
        fprintf(stderr, "Failed to initialize the device (code %+d).\n", rslt);
        exit(4);
    }

    int row, col;

    initscr(); // Init curses mode
    noecho(); // Don't need to see user input
    cbreak(); // Disable line buffering, gimme every thing
    keypad(stdscr, TRUE); // Gimme that spicy F button
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

    inpWindows inpWindow;
    inpWindow.inputWindow = inputWindow;
    inpWindow.messageWindow = messageWindow;

    if(pthread_create(&thread_userinput, NULL, watch_userinput, (void *) &inpWindow)){
        endwin();
        
        printf("Fail to create user input thread\n");
        
        return -2;
    }    

    if(pthread_create(&thread_sensordata, NULL, watch_sensordata, (void *) sensorsDataWindow)){
        endwin();
        
        printf("Fail to create sensor data thread\n");
        
        return -3;
    }

    if(pthread_create(&thread_potenciometer, NULL, watch_potenciometer, (void *) messageWindow)){
        endwin();
        
        printf("Fail to create potenciometer thread\n");
        
        return -4;
    }

    pthread_join(thread_userinput, NULL);

    delwin(menuWindow);
    delwin(sensorsDataWindow);
    delwin(inputWindow);
    delwin(messageWindow);

    onExit(true);

    return 0;
}

void onExit(bool waitUI){
    pthread_cancel(thread_userinput);
    pthread_cancel(thread_sensordata);
    pthread_cancel(thread_potenciometer);

    // Turn off vent
    bcm2835_gpio_write(RPI_V2_GPIO_P1_18, 1);
    // Turn off res
    bcm2835_gpio_write(RPI_V2_GPIO_P1_16, 1);

    ClrLcd();

    if(waitUI){
        msg_goodbye();

        lcdLoc(LINE1);
        typeln("   Good Bye!   ");

        lcdLoc(LINE2);
        typeln("Have a nice day");
        getch(); // Wait input to exit
    }

    endwin(); // Need to stop curses mode or you will be cursed

    exit(0);
}

void sigHandler(int signal){
    onExit(false);
}

void *watch_userinput(void *args){
    inpWindows *inpWindow = (inpWindows*) args;
    WINDOW *inputWindow = inpWindow->inputWindow;
    WINDOW *messageWindow = inpWindow->messageWindow;

    int menuOption;
    while((menuOption = getch()) != KEY_F(1)){
        if(menuOption == KEY_F(2)){
            mode = 1;
        }
        else if(menuOption == KEY_F(3)){
            mode = 2;
            float new_referencetemperature;

            echo();

            clear_window(inputWindow);

            mvwprintw(inputWindow, 1, 1, "Type new reference temperature > ");

            wscanw(inputWindow, "%f", &new_referencetemperature);
            referencetemperature = new_referencetemperature;

            noecho();

            clear_window(inputWindow);

            strcpy(str_referencetemperature, "Reference temperature: ");
            char buff[20] = "";
            sprintf(buff, "%.2f", referencetemperature);

            strcat(str_referencetemperature, buff);
            strcat(str_referencetemperature, " oC");

            if(strcmp(str_histeresis, "")){
                canStart = true;
                show_message(messageWindow, "Type: keyboard input", str_referencetemperature, str_histeresis);
            }
        }
        else if(menuOption == KEY_F(4)){
            float new_histeresis;

            echo();

            clear_window(inputWindow);

            mvwprintw(inputWindow, 1, 1, "Type new histeresis temperature > ");

            wscanw(inputWindow, "%f", &new_histeresis);
            g_histeresis = new_histeresis;

            noecho();

            clear_window(inputWindow);

            strcpy(str_histeresis, "Histeresis: ");
            char buff[20] = "";
            sprintf(buff, "%.2f", new_histeresis);

            strcat(str_histeresis, buff);
            strcat(str_histeresis, " oC");

            if(strcmp(str_referencetemperature, "")){
                canStart = true;
                show_message(messageWindow, "Type: keyboard input", str_referencetemperature, str_histeresis);
            }
        }
    }

    return 0;
}

void *watch_sensordata(void *args){
    WINDOW *sensorsDataWindow = (WINDOW*) args;

    char empty_line[1000] = "";

    for(int i = 0; i < (COLS % 2 ? COLS / 2 - 1  : COLS / 2 - 2); i++){
        empty_line[i] = ' ';
    }

    for(int i = 1, j = 1; ; i++){
        if(canStart){
            char str_printallsensors[100] = "TR: ";

            char buff[20] = "";
            sprintf(buff, "%.2f", referencetemperature);

            strcat(str_printallsensors, buff);
            strcat(str_printallsensors, " oC TI: ");

            char buff1[20] = "";

            float res1;
            float res2;
            getDatas(&res1, &res2);

            sprintf(buff1, "%.2f", res1);
            tempIn = res1;
            potenciometerVal = res2;

            strcat(str_printallsensors, buff1);
            strcat(str_printallsensors, " oC TE: ");

            float _temp;
            rslt = stream_sensor_data_forced_mode(&dev, &_temp);
            if (rslt != BME280_OK)
            {
                // fprintf(stderr, "Failed to stream sensor data (code %+d).\n", rslt);
                exit(1);
            }

            char buff2[20] = "";
            sprintf(buff2, "%.2f", _temp);
            tempEx = _temp;

            strcat(str_printallsensors, buff2);
            strcat(str_printallsensors, " oC");

            mvwprintw(sensorsDataWindow, i, j, empty_line);
            mvwprintw(sensorsDataWindow, i, j, str_printallsensors);
            wrefresh(sensorsDataWindow);

            handleData(referencetemperature, g_histeresis, res1);
        }
        else{
            i = 0;
            sleep(1);
            continue;
        }
        
        if(i == LINES - 2){
            i = 0;
        }
        else{
            mvwprintw(sensorsDataWindow, i+1, j, empty_line);
            wrefresh(sensorsDataWindow);
        }

    }

    return 0;
}

void *watch_potenciometer(void *args){
    WINDOW *messageWindow = (WINDOW*) args;

    while(1){
        if(mode != 1){
            sleep(1);
            continue;
        }
        usleep(500000);
        referencetemperature = potenciometerVal;
        strcpy(str_referencetemperature, "Reference temperature: ");
        char buff[20] = "";
        sprintf(buff, "%.2f", referencetemperature);

        strcat(str_referencetemperature, buff);
        strcat(str_referencetemperature, " oC");

        if(strcmp(str_histeresis, "")){
            canStart = true;
            show_message(messageWindow, "Type: potenciometer", str_referencetemperature, str_histeresis);
        }
    }
}

void handleData(float tempWanted, float oscValue, float temp){
    oscValue /= 2;

    if(temp > tempWanted + oscValue){
        // Turn on vent
        bcm2835_gpio_write(RPI_V2_GPIO_P1_18, 0);
        // Turn off resistor
        bcm2835_gpio_write(RPI_V2_GPIO_P1_16, 1);
    }
    else if(temp < tempWanted - oscValue){
        // Turn off vent
        bcm2835_gpio_write(RPI_V2_GPIO_P1_18, 1);
        // Turn on resistor
        bcm2835_gpio_write(RPI_V2_GPIO_P1_16, 0);
    }

    if(logIts == 3){
        logIts = 0;
        // Open CSV file
        FILE *arq;
        arq = fopen("./dados.csv", "r+");
        if(arq){
            // If exists go to end
            fseek(arq, 0, SEEK_END);
        }
        else{
            // Oppen in append mode
            arq = fopen("./dados.csv", "a");

            // header
            fprintf(arq, "Temperature Int (oC), Temperature Ex (oC), Temperature Re (oC), Data\n");
        }

        if(arq){
            // Last values every 2 secs
            time_t rawtime;
            struct tm * timeinfo;

            time(&rawtime);
            timeinfo = localtime(&rawtime);

            fprintf(arq, "%0.2lf, %0.2lf, %0.2lf, %s", tempIn, tempEx, referencetemperature, asctime (timeinfo));
        }
        else{
            printf("Nao foi possivel criar/abrir o arquivo em modo escrita. Permissoes?\n");
            exit(-1);
        }

        // close CSV file
        fclose(arq);
    }
    logIts++;

    lcdLoc(LINE1);
    char lineLCD1[16] = "";
    sprintf(lineLCD1, "TI%.2f TE%.2f", tempIn, tempEx);
    typeln(lineLCD1);

    lcdLoc(LINE2);
    char lineLCD2[16] = "";
    sprintf(lineLCD2, "TR %.2f", referencetemperature);
    typeln(lineLCD2);
}