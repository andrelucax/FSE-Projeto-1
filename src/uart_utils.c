#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>          //Used for UART
#include <termios.h>        //Used for UART

#include <uart_utils.h>

int startUart(){
    int uart0_filestream = -1;
    uart0_filestream = open("/dev/serial0", O_RDWR | O_NOCTTY | O_NDELAY);      //Open in non blocking read/write mode
    if (uart0_filestream == -1)
    {
        return uart0_filestream;
    }

    struct termios options;
    tcgetattr(uart0_filestream, &options);
    options.c_cflag = B115200 | CS8 | CLOCAL | CREAD;     //<Set baud rate
    options.c_iflag = IGNPAR;
    options.c_oflag = 0;
    options.c_lflag = 0;
    tcflush(uart0_filestream, TCIFLUSH);
    tcsetattr(uart0_filestream, TCSANOW, &options);

    return uart0_filestream;
}

void getDatas(float *res1, float *res2){
    int uart = startUart();

    if (uart != -1){
        char buffer[] = {0xA1, 8, 2, 5, 1};

        int res = write(uart, &buffer, 5);
        if (res < 0)
        {
            // printf("UART write error\n");
            close(uart);
            return;
        }
        
        usleep(200000);

        res = read(uart, (void*) res1, 4);
        if (res < 0)
        {
            close(uart);
            // printf("UART read error\n");
            return;
        }

        char buffer1[] = {0xA2, 8, 2, 5, 1};

        int ress = write(uart, &buffer1, 5);
        if (ress < 0)
        {
            // printf("UART write error\n");
            close(uart);
            return;
        }
        
        usleep(200000);

        ress = read(uart, (void*) res2, 4);
        close(uart);
        if (ress < 0)
        {
            // printf("UART read error\n");
            return;
        }
    }

    return;
}

float getPotenciometer(){
    int uart = startUart();
    float resVal = -1.0;

    if (uart != -1){
        char buffer[] = {0xA2, 8, 2, 5, 1};

        int res = write(uart, &buffer, 5);
        if (res < 0)
        {
            // printf("UART write error\n");
            close(uart);
            return resVal;
        }
        
        usleep(400000);

        res = read(uart, (void*)&resVal, 4);
        close(uart);
        if (res < 0)
        {
            // printf("UART read error\n");
            return resVal;
        }
    }

    return resVal;
}