#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include <wiringPi.h>

int movie_count;
int movie_curr = 1;
char **movies;

/* WiringPi numbering */
/* FIXME: should be configurable */
int btn_next_pin = 13;
int btn_prior_pin = 14;

#ifdef __arm__
#define RASPBI 1
#endif

static int flag_interrupt = 0;

inline int check_interrupt () {
  int i = flag_interrupt;
  flag_interrupt = 0;
  return i;
}

inline void raise_interrupt () {
  flag_interrupt = 1;
}

void run_or_die (char *command) {
    int ret = system(command);
    if ( ret != 0 ) {
	fprintf(stderr, "command '%s' failed, exiting %d", command, ret);
	exit(EXIT_FAILURE);
    }
}

void setup_gpio_button (int pin) {
    int limit = 256;
    char command[limit];

    int gpio_pin = pin;
#ifdef RASPBI
    gpio_pin = wpiPinToGpio(pin);
#endif
    snprintf(command, limit, "gpio edge %d rising", gpio_pin);

#ifdef RASPBI
    pinMode(pin, INPUT);
    pullUpDnControl(pin, PUD_UP);
    run_or_die(command);
#endif
}

void setup_gpio () {
    int ret = 0;
#ifdef RASPBI
    // wiringPiSetupSys() doesn't seem to work
    ret = wiringPiSetup();
#endif
    if ( ret == -1 ) {
	fprintf(stderr, "wiringPi setup failed: %d", errno);
	exit(EXIT_FAILURE);
    }
    setup_gpio_button(btn_next_pin);
    setup_gpio_button(btn_prior_pin);
}

char* current_movie () {
    return movies[movie_curr];
}

void next_movie () {
    movie_curr++;
    if ( movie_curr >= movie_count ) {
	/* loop around */
	movie_curr = 1;
    }
}

void prior_movie () {
    if ( movie_curr > 1 ) {
	movie_curr--;
    } else {
	/* loop around */
	movie_curr = movie_count - 1;
    }
}

/* interrupt handlers .... 
 * note these run within a while loop which is already hitting next_movie on
 * each iteration
 */
void next_movie_intr () {
    printf("interrupt: next\n");
    raise_interrupt();
}

void prior_movie_intr () {
    printf("interrupt: prior\n");
    prior_movie();		/* once to counteract the next movie which is to come */
    prior_movie();		/* once to actually go back */
    raise_interrupt();
}


void setup_button_callbacks () {
    /* https://projects.drogon.net/raspberry-pi/wiringpi/functions/ */
    void (*fp)();
    fp = &next_movie_intr;
    /* FIXME: what about retval? */
    wiringPiISR(btn_next_pin, INT_EDGE_RISING, fp);
    fp = &prior_movie_intr;
    wiringPiISR(btn_prior_pin, INT_EDGE_RISING, fp);
}

/* for testing on non-raspi only */


void setup_interrupt_callbacks () {
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = next_movie;
    sigaction(SIGUSR1, &sa, NULL);

    sa.sa_handler = prior_movie;
    sigaction(SIGUSR2, &sa, NULL);
}


