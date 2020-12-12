#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <fcntl.h> 
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <sys/stat.h>

int set_interface_attribs(int fd, int speed, int parity) {
	struct termios tty;
    if (tcgetattr (fd, &tty) != 0) {
        return 1;
    }

    cfsetospeed (&tty, speed);
    cfsetispeed (&tty, speed);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     
    tty.c_iflag &= ~IGNBRK;         
    tty.c_lflag = 0;              
    tty.c_oflag = 0;                
    tty.c_cc[VMIN]  = 1;          
    tty.c_cc[VTIME] = 5;       
    tty.c_iflag &= ~(IXON | IXOFF | IXANY); 
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~(PARENB | PARODD);     
    tty.c_cflag |= parity;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    if (tcsetattr (fd, TCSANOW, &tty) != 0) {
        return 1;
    }

    return 0;
}

void putb(int fd, unsigned char c) {
	write(fd, &c, 1);
}

unsigned char getb(int fd) {
	unsigned char c;
	read(fd, &c, 1);
	return c;
}


enum {
	S_WAIT_START_CRC,
	S_SEND_BLOCK,
	S_WAIT_BLOCK_RESP,
	S_WAIT_FINAL_ACK,

	S_PRINT,

	BLKSIZE = 128
};

enum : unsigned char {
	X_SOH = 0x01,
	X_EOT = 0x04,
	X_ACK = 0x06,
	X_NAK = 0x15
};

const unsigned char CRCLO[256] = {
	0x00,0x21,0x42,0x63,0x84,0xA5,0xC6,0xE7,0x08,0x29,0x4A,0x6B,0x8C,0xAD,0xCE,0xEF,
	0x31,0x10,0x73,0x52,0xB5,0x94,0xF7,0xD6,0x39,0x18,0x7B,0x5A,0xBD,0x9C,0xFF,0xDE,
	0x62,0x43,0x20,0x01,0xE6,0xC7,0xA4,0x85,0x6A,0x4B,0x28,0x09,0xEE,0xCF,0xAC,0x8D,
	0x53,0x72,0x11,0x30,0xD7,0xF6,0x95,0xB4,0x5B,0x7A,0x19,0x38,0xDF,0xFE,0x9D,0xBC,
	0xC4,0xE5,0x86,0xA7,0x40,0x61,0x02,0x23,0xCC,0xED,0x8E,0xAF,0x48,0x69,0x0A,0x2B,
	0xF5,0xD4,0xB7,0x96,0x71,0x50,0x33,0x12,0xFD,0xDC,0xBF,0x9E,0x79,0x58,0x3B,0x1A,
	0xA6,0x87,0xE4,0xC5,0x22,0x03,0x60,0x41,0xAE,0x8F,0xEC,0xCD,0x2A,0x0B,0x68,0x49,
	0x97,0xB6,0xD5,0xF4,0x13,0x32,0x51,0x70,0x9F,0xBE,0xDD,0xFC,0x1B,0x3A,0x59,0x78,
	0x88,0xA9,0xCA,0xEB,0x0C,0x2D,0x4E,0x6F,0x80,0xA1,0xC2,0xE3,0x04,0x25,0x46,0x67,
	0xB9,0x98,0xFB,0xDA,0x3D,0x1C,0x7F,0x5E,0xB1,0x90,0xF3,0xD2,0x35,0x14,0x77,0x56,
	0xEA,0xCB,0xA8,0x89,0x6E,0x4F,0x2C,0x0D,0xE2,0xC3,0xA0,0x81,0x66,0x47,0x24,0x05,
	0xDB,0xFA,0x99,0xB8,0x5F,0x7E,0x1D,0x3C,0xD3,0xF2,0x91,0xB0,0x57,0x76,0x15,0x34,
	0x4C,0x6D,0x0E,0x2F,0xC8,0xE9,0x8A,0xAB,0x44,0x65,0x06,0x27,0xC0,0xE1,0x82,0xA3,
	0x7D,0x5C,0x3F,0x1E,0xF9,0xD8,0xBB,0x9A,0x75,0x54,0x37,0x16,0xF1,0xD0,0xB3,0x92,
	0x2E,0x0F,0x6C,0x4D,0xAA,0x8B,0xE8,0xC9,0x26,0x07,0x64,0x45,0xA2,0x83,0xE0,0xC1,
	0x1F,0x3E,0x5D,0x7C,0x9B,0xBA,0xD9,0xF8,0x17,0x36,0x55,0x74,0x93,0xB2,0xD1,0xF0
};

const unsigned char CRCHI[256] = {
	0x00,0x10,0x20,0x30,0x40,0x50,0x60,0x70,0x81,0x91,0xA1,0xB1,0xC1,0xD1,0xE1,0xF1,
	0x12,0x02,0x32,0x22,0x52,0x42,0x72,0x62,0x93,0x83,0xB3,0xA3,0xD3,0xC3,0xF3,0xE3,
	0x24,0x34,0x04,0x14,0x64,0x74,0x44,0x54,0xA5,0xB5,0x85,0x95,0xE5,0xF5,0xC5,0xD5,
	0x36,0x26,0x16,0x06,0x76,0x66,0x56,0x46,0xB7,0xA7,0x97,0x87,0xF7,0xE7,0xD7,0xC7,
	0x48,0x58,0x68,0x78,0x08,0x18,0x28,0x38,0xC9,0xD9,0xE9,0xF9,0x89,0x99,0xA9,0xB9,
	0x5A,0x4A,0x7A,0x6A,0x1A,0x0A,0x3A,0x2A,0xDB,0xCB,0xFB,0xEB,0x9B,0x8B,0xBB,0xAB,
	0x6C,0x7C,0x4C,0x5C,0x2C,0x3C,0x0C,0x1C,0xED,0xFD,0xCD,0xDD,0xAD,0xBD,0x8D,0x9D,
	0x7E,0x6E,0x5E,0x4E,0x3E,0x2E,0x1E,0x0E,0xFF,0xEF,0xDF,0xCF,0xBF,0xAF,0x9F,0x8F,
	0x91,0x81,0xB1,0xA1,0xD1,0xC1,0xF1,0xE1,0x10,0x00,0x30,0x20,0x50,0x40,0x70,0x60,
	0x83,0x93,0xA3,0xB3,0xC3,0xD3,0xE3,0xF3,0x02,0x12,0x22,0x32,0x42,0x52,0x62,0x72,
	0xB5,0xA5,0x95,0x85,0xF5,0xE5,0xD5,0xC5,0x34,0x24,0x14,0x04,0x74,0x64,0x54,0x44,
	0xA7,0xB7,0x87,0x97,0xE7,0xF7,0xC7,0xD7,0x26,0x36,0x06,0x16,0x66,0x76,0x46,0x56,
	0xD9,0xC9,0xF9,0xE9,0x99,0x89,0xB9,0xA9,0x58,0x48,0x78,0x68,0x18,0x08,0x38,0x28,
	0xCB,0xDB,0xEB,0xFB,0x8B,0x9B,0xAB,0xBB,0x4A,0x5A,0x6A,0x7A,0x0A,0x1A,0x2A,0x3A,
	0xFD,0xED,0xDD,0xCD,0xBD,0xAD,0x9D,0x8D,0x7C,0x6C,0x5C,0x4C,0x3C,0x2C,0x1C,0x0C,
	0xEF,0xFF,0xCF,0xDF,0xAF,0xBF,0x8F,0x9F,0x6E,0x7E,0x4E,0x5E,0x2E,0x3E,0x0E,0x1E
};

unsigned short xcrc(unsigned char buf[BLKSIZE]) {
	unsigned char crclo = 0;
	unsigned char crchi = 0;
    
    for(int i = 0; i < BLKSIZE; i++) {
    	unsigned char x = buf[i] ^ crchi;
    	crchi = crclo ^ CRCHI[x];
    	crclo = CRCLO[x];
    }

    return ((unsigned short)crchi << 8) | (unsigned short)crclo;
}


void flush_signalfd(int fd) {
	signalfd_siginfo _info;
	while(read(fd, &_info, sizeof(signalfd_siginfo)) > 0);
}


int main(int argc, char **argv) {
	if(argc != 3) {
		fprintf(stderr, "Usage: phuck <port> <file>\n");
		return 1;
	}


	int fh = open(argv[2], O_RDONLY);
	if(!fh) {
		fprintf(stderr, "Failed to open %s\n", argv[2]);
		return 1;
	}


	int port = open(argv[1], O_RDWR | O_NOCTTY | O_SYNC | O_NONBLOCK);
	if(port < 0) {
		fprintf(stderr, "Failed to open %s\n", argv[1]);
		return 1;
	}

	if(set_interface_attribs(port, B19200, 0)) {
		fprintf(stderr, "Failed to set interface attribs!\n");
		return 1;
	}


	printf("Connected on %s.\n", argv[1]);


	epoll_event ev_port;
	ev_port.events = EPOLLIN;
	ev_port.data.fd = port;

	sigset_t sigintmask;
	sigemptyset(&sigintmask);
	sigaddset(&sigintmask, SIGINT);
	int sigintfd = signalfd(-1, &sigintmask, O_NONBLOCK);

	sigset_t sigtstpmask;
	sigemptyset(&sigtstpmask);
	sigaddset(&sigtstpmask, SIGTSTP);
	int sigtstpfd = signalfd(-1, &sigtstpmask, O_NONBLOCK);

	sigset_t sigmask;
	sigemptyset(&sigmask);
	sigaddset(&sigmask, SIGINT);
	sigaddset(&sigmask, SIGTSTP);
	sigprocmask(SIG_SETMASK, &sigmask, NULL);


	epoll_event ev_sigint;
	ev_sigint.events = EPOLLIN;
	ev_sigint.data.fd = sigintfd;

	epoll_event ev_sigtstp;
	ev_sigtstp.events = EPOLLIN;
	ev_sigtstp.data.fd = sigtstpfd;

	int ep = epoll_create(3);
	epoll_ctl(ep, EPOLL_CTL_ADD, port, &ev_port);
	epoll_ctl(ep, EPOLL_CTL_ADD, sigintfd, &ev_sigint);
	epoll_ctl(ep, EPOLL_CTL_ADD, sigtstpfd, &ev_sigtstp);

	signal(SIGINT, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);


	int stat = 0;
	int attempt = 0;
	int blkcnt;
	int blk = 1;
	unsigned char blkno = 1;
	unsigned char blkbuf[BLKSIZE];


	struct stat sb;
	fstat(fh, &sb);
	blkcnt = (int)ceil((double)sb.st_size / (double)BLKSIZE);


	printf("Waiting for reciever to begin XMODEM transfer of %d bytes...\n", blkcnt * BLKSIZE);


	memset(blkbuf, 0, BLKSIZE); 
	assert(read(fh, blkbuf, BLKSIZE) != -1);


	int state = S_WAIT_START_CRC;

	int lpc = 0;
	bool run = true;
	while(run) {
		epoll_event event;
		int c = epoll_wait(ep, &event, 1, 0);
		
		if(c == -1) {
			printf("epoll error!\n");
			break;
		}

		if(c == 0) {
			usleep(1000);
			continue;
		}

		if((event.events & EPOLLIN) != 0 && event.data.fd == sigintfd) {
			flush_signalfd(sigintfd);
			run = false;
			break;
		} else if((event.events & EPOLLIN) != 0 && event.data.fd == sigtstpfd) {
			flush_signalfd(sigtstpfd);
			if(state == S_PRINT) {
				fh = open(argv[2], O_RDONLY);
				if(!fh) {
					fprintf(stderr, "Failed to open %s\n", argv[2]);
					continue;
				}

				struct stat sb;
				fstat(fh, &sb);
				blkcnt = (int)ceil((double)sb.st_size / (double)BLKSIZE);

				memset(blkbuf, 0, BLKSIZE);
				assert(read(fh, blkbuf, BLKSIZE) > 0);
				
				blkno = 1;
				blk = 1;
				state = S_WAIT_START_CRC;

				putb(port, '$');
				putb(port, 'X');
				putb(port, 'B');
				putb(port, 'O');
				putb(port, 'O');
				putb(port, 'T');
				putb(port, '$');

				printf("Waiting for reciever to begin XMODEM transfer of %d bytes...\n", blkcnt * BLKSIZE);
				continue;
			}
		} else {
			assert(event.events & EPOLLIN);
			assert(event.data.fd == port);

			switch(state) {
				case S_WAIT_START_CRC: {
					auto f = getb(port);

					if(f == 'C') {
						state = S_SEND_BLOCK;
					} else {
						break;
					}
					// NOTE: intentional fallthrough!
				}
				case S_SEND_BLOCK: {
				_S_SEND_BLOCK:
					if(blk > 1) printf("\x1B[1A");
					printf("\x1B[2KSending block %d/%d...\n", blk, blkcnt);
					putb(port, X_SOH);
					putb(port, blkno);
					putb(port, ~blkno);
				
					for(int i = 0; i < BLKSIZE; i++) putb(port, blkbuf[i]);
				
					unsigned short crc = xcrc(blkbuf);
					putb(port, (unsigned char) (crc >> 8) & 0xFF);
					putb(port, (unsigned char) crc & 0xFF);
					
					state = S_WAIT_BLOCK_RESP;
					break;
				}
				case S_WAIT_BLOCK_RESP: {
					auto f = getb(port);
					if(f == X_ACK) {
						attempt = 0;
						blkno++;
						blk++;

						if(read(fh, blkbuf, BLKSIZE) == 0) {
							putb(port, X_EOT);

							close(fh);
							state = S_WAIT_FINAL_ACK;
						} else {
							state = S_SEND_BLOCK;
							goto _S_SEND_BLOCK;
						}
					} else {
						// TOOD: should X_NACK be handled differently than invalid responses?
						attempt++;
						if(attempt >= 3) {
							fprintf(stderr, "XMODEM transfer failed after three failed attempts to send a block!\n");
							stat = 1;
							goto end;
						} else {
							state = S_SEND_BLOCK;
							goto _S_SEND_BLOCK;
						}
					}
					break;
				}
				case S_WAIT_FINAL_ACK: {
					// NOTE: Technically, ignoring this probably isn't the best... but eh.
					// This byte is the final X_ACK sent from the reciever after X_EOT.
					getb(port);
					printf("XMODEM transfer complete! Press Ctrl+Z to retransmit the file.\n\n");
					state = S_PRINT;
					break;
				}
				case S_PRINT: {
					auto f = getb(port);
					printf("%c", f);
					fflush(stdout);
					/*
					printf("%02X ", f);
					fflush(stdout);
		
					if(++lpc == 16) {
						lpc = 0;
						printf("\n");
					}
					*/
					break;
				}
				default:
					assert(0);
			}
		}
	}
end:
	if(lpc > 0) printf("\n");

	printf("Disconnected.\n");

	close(port);
	close(ep);
	close(sigintfd);
	return stat;
}