#include <threads_utils.h>
#include <ncurses_utils.h>
#include <uart_utils.h>
#include <float.h>
#include <string.h>
#include <bme_te.h>
#include <control_lcd_16x2.h>
#include <bcm2835.h>
#include <semaphore.h>

pthread_t thread_userinput;
pthread_t thread_nextvals;
pthread_t thread_gettitr;
pthread_t thread_gette;
pthread_t thread_getpot;

sem_t sem_nextvals;
sem_t sem_gettitr;
sem_t sem_gette;
sem_t sem_getpot;

int mode = 0; // 1 to potentiometer and 2 to keyboard input

int logIts = 0; // need to log in value 3 and reset to 0

double referencetemperature = DBL_MAX;
double histeresis = DBL_MAX;
double tempIn = DBL_MAX;
double tempEx = DBL_MAX;

int init_threads()
{
    if (pthread_create(&thread_userinput, NULL, watch_userinput, NULL))
    {
        return -1;
    }

    if (pthread_create(&thread_nextvals, NULL, watch_nextvals, NULL))
    {
        return -2;
    }
    sem_init(&sem_nextvals, 0, 0);

    if (pthread_create(&thread_gettitr, NULL, watch_gettitr, NULL))
    {
        return -3;
    }
    sem_init(&sem_gettitr, 0, 0);

    if (pthread_create(&thread_gette, NULL, watch_gette, NULL))
    {
        return -4;
    }
    sem_init(&sem_gette, 0, 0);

    if (pthread_create(&thread_getpot, NULL, watch_getpot, NULL))
    {
        return -5;
    }
    sem_init(&sem_getpot, 0, 0);

    return 0;
}

void finish_threads()
{
    pthread_cancel(thread_userinput);
    pthread_cancel(thread_nextvals);
    pthread_cancel(thread_gettitr);
    pthread_cancel(thread_gette);
    pthread_cancel(thread_getpot);
}

void *watch_userinput()
{
    int menuOption;
    while ((menuOption = getch()) != KEY_F(1))
    {
        if (menuOption == KEY_F(2))
        {
            mode = 1;
        }
        else if (menuOption == KEY_F(3))
        {
            mode = 2;

            get_input_temperature(&referencetemperature);
        }
        else if (menuOption == KEY_F(4))
        {
            get_input_histeresis(&histeresis);
        }
        else
        {
            continue;
        }

        update_messages(mode, referencetemperature, histeresis);
    }

    return 0;
}

void wait_to_finish()
{
    pthread_join(thread_userinput, NULL);
}

void calc_next_values()
{
    sem_post(&sem_nextvals);
}

void *watch_nextvals()
{
    while (1)
    {
        if (referencetemperature != DBL_MAX && histeresis != DBL_MAX)
        {
            // Semaphores
            sem_post(&sem_gettitr);
            sem_post(&sem_gette);

            char lcd_line1[16] = "";
            char lcd_line2[16] = "";

            char next_info[500] = "";
            if (tempIn != DBL_MAX)
            {
                char buff[100] = "";
                sprintf(buff, "TI: %.2lf ", tempIn);
                sprintf(lcd_line1, "TI%.2lf ", tempIn);
                strcat(next_info, buff);
            }
            else
            {
                strcat(next_info, "TI: fail ");
                sprintf(lcd_line1, "TI F ");
            }

            if (referencetemperature != DBL_MAX)
            {
                char buff[100] = "";
                sprintf(buff, "TR: %.2lf ", referencetemperature);
                sprintf(lcd_line2, "TR%.2lf", referencetemperature);
                strcat(next_info, buff);
            }
            else
            {
                strcat(next_info, "TR: fail ");
                sprintf(lcd_line2, "TR F");
            }

            if (tempEx != DBL_MAX)
            {
                char buff[100] = "";
                sprintf(buff, "TE: %.2lf", tempEx);
                char str_tmpex[16];
                sprintf(str_tmpex, "TE%.2lf", tempEx);
                strcat(next_info, buff);
                strcat(lcd_line1, str_tmpex);
            }
            else
            {
                strcat(next_info, "TE: fail");
                strcat(lcd_line1, "TE F");
            }

            print_sensors_data(next_info);

            // sprintf(lcd_line1, "TI%.2lf TE%.2lf", tempIn, tempEx);
            strcat(lcd_line1, "                ");
            lcdLoc(LINE1); // TI TE
            typeln(lcd_line1);

            strcat(lcd_line2, "                ");
            lcdLoc(LINE2); // TR
            typeln(lcd_line2);

            handle_vet_res();
        }
        else if (mode == 1)
        {
            sem_post(&sem_getpot);
        }

        if (logIts == 3)
        {
            if (tempIn != DBL_MAX && tempEx != DBL_MAX && referencetemperature != DBL_MAX)
            {
                // Open CSV file
                FILE *arq;
                arq = fopen("./dados.csv", "r+");
                if (arq)
                {
                    // If exists go to end
                    fseek(arq, 0, SEEK_END);
                }
                else
                {
                    // Oppen in append mode
                    arq = fopen("./dados.csv", "a");

                    // header
                    fprintf(arq, "Temperature Int (oC), Temperature Ex (oC), Temperature Re (oC), Data\n");
                }

                if (arq)
                {
                    // Last values every 2 secs
                    time_t rawtime;
                    struct tm *timeinfo;

                    time(&rawtime);
                    timeinfo = localtime(&rawtime);

                    fprintf(arq, "%0.2lf, %0.2lf, %0.2lf, %s", tempIn, tempEx, referencetemperature, asctime(timeinfo));
                }
                else
                {
                    printf("Nao foi possivel criar/abrir o arquivo em modo escrita. Permissoes?\n");
                    exit(-1);
                }

                // close CSV file
                fclose(arq);
            }
            logIts = -1;
        }
        logIts++;

        sem_wait(&sem_nextvals);
    }

    return 0;
}

void handle_vet_res()
{
    double oscValue = histeresis / 2;

    if (tempIn > referencetemperature + oscValue)
    {
        // Turn on vent
        bcm2835_gpio_write(RPI_V2_GPIO_P1_18, 0);
        // Turn off resistor
        bcm2835_gpio_write(RPI_V2_GPIO_P1_16, 1);
    }
    else if (tempIn < referencetemperature - oscValue)
    {
        // Turn off vent
        bcm2835_gpio_write(RPI_V2_GPIO_P1_18, 1);
        // Turn on resistor
        bcm2835_gpio_write(RPI_V2_GPIO_P1_16, 0);
    }
}

// void *watch_getti()
// {
//     // int rtrn_getdata =
//     getTi(&tempIn);
//     // if(rtrn_getdata){
//     //     tempIn = DBL_MAX;
//     // }
//     return 0;
// }

void *watch_gettitr()
{
    while (1)
    {
        sem_wait(&sem_gettitr);
        if (mode == 2)
        {
            getTi(&tempIn);
            continue;
        }
        // int rtrn_getdatas =
        getDatas(&tempIn, &referencetemperature);
        // if(rtrn_getdatas){
        //     tempIn = DBL_MAX;
        // }
    }
    return 0;
}

void *watch_gette()
{
    while (1)
    {
        sem_wait(&sem_gette);
        tempEx = (double)get_bme280_temperature();
    }
    return 0;
}

void *watch_getpot()
{
    while (1)
    {
        sem_wait(&sem_getpot);
        // int rtrn_getpot =
        getPot(&referencetemperature);
        // if(rtrn_getpot){
        //     referencetemperature = DBL_MAX;
        // }
        update_messages(mode, referencetemperature, histeresis);
    }

    return 0;
}