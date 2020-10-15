#include <ncurses_utils.h>
#include <threads_utils.h>
#include <control_lcd_16x2.h>
#include <bme_te.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <bcm2835.h>

// Handle signals
void sig_handler(int signum);

void on_exit_p(bool wait_ui);

bool is_running = true;

int main()
{
    // Get SIGALARM
    signal(SIGALRM, sig_handler);

    // SIGALARM every 0.5 secs
    ualarm(500000, 500000);

    // Get signals
    signal(SIGKILL, sig_handler);
    signal(SIGSTOP, sig_handler);
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
    signal(SIGABRT, sig_handler);
    signal(SIGQUIT, sig_handler);

    int isOk;

    isOk = setup_bme280();
    if (isOk)
    {
        printf("Failed to start bme280. Error %d\n", isOk);
        return (isOk);
    }

    isOk = lcd_init();
    if (isOk)
    {
        printf("Failed to start lcd. Error %d\n", isOk);
        return (isOk);
    }

    if (!bcm2835_init())
    {
        printf("Failed to start bcm2835\n");
        exit(1);
    };

    bcm2835_gpio_fsel(RPI_V2_GPIO_P1_18, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(RPI_V2_GPIO_P1_16, BCM2835_GPIO_FSEL_OUTP);

    // Initialize ncurses and screens
    isOk = init_screens();
    if (isOk)
    {
        printf("Failed to start ncurses. Error %d\n", isOk);
        return (isOk);
    }

    // Initialize threads
    isOk = init_threads();
    if (isOk)
    {
        printf("Failed to start threads. Error %d\n", isOk);
        return (isOk);
    }

    // Wait user input to finish
    wait_to_finish();
    is_running = false;

    on_exit_p(true);

    return 0;
}

void sig_handler(int signum)
{
    if (signum == SIGALRM)
    {
        if (is_running)
        {
            // Get values every 500 ms
            calc_next_values();
        }
    }
    else
    {
        on_exit_p(false);

        exit(0);
    }
}

void on_exit_p(bool wait_ui)
{
    ClrLcd();

    // Turn off vent
    bcm2835_gpio_write(RPI_V2_GPIO_P1_18, 1);
    // Turn off res
    bcm2835_gpio_write(RPI_V2_GPIO_P1_16, 1);

    if (wait_ui)
    {
        say_goodbye();
    }

    // Clean screens
    finish_screens(wait_ui);

    // Clean Threads
    finish_threads();
}