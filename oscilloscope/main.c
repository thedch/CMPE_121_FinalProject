// Final Project for CMPE 121 at UCSC
// Daniel Hunter
// O'Scope

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "VG/openvg.h"
#include "VG/vgu.h"
#include "fontinfo.h"
#include "shapes.h"

#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <wiringPi.h>

#define MAX_INPUT 100

const int sineTable[128] =
{
	128, 134, 140, 147, 153, 159, 165, 171,
	177, 183, 188, 194, 199, 204, 209, 214,
	218, 223, 227, 231, 234, 238, 241, 244,
	246, 248, 250, 252, 253, 254, 255, 255,
	255, 255, 255, 254, 253, 252, 250, 248,
	246, 244, 241, 238, 234, 231, 227, 223,
	218, 214, 209, 204, 199, 194, 188, 183,
	177, 171, 165, 159, 153, 147, 140, 134,
	128, 122, 115, 109, 103,  97,  91,  85,
	79,  73,  68,  62,  57,  52,  47,  42,
	37,  33,  29,  25,  22,  18,  15,  12,
	10,   7,   6,   4,   2,   1,   1,   0,
	0,   0,   1,   1,   2,   4,   6,   7,
	10,  12,  15,  18,  22,  25,  29,  33,
	37,  42,  47,  52,  57,  62,  68,  73,
	79,  85,  91,  97, 103, 109, 115, 122
};

int detectCommand(char* command);

int graphSetup(int width, int height, int xscale, int yscale, int trigger_channel, int trigger_slope, int trigger_level, int mode, int nchannels);

int nchannels = 1;
int mode = 1; // 1 is free_run, 2 is trigger
int xscale = 50;
int yscale = 2;
int trigger_level = 1;
int trigger_slope = 1;
int trigger_channel = 1;
int graphPotData = 1;

#define TRIGGER_LEVEL 150 // 0-256, please

#define BAUDRATE B115200 // UART speed

struct signal {
	uint8_t signal1;
	uint8_t signal2;
	uint8_t potData; // pot data fed into 8 bit ADC
};

struct signal mySignal;
struct signal myOldSignal;

int main () {

	// UART set up
	struct termios serial; // Structure to contain UART parameters

	char* dev_id = "/dev/serial0"; // UART device identifier
	uint8_t rxData = 0; // Receive data
	uint8_t prevRxData = 0;
	int read_bytes = 0;
	int i = 0;
	int HPixel1 = 0;

	printf("Opening %s\n", dev_id);
	int fd = open(dev_id, O_RDWR | O_NOCTTY | O_NDELAY);

	if (fd == -1) { // Open failed
		perror(dev_id);
		return -1;
	}

	// Get UART configuration
	if (tcgetattr(fd, &serial) < 0) {
		perror("Getting configuration");
		return -1;
	}

	// Set UART parameters in the termios structure
	serial.c_iflag = 0;
	serial.c_oflag = 0;
	serial.c_lflag = 0;
	serial.c_cflag = BAUDRATE | CS8 | CREAD | PARENB | PARODD;
	// Speed setting + 8-bit data + Enable RX + Enable Parity + Odd Parity

	serial.c_cc[VMIN] = 0; // 0 for Nonblocking mode
	serial.c_cc[VTIME] = 0; // 0 for Nonblocking mode

	// Set the parameters by writing the configuration
	tcsetattr(fd, TCSANOW, &serial);

	// Begin taking user input
	char s[3];
	int commandCode = 0;
	int signalOneOffset = 500;	
	char inputFromUser[MAX_INPUT];
	int rcount = 0;

	printf("Welcome to Daniel Hunter's O'Scope!\n");
	printf("Please enter any desired commands, or enter start to begin.\n");

	//for (i = 0; i < 10000; i++) {
	//do {
	//read_bytes = read(fd, &rxData, 1);
	//} while (read_bytes < 1);
	//printf("%d\n", rxData);
	//}

	do {
		fgets(inputFromUser, MAX_INPUT - 1, stdin);
		commandCode = detectCommand(inputFromUser);
	} while (commandCode != 8);

	// Set up graphics
	int width, height;
	init(&width, &height);

	while (1) {
		graphSetup(width, height, xscale, yscale, trigger_channel, trigger_slope, trigger_level, mode, nchannels);
		for (HPixel1 = 0; HPixel1 < 1920; HPixel1 = HPixel1 + xscale) { // HPixel1 * xscale < 1920?
			// Walk across the screen, reading bytes and drawing data
			while (rcount < sizeof(mySignal)) {
				read_bytes = read(fd, &mySignal, sizeof(mySignal)-rcount);
				rcount += read_bytes;
			}
			rcount = 0;
			
			if (HPixel1 != 0 || mode == 1 || mySignal.signal1 > TRIGGER_LEVEL) {
				// Draw the first channel
				Line(HPixel1,
					myOldSignal.signal1 + 100,
					HPixel1 + xscale,
					mySignal.signal1 + 100);
				
				if (nchannels == 2) {
					// Draw the second channel
					Stroke(0, 255, 0, 1); // Green line for second graph
					
					Line(HPixel1,
						myOldSignal.signal2 + signalOneOffset + graphPotData,
						HPixel1 + xscale,
						mySignal.signal2 + signalOneOffset + graphPotData);
						
					Stroke(255, 0, 0, 1); // Red line for graph
				}				
				myOldSignal.signal1 = mySignal.signal1;
				myOldSignal.signal2 = mySignal.signal2;
			} else {
				break;
			}				
		}
		graphPotData = mySignal.potData;
		End(); // End the picture
	}

	fgets(s, 2, stdin); // look at the pic, end with [RETURN]
	finish(); // Graphics cleanup
	exit(0);

}

// gcc -I/opt/vc/include -I/opt/vc/include/interface/vmcs_host/linux -I/opt/vc/include/interface/vcos/pthreads main.c -o main -lshapes

graphSetup(width, height, xscale, yscale, trigger_channel, trigger_slope, trigger_level, mode, nchannels) {

	int i;
	char strToPrint[MAX_INPUT];

	Start(width, height); // Start the picture
	Background(0, 0, 0); // Black background

	Stroke(255, 255, 255, .5);
	StrokeWidth(2);

	for (i = 0; i < 1920; i = i + 150) {
		Line(i, 0, i, 1200);
	}

	for (i = 0; i < 1200; i = i + 150) {
		Line(0, i, 1920, i);
	}

	Stroke(255, 255, 255, 1);
	StrokeWidth(2);
	Rect(0, 0, 500, 220);

	Fill(255, 255, 255, 1); // Set the fill to be fully white for the text
	int textHeight = 10;
	int textSize = 20;
	int textOffset = 30;

	sprintf(strToPrint, "Xscale = %d", xscale);
	TextMid(250, textHeight, strToPrint, SerifTypeface, textSize);
	textHeight += textOffset;
	sprintf(strToPrint, "Yscale = %d", yscale);
	TextMid(250, textHeight, strToPrint, SerifTypeface, textSize);
	textHeight += textOffset;
	sprintf(strToPrint, "Trigger Channel = %d", trigger_channel);
	TextMid(250, textHeight, strToPrint, SerifTypeface, textSize);
	textHeight += textOffset;
	sprintf(strToPrint, "Trigger Slope = %d", trigger_slope);
	TextMid(250, textHeight, strToPrint, SerifTypeface, textSize);
	textHeight += textOffset;
	sprintf(strToPrint, "Trigger Level = %d", trigger_level);
	TextMid(250, textHeight, strToPrint, SerifTypeface, textSize);
	textHeight += textOffset;
	sprintf(strToPrint, "Mode = %d", mode);
	TextMid(250, textHeight, strToPrint, SerifTypeface, textSize);
	textHeight += textOffset;
	sprintf(strToPrint, "# Channels = %d", nchannels);
	TextMid(250, textHeight, strToPrint, SerifTypeface, textSize);

	Stroke(255, 0, 0, 1); // Red line for graph

	return 0;
}

int detectCommand(char* command) {

	if (strstr(command, "nchannels") != NULL) {
		if (strstr(command, "1") != NULL) {
			nchannels = 1;
			printf("Set nchannels to 1\n");
		} else if (strstr(command, "2") != NULL) {
			nchannels = 2;
			printf("Set nchannels to 2\n");
		} else {
			printf("[ERROR] Invalid nchannels input. Please use either 1 or 2.\n");
		}
		return 1;

	} else if (strstr(command, "mode") != NULL) {
		if (strstr(command, "free_run") != NULL) {
			mode = 1;
			printf("Set mode to free_run\n");
		} else if (strstr(command, "trigger") != NULL) {
			mode = 2;
			printf("Set mode to trigger\n");
		} else {
			printf("[ERROR] Invalid mode input. Please use either free_run or trigger.\n");
		}
		return 2;

	} else if (strstr(command, "trigger_level") != NULL) {
		return 3;

	} else if (strstr(command, "trigger_slope") != NULL) {
		if (strstr(command, "positive") != NULL) {
			printf("Set trigger slope to positive\n");
		} else if (strstr(command, "negative") != NULL) {
			printf("Set trigger slope to negative\n");
		} else {
			printf("[ERROR] Invalid trigger slope input. Please use either positive or negative.\n");
		}
		return 4;

	} else if (strstr(command, "trigger_channel") != NULL) {
		if (strstr(command, "1") != NULL) {
			printf("Set trigger channel to 1\n");
			trigger_channel = 1;
		} else if (strstr(command, "0") != NULL) {
			printf("Set trigger channel to 0\n");
			trigger_channel = 0;
		} else {
			printf("[ERROR] Invalid trigger channel input. Please use either positive or negative.\n");
		}
		return 5;

	} else if (strstr(command, "yscale") != NULL) {
		if (strstr(command, "0.5") != NULL) {
			printf("Set yscale to 0.5\n");
			yscale = 0.5;
		} else if (strstr(command, "1.5") != NULL) {
			printf("Set yscale to 1.5\n");
			yscale = 1.5;
		} else if (strstr(command, "1") != NULL) {
			printf("Set yscale to 1\n");
			yscale = 1;
		} else if (strstr(command, "2") != NULL) {
			printf("Set yscale to 2\n");
			yscale = 2;
		} else {
			printf("[ERROR] Invalid yscale input.\n");
		}
		return 6;

	} else if (strstr(command, "xscale") != NULL) {
		if (strstr(command, "10000") != NULL) {
			printf("Set xscale to 10000\n");
			xscale = 10000;
		} else if (strstr(command, "5000") != NULL) {
			printf("Set xscale to 5000\n");
			xscale = 5000;
		} else if (strstr(command, "2000") != NULL) {
			printf("Set xscale to 2000\n");
			xscale = 2000;
		} else if (strstr(command, "1000") != NULL) {
			printf("Set xscale to 1000\n");
			xscale = 1000;
		} else if (strstr(command, "500") != NULL) {
			printf("Set xscale to 500\n");
			xscale = 500;
		} else if (strstr(command, "100") != NULL) {
			printf("Set xscale to 100\n");
			xscale = 100;
		} else if (strstr(command, "10") != NULL) {
			printf("Set xscale to 10\n");
			xscale = 10;
		} else if (strstr(command, "5") != NULL) {
			printf("Set xscale to 5\n");
			xscale = 5;
		} else if (strstr(command, "1") != NULL) {
			printf("Set xscale to 1\n");
			xscale = 1;
		} else {
			printf("[ERROR] Invalid xscale input.\n");
		}
		return 7;

	} else if (strstr(command, "start") != NULL) {
		return 8;

	} else {
		printf("[ERROR] Command Not Recognized. Please try again.\n");
		return 0;
	}
}
