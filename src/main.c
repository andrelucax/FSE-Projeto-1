#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <bcm2835.h>

#include <window.h>
#include <menu.h>
#include <uart_utils.h>
#include <linux_userspace.c>

pthread_t thread_userinput;
pthread_t thread_sensordata;
pthread_t thread_potenciometer;

int mode = 0;
float referencetemperature;
float potenciometerVal;
float g_histeresis;
bool canStart = false;
struct bme280_dev dev;
int8_t rslt = BME280_OK;
char str_histeresis[50] = "";
char str_referencetemperature[50] = "";

void *watch_userinput(void *args);
void *watch_sensordata(void *args);
void *watch_potenciometer(void *args);
void handleData(float tempWanted, float oscValue, float temp);

typedef struct inpWindows{
    WINDOW *inputWindow;
    WINDOW *messageWindow;
} inpWindows;

int main() {
    if (!bcm2835_init()){
        printf("Erro ao iniciar bcm2835\n");
        exit(1);
    };

    bcm2835_gpio_fsel(RPI_V2_GPIO_P1_18, BCM2835_GPIO_FSEL_OUTP);

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

    pthread_cancel(thread_sensordata);
    pthread_cancel(thread_potenciometer);

    msg_goodbye();
    
    delwin(menuWindow);
    delwin(sensorsDataWindow);
    delwin(inputWindow);
    delwin(messageWindow);

    getch(); // Wait input to exit

    endwin(); // Need to stop curses mode

    bcm2835_gpio_fsel(RPI_V2_GPIO_P1_18, BCM2835_GPIO_FSEL_OUTP);

    // Liga ventilador
    bcm2835_gpio_write(RPI_V2_GPIO_P1_18, 1);
    // Desliga resistor
    bcm2835_gpio_write(RPI_V2_GPIO_P1_16, 1);

    return 0;
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
        // Liga ventilador
        bcm2835_gpio_write(RPI_V2_GPIO_P1_18, 0);
        // Desliga resistor
        bcm2835_gpio_write(RPI_V2_GPIO_P1_16, 1);
    }
    else if(temp < tempWanted - oscValue){
        // Desliga ventilador
        bcm2835_gpio_write(RPI_V2_GPIO_P1_18, 1);
        // Liga resistor
        bcm2835_gpio_write(RPI_V2_GPIO_P1_16, 0);
    }

}