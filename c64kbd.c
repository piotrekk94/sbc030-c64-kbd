#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <linux/uinput.h>
#include <termios.h>

char key_state[64] = {0};

static unsigned char keycodes[64] = {
	KEY_2, KEY_Q, KEY_ESC, KEY_SPACE, 0, KEY_LEFTCTRL, KEY_BACKSPACE, KEY_1,
	KEY_4, KEY_E, KEY_S, KEY_Z, KEY_LEFTSHIFT, KEY_A, KEY_W, KEY_3,
	
	KEY_6, KEY_T, KEY_F, KEY_C, KEY_X, KEY_D, KEY_R, KEY_5,
	KEY_8, KEY_U, KEY_H, KEY_B, KEY_V, KEY_G, KEY_Y, KEY_7,

	KEY_0, KEY_O, KEY_K, KEY_M, KEY_N, KEY_J, KEY_I, KEY_9,
	KEY_MINUS, 0/* @ */, 0/* (: */, KEY_DOT, KEY_COMMA, KEY_L, KEY_P, 0/* + */,

	KEY_HOME, KEY_UP, KEY_EQUAL, KEY_RIGHTSHIFT, KEY_SLASH, KEY_SEMICOLON, 0/* * */, 0/* pound */,
	KEY_F7, KEY_F5, KEY_F3, KEY_F1, KEY_DOWN, KEY_RIGHT, KEY_ENTER, KEY_DELETE,
};

void emit(int fd, int type, int code, int val)
{
	struct input_event ie;
	ie.type = type;
	ie.code = code;
	ie.value = val;

	ie.time.tv_sec = 0;
	ie.time.tv_usec = 0;

	write(fd, &ie, sizeof(ie));
}

int main(void)
{
	struct termios options;
	struct uinput_setup usetup;

	int fd = open("/dev/uinput", O_WRONLY);

	if(fd == -1){
		perror("Failed to open /dev/uinput");
		return errno;
	}

	ioctl(fd, UI_SET_EVBIT, EV_KEY);
	ioctl(fd, UI_SET_EVBIT, EV_SYN);

	for(int i = 0; i < sizeof(keycodes); i++)
		if(keycodes[i])
			ioctl(fd, UI_SET_KEYBIT, keycodes[i]);

	memset(&usetup, 0, sizeof(usetup));
	usetup.id.bustype = BUS_RS232;
	usetup.id.vendor = 0x2137;
	usetup.id.product = 0x7312;
	strcpy(usetup.name, "C64 Keyboard");	


	ioctl(fd, UI_DEV_SETUP, &usetup);
	ioctl(fd, UI_DEV_CREATE);

	int tty = open("/dev/ttyS1", O_RDWR | O_NOCTTY | O_SYNC);

	if(tty == -1){
		perror("Failed to open /dev/ttyS1");
		return errno;
	} else {
		fcntl(tty, F_SETFL, 0);
	}

	tcgetattr(tty, &options);

	cfsetispeed(&options, B38400);
	cfsetospeed(&options, B38400);

	options.c_cflag |= (CLOCAL | CREAD);

	options.c_cflag &= ~CSIZE;
	options.c_cflag |= CS8;

	options.c_cflag &= ~PARENB;
	options.c_cflag &= ~CSTOPB;

	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	options.c_iflag &= ~(IXON | IXOFF | IXANY);

	options.c_oflag &= ~OPOST;

	if(tcsetattr(tty, TCSANOW, &options) == -1){
		perror("failed to set tcsetattr");
		return errno;
	}

	for(;;){
		unsigned char key;
		if(read(tty, &key, 1) != 1)
			break;

		if(key >= 128){
			key -= 128;

			if(key >= 64){
				emit(fd, EV_KEY, keycodes[key - 64], 0);
				emit(fd, EV_SYN, SYN_REPORT, 0);
				key_state[key - 64] = 0;
			} else {
				if(key_state[key]){
					emit(fd, EV_KEY, keycodes[key], 2);
				} else {
					emit(fd, EV_KEY, keycodes[key], 1);
					key_state[key] = 1;
				}
				emit(fd, EV_SYN, SYN_REPORT, 0);
			}
		}
	}

	return 0;

}
