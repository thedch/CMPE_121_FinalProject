// Final Project for CMPE 121 at UCSC
// Daniel Hunter
// Logic Analyzer

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

int detectCommand(char* command);

int graphSetup(int width, int height, int xscale, int trigger_dir);

int graphChannels(int channel[][5000]);

int nchannels = 1;
int trigger_dir = 1; // 1 is positive, 0 is negative
int mem_depth = 5000;
int xscale = 50;
int yscale = 100;
int channelOffset = 150;

int main () {
	
	// UART set up
	struct termios serial; // Structure to contain UART parameters
	
	char* dev_id = "/dev/serial0"; // UART device identifier    
    uint8_t rxData = 0; // Receive data    
    int read_bytes = 0;    
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
	
	printf("Welcome to Daniel Hunter's Logic Analyzer!\n");
	printf("Please enter any desired commands, or enter run to begin.\n");
	
	int commandCode = 0;
	int i = 0;
	int channel[8][5000];
	int prevChannelData = 0;
	char s[3];
	char inputFromUser[MAX_INPUT];
	
	do {
		fgets(inputFromUser, MAX_INPUT - 1, stdin);
		commandCode = detectCommand(inputFromUser);
	} while (commandCode != 7);
	
	int width, height;
	init(&width, &height);
	wiringPiSetup();
	pinMode(0, INPUT);
	
	//for (i = 0; i < 5000; i++) {
		//channel[0][i] = i%2;
		//channel[1][i] = i%2;
		//channel[2][i] = i%2;
	//}
	
	//channel[0][0] = 0;
	//channel[0][1] = 0;
	//channel[0][2] = 0;
	//channel[0][3] = 1;
	//channel[0][4] = 1;
	//channel[0][5] = 1;
	//channel[0][6] = 1;
	
	for (i = 0; i < 5000; i++) {
		// Get a byte
		do {
			read_bytes = read(fd, &rxData, 1);
		} while (read_bytes < 1);
		
		// Store the byte as bits
		channel[0][i] = (rxData & 0x80) ? 1 : 0; // [0] is MSB. so compare to 1000 0000
		channel[1][i] = (rxData & 0x40) ? 1 : 0;
		channel[2][i] = (rxData & 0x20) ? 1 : 0;
		channel[3][i] = (rxData & 0x10) ? 1 : 0;
		
		channel[4][i] = (rxData & 0x08) ? 1 : 0;
		channel[5][i] = (rxData & 0x04) ? 1 : 0;
		channel[6][i] = (rxData & 0x02) ? 1 : 0;
		channel[7][i] = (rxData & 0x01) ? 1 : 0;
		
		// if channel = trigger cond, break and store current index
	}
	
	while(1) {
		// Set up the graph
		graphSetup(width, height, xscale, trigger_dir);
		graphChannels(channel);		
		End();
	}
	
	fgets(s, 2, stdin); // look at the pic, end with [RETURN]
	finish(); // Graphics cleanup
	exit(0);
	
}

// gcc -I/opt/vc/include -I/opt/vc/include/interface/vmcs_host/linux -I/opt/vc/include/interface/vcos/pthreads main.c -o main -lshapes -lwiringPi

graphChannels(int channel[][5000]) {
	int curChan = 0;
	int HPixel1 = 0;
	while (curChan < nchannels) {		
		for (HPixel1 = 0; HPixel1 * xscale < 1920; HPixel1++) {
			Line(HPixel1*xscale, channel[curChan][HPixel1]*yscale + channelOffset*curChan, HPixel1*xscale + xscale, channel[curChan][HPixel1]*yscale + channelOffset*curChan);
			if (channel[curChan][HPixel1] != channel[curChan][HPixel1 - 1]) {
				// A transition was made, add a vertical line
				Line(HPixel1*xscale, channel[curChan][HPixel1]*yscale + channelOffset*curChan, HPixel1*xscale, channel[curChan][HPixel1-1]*yscale + channelOffset*curChan);
			}
		}
		Stroke((rand() % 128) + 128, (rand() % 128) + 128, (rand() % 128) + 128, 1);
		curChan++;
	}	
	return 0;
}

graphSetup(width, height, xscale, trigger_dir) {
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
	
	Stroke(255, 0, 0, 1); // Red line for graph
	return 0;
}

detectCommand(char* command) {	
	
	if (strstr(command, "nchannels") != NULL) {
		if (strstr(command, "1") != NULL) {
			nchannels = 1;
			printf("Set nchannels to 1\n");
		} else if (strstr(command, "2") != NULL) {
			nchannels = 2;
			printf("Set nchannels to 2\n");
		} else if (strstr(command, "3") != NULL) {
			nchannels = 3;
			printf("Set nchannels to 3\n");
		} else if (strstr(command, "4") != NULL) {
			nchannels = 4;
			printf("Set nchannels to 4\n");
		} else if (strstr(command, "5") != NULL) {
			nchannels = 5;
			printf("Set nchannels to 5\n");
		} else if (strstr(command, "6") != NULL) {
			nchannels = 6;
			printf("Set nchannels to 6\n");
		} else if (strstr(command, "7") != NULL) {
			nchannels = 7;
			printf("Set nchannels to 7\n");
		} else if (strstr(command, "8") != NULL) {
			nchannels = 8;
			printf("Set nchannels to 8\n");
		} else {
			printf("[ERROR] Invalid nchannels input. Please use 1 through 8.\n");
		}
		return 1;
	
	} else if (strstr(command, "trigger_cond") != NULL) {
		printf("Coming soon!");
		return 2;
		
	} else if (strstr(command, "trigger_dir") != NULL) {
		if (strstr(command, "positive") != NULL) {
			trigger_dir = 1;
			printf("Set trigger direction to positive\n");
		} else if (strstr(command, "negative") != NULL) {
			trigger_dir = 0;
			printf("Set trigger direction to negative\n");
		} else {
			printf("[ERROR] Invalid trigger direction input. Please use positive or negative.\n");
		}
		return 3;
			
	} else if (strstr(command, "mem_depth") != NULL) {
		if (strstr(command, "1000") != NULL) {
			mem_depth = 1000;
			printf("Set memory depth to 1000\n");
		} else if (strstr(command, "2000") != NULL) {
			mem_depth = 2000;
			printf("Set memory depth to 2000\n");
		} else if (strstr(command, "3000") != NULL) {
			mem_depth = 3000;
			printf("Set memory depth to 3000\n");
		} else if (strstr(command, "4000") != NULL) {
			mem_depth = 4000;
			printf("Set memory depth to 4000\n");
		} else if (strstr(command, "5000") != NULL) {
			mem_depth = 5000;
			printf("Set memory depth to 5000\n");
		} else {
			printf("[ERROR] Invalid memory depth input. Please use 1000-5000.\n");
		}
		return 4;
			
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
		} else if (strstr(command, "1") != NULL) {
			printf("Set xscale to 1\n");
			xscale = 1;
		} else {
			printf("[ERROR] Invalid xscale input.\n");
		}			
		return 6;
		
	} else if (strstr(command, "run") != NULL) {
		return 7;
	
	} else {
		printf("[ERROR] Command Not Recognized. Please try again.\n");
		return 0;
	}

}
	
