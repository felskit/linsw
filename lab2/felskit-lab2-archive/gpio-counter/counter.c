#include <fcntl.h>
#include <time.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define ERR(source) perror(source),\
	fprintf(stderr, "%s:%d\n", __FILE__, __LINE__),\
	exit(EXIT_FAILURE)

#define MAX_NUM  16
#define BUF_SIZE 64
#define DEBOUNCE 50

#define BTN_COUNT 3
#define BTN_LEFT  10
#define BTN_RIGHT 22
#define BTN_TOP   27

#define LED_COUNT 4
#define LED_BLUE  17
#define LED_WHITE 18
#define LED_GREEN 23
#define LED_RED   24

void export_fds(int* fds, int* pins, char* dir, int count);
void setup_edges(int* pins, char* on, int count);
void unexport_fds(int* fds, int* pins, int count);
void light_leds(int* fds, int count, int num);
char read_value(int fd);
int poll_btns(int* fds, int count);
int mod(int a, int b);

int main(void) {
	int num = 0, work = 1, pressed;
	int btnfds[BTN_COUNT];
	int ledfds[LED_COUNT];
	int btnpins[BTN_COUNT] = { BTN_LEFT, BTN_RIGHT, BTN_TOP };
	int ledpins[LED_COUNT] = { LED_BLUE, LED_WHITE, LED_GREEN, LED_RED };

	export_fds(btnfds, btnpins, "in", BTN_COUNT);
	export_fds(ledfds, ledpins, "out", LED_COUNT);
	setup_edges(btnpins, "falling", BTN_COUNT);

	while (work) {
		light_leds(ledfds, LED_COUNT, num);
		while ((pressed = poll_btns(btnfds, BTN_COUNT)) < 0);
		if (pressed == 3) {
			printf("reset\n\n");
			num = 0;
			continue;
		}
		switch (btnpins[pressed]) {
			case BTN_LEFT:
				printf("-\n\n");
				num = mod(--num, MAX_NUM);
				break;
			case BTN_RIGHT:
				printf("+\n\n");
				num = mod(++num, MAX_NUM);
				break;
			case BTN_TOP:
				printf("stop\n");
				work = 0;
		}
	}

	unexport_fds(btnfds, btnpins, BTN_COUNT);
	unexport_fds(ledfds, ledpins, LED_COUNT);
	return EXIT_SUCCESS;
}

void export_fds(int* fds, int* pins, char* dir, int count) {
	int exportfd, directionfd, i;
	char buf[BUF_SIZE];

	if ((exportfd = open("/sys/class/gpio/export", O_WRONLY)) < 0) {
		ERR("opening export file failed");
	}

	for (i = 0; i < count; ++i) {
		snprintf(buf, BUF_SIZE, "%d", pins[i]);
		if (write(exportfd, buf, strlen(buf)) < 0) {
			ERR("writing to export file failed");
		}
	}

	for (i = 0; i < count; ++i) {
		snprintf(buf, BUF_SIZE, "/sys/class/gpio/gpio%d/direction", pins[i]);
		if ((directionfd = open(buf, O_WRONLY)) < 0) {
			ERR("opening direction file failed");
		}
		if (write(directionfd, dir, strlen(dir)) < 0) {
			ERR("writing to direction file failed");
		}
		if (close(directionfd) < 0) {
			ERR("closing direction file failed");
		}
	}

	for (i = 0; i < count; ++i) {
		snprintf(buf, BUF_SIZE, "/sys/class/gpio/gpio%d/value", pins[i]);
		if ((fds[i] = open(buf, O_RDWR)) < 0) {
			ERR("opening value file failed");
		}
	}

	if (close(exportfd) < 0) {
		ERR("closing export file failed");
	}
}

void setup_edges(int* pins, char* on, int count) {
	int edgefd, i;
	char buf[BUF_SIZE];

	for (i = 0; i < count; ++i) {
		snprintf(buf, BUF_SIZE, "/sys/class/gpio/gpio%d/edge", pins[i]);
		if ((edgefd = open(buf, O_WRONLY)) < 0) {
			ERR("opening edge file failed");
		}
		if (write(edgefd, on, strlen(on)) < 0) {
			ERR("writing to edge type failed");
		}
		if (close(edgefd) < 0) {
			ERR("closing edge file failed");
		}
	}
}

void unexport_fds(int* fds, int* pins, int count) {
	int unexportfd, i;
	char buf[BUF_SIZE];

	if ((unexportfd = open("/sys/class/gpio/unexport", O_WRONLY)) < 0) {
		ERR("opening unexport file failed");
	}

	for (i = 0; i < count; ++i) {
		if (close(fds[i]) < 0) {
			ERR("closing value file failed");
		}
	}

	for (i = 0; i < count; ++i) {
		snprintf(buf, BUF_SIZE, "%d", pins[i]);
		if (write(unexportfd, buf, strlen(buf)) < 0) {
			ERR("writing to unexport file failed");
		}
	}

	if (close(unexportfd) < 0) {
		ERR("closing export file failed");
	}
}

void light_leds(int* fds, int count, int num) {
	int i, bit;
	char value;

	for (i = 0; i < count; ++i) {
		bit = (num >> i) & 1;
		value = bit ? '1' : '0';
		printf("LED_%d: %c\n", i, value);
		if (write(fds[i], &value, sizeof(value)) < 0) {
			ERR("writing to value file failed");
		}
	}
}

char read_value(int fd) {
	char value;

	if (lseek(fd, 0, SEEK_SET) < 0) {
		ERR("cannot seek to start of value file");
	}
	if (read(fd, &value, sizeof(value)) < 0) {
		ERR("cannot read from value file");
	}

	return value;
}

int poll_btns(int* fds, int count) {
	int pressed, i;
	struct pollfd pollfds[count];
	
	for (i = 0; i < count; ++i)
	{
		pollfds[i].fd = fds[i];
		pollfds[i].events = POLLPRI | POLLERR;
	}

	for (i = 0; i < count; ++i) {
		read_value(fds[i]);
	}

	if (poll(pollfds, count, -1) < 0) {
		ERR("poll failed");
	}

	for (i = 0; i < count; ++i) {
		read_value(fds[i]);
	}

	switch (poll(pollfds, count, DEBOUNCE)) {
		case -1:
			ERR("second poll failed");
		case 0:
			pressed = -1;
			break;
		default:
			return -1;
	}

	for (i = 0; i < count; ++i) {
		if (read_value(fds[i]) == '0') {
			printf("BTN_%d is pressed\n", i);
			if (pressed == 0 && i == 1) {
				return 3;
			}
			pressed = pressed < 0 ? i : -1;
		}
	}

	return pressed;
}

int mod(int a, int b) {
	int r = a % b;
	return r < 0 ? r + b : r;
}
