#ifndef _UART_UTILS_H_
#define _UART_UTILS_H_

#include <stdio.h>
#include <unistd.h>

int startUart();
int getDatas(double *ti, double *pot);
int getTi(double *ti);
int getPot(double *pot);

#endif /* _UART_UTILS_H_ */