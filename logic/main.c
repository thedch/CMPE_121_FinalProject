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
#define BAUDRATE B115200 // UART speed

int detectCommand(char* command);

int graphSetup(int width, int height, int xscale, int trigger_dir);

int graphChannels(int channel[][5000], int finalIndex);

int nchannels = 8;
int trigger_dir = 1; // 1 is positive, 0 is negative
int mem_depth = 100;
int xscale = 25;
int yscale = 100;
int channelOffset = 150;

struct signal {
	uint8_t signal; // 8 bit number consisting of the 8 channels combined into a number
	uint8_t potData; // pot data fed into 8 bit ADC
};

struct signal mySignal;

int main () {
	
	// UART set up
	struct termios serial; // Structure to contain UART parameters
	
	char* dev_id = "/dev/serial0"; // UART device identifier        
    int read_bytes = 0;    
    
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
	
	int commandCode = 0;	
	int channel[8][5000];
	int prevChannelData = 0;
	char s[3];
	char inputFromUser[MAX_INPUT];
		
	int k;
	int rcount = 0;
	for (k = 0; k < 1000; k++) {
		while (rcount < sizeof(mySignal)) {
			read_bytes = read(fd, &mySignal, sizeof(mySignal)-rcount);
			rcount += read_bytes;
		}
		rcount = 0;
		printf("%d, %d\n", mySignal.signal, mySignal.potData);		
	}
	
	printf("Welcome to Daniel Hunter's Logic Analyzer!\n");
	printf("Please enter any desired commands, or enter run to begin.\n");
	
	do {
		fgets(inputFromUser, MAX_INPUT - 1, stdin);
		commandCode = detectCommand(inputFromUser);
	} while (commandCode != 7);
	
	int width, height;
	init(&width, &height);
	
	int i = 0;
	int j = 0;
	int doneFlag = 0;
	int triggeredFlag = 0;
	int finalIndex = 0;
	// int rcount = 0;
	
	while(1) {
		// Recieve the struct
		while (rcount < sizeof(mySignal)) {
			read_bytes = read(fd, &mySignal, sizeof(mySignal)-rcount);
			rcount += read_bytes;
		}
		rcount = 0;
		// channelOffset = 150 + mySignal.potData;
		
		if (!doneFlag) {					
			// Store the byte as bits
			channel[0][i] = (mySignal.signal & 0x80) ? 1 : 0; // [0] is MSB. so compare to 1000 0000
			channel[1][i] = (mySignal.signal & 0x40) ? 1 : 0;
			channel[2][i] = (mySignal.signal & 0x20) ? 1 : 0;
			channel[3][i] = (mySignal.signal & 0x10) ? 1 : 0;

			channel[4][i] = (mySignal.signal & 0x08) ? 1 : 0;
			channel[5][i] = (mySignal.signal & 0x04) ? 1 : 0;
			channel[6][i] = (mySignal.signal & 0x02) ? 1 : 0;
			channel[7][i] = (mySignal.signal & 0x01) ? 1 : 0;
			
			if (channel[0][i] && channel[1][i] && channel[2][i] && channel[3][i] && channel[4][i] && channel[5][i] && channel[6][i] && channel[7][i]) { // Trigger condition checker
				triggeredFlag = 1;
			}
			
			i++;
			i = i % mem_depth; // Walk through the array, rolling over at mem_depth
		
			
			if (triggeredFlag) {
				j++;
				if (j >= mem_depth/2) { // Save half of memory depth samples to display
					finalIndex = i; // Save where we stopped to graph it later
					doneFlag = 1;
				}
			}
		} else { // The analyzer has been triggered and the data is saved in the array, graph it based on potData
			graphSetup(width, height, xscale, trigger_dir);
			graphChannels(channel, finalIndex);
			End();
		}
	}

	fgets(s, 2, stdin); // look at the pic, end with [RETURN]
	finish(); // Graphics cleanup
	exit(0);

}

// gcc -I/opt/vc/include -I/opt/vc/include/interface/vmcs_host/linux -I/opt/vc/include/interface/vcos/pthreads main.c -o main -lshapes

int graphChannels(int channel[][5000], int finalIndex) {
	int curChan = 0;
	int HPixel1 = 0;
	while (curChan < nchannels) {	
		Stroke((rand() % 128) + 128, (rand() % 128) + 128, (rand() % 128) + 128, 1);
		// potData = 255;
		// Stroke(potData, 0, 0, 1);
		for (HPixel1 = 0; HPixel1 * xscale < 1920; HPixel1++) {
			Line(HPixel1*xscale, 
				channel[curChan][finalIndex - mem_depth/2 + HPixel1]*yscale + channelOffset*curChan, 
				HPixel1*xscale + xscale, 
				channel[curChan][finalIndex - mem_depth/2 + HPixel1]*yscale + channelOffset*curChan);
				
			if (channel[curChan][finalIndex - mem_depth/2 + HPixel1] != channel[curChan][finalIndex - mem_depth/2 + HPixel1 - 1]) {
				// A transition was made, add a vertical line
				Line(HPixel1*xscale, 
					channel[curChan][finalIndex - mem_depth/2 + HPixel1]*yscale + channelOffset*curChan, 
					HPixel1*xscale, 
					channel[curChan][finalIndex - mem_depth/2 + HPixel1-1]*yscale + channelOffset*curChan);
			}
		}		
		curChan++;
	}	
	return 0;
}

int graphSetup(width, height, xscale, trigger_dir) {
	int i;
	char strToPrint[MAX_INPUT];	

	Start(width, height);
	Background(0, 0, 0); // Black background
	
	Stroke(255, 255, 255, .5);
	StrokeWidth(2);
	
	for (i = 0; i < 1920; i = i + 150) {
		Line(i, 0, i, 1200);
	}
	
	for (i = 0; i < 1200; i = i + 150) {
		Line(0, i, 1920, i);
	}	
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
	
