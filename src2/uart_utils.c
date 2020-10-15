#include <uart_utils.h>
#include <fcntl.h>
#include <termios.h>

int startUart()
{
    int uart0_filestream = -1;
    uart0_filestream = open("/dev/serial0", O_RDWR | O_NOCTTY | O_NDELAY); //Open in non blocking read/write mode
    if (uart0_filestream == -1)
    {
        return uart0_filestream;
    }

    struct termios options;
    tcgetattr(uart0_filestream, &options);
    options.c_cflag = B115200 | CS8 | CLOCAL | CREAD; //<Set baud rate
    options.c_iflag = IGNPAR;
    options.c_oflag = 0;
    options.c_lflag = 0;
    tcflush(uart0_filestream, TCIFLUSH);
    tcsetattr(uart0_filestream, TCSANOW, &options);

    return uart0_filestream;
}

int getDatas(double *ti, double *pot)
{
    int uart = startUart();
    if (uart == -1)
    {
        return uart;
    }

    char buffer[] = {0xA1, 8, 2, 5, 1};

    int res = write(uart, &buffer, 5);
    if (res < 0)
    {
        close(uart);
        return -2;
    }

    usleep(100000);

    float ti4b;
    res = read(uart, (void *)&ti4b, 4);
    if (res < 0)
    {
        close(uart);
        return -3;
    }
    *ti = (double) ti4b;

    char buffer1[] = {0xA2, 8, 2, 5, 1};

    int ress = write(uart, &buffer1, 5);
    if (ress < 0)
    {
        close(uart);
        return -4;
    }

    usleep(100000);

    float pot4b;
    ress = read(uart, (void *)&pot4b, 4);
    close(uart);
    if (ress < 0)
    {
        return -5;
    }
    *pot = (double) pot4b;

    return 0;
}

int getTi(double *ti)
{
    int uart = startUart();
    if (uart == -1)
    {
        return uart;
    }

    char buffer[] = {0xA1, 8, 2, 5, 1};

    int res = write(uart, &buffer, 5);
    if (res < 0)
    {
        close(uart);
        return -2;
    }

    usleep(100000);

    float ti4b;
    res = read(uart, (void *)&ti4b, 4);
    close(uart);
    if (res < 0)
    {
        return -3;
    }

    *ti = (double) ti4b;

    return 0;
}

int getPot(double *pot){
    int uart = startUart();
    if (uart == -1)
    {
        return uart;
    }

    char buffer1[] = {0xA2, 8, 2, 5, 1};

    int ress = write(uart, &buffer1, 5);
    if (ress < 0)
    {
        close(uart);
        return -4;
    }

    usleep(100000);

    float pot4b;
    ress = read(uart, (void *)&pot4b, 4);
    close(uart);
    if (ress < 0)
    {
        return -5;
    }
    *pot = (double) pot4b;

    return 0;
}