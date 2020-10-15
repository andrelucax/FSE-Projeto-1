#ifndef _THREADS_UTILS_H_
#define _THREADS_UTILS_H_

#include <pthread.h>

int init_threads();
void finish_threads();
void *watch_userinput();
void wait_to_finish();
void calc_next_values();
void *watch_nextvals();
// void *watch_getti();
void *watch_gettitr();
void *watch_gette();
void *watch_getpot();
void handle_vet_res();

#endif /* _THREADS_UTILS_H_ */