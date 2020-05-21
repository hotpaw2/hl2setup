//
// terminal console version of hl2setup
// 2020-05-16  rhn@nicholson.com
// version 0.2.0
//

#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <math.h>

// #include <string>
// #include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <ifaddrs.h>

#ifdef _WIN32
#define QUISK_SHUT_RD	SD_RECEIVE
#define QUISK_SHUT_BOTH	SD_BOTH
#define close__socket	closesocket
static int cleanupWSA = 0;			// Must we call WSACleanup() ?
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#define INVALID_SOCKET	-1
#define QUISK_SHUT_RD	SHUT_RD
#define QUISK_SHUT_BOTH	SHUT_RDWR
#define close__socket	close
#endif

#include "hl2.h"


void send_discover(int rx_discover_socket)
{
    unsigned char data[64];
    int i, n = 0;
    int port = 1024;
    static struct sockaddr_in bcast_Addr;

    struct ifaddrs * ifap, * p;

    data[0] = 0xEF;
    data[1] = 0xFE;
    data[2] = 0x02;
    for (i = 3; i < 64; i++) { data[i] = 0; }
    memset(&bcast_Addr, 0, sizeof(bcast_Addr));
    bcast_Addr.sin_family = AF_INET;
    bcast_Addr.sin_port = htons(port);
    if (getifaddrs(&ifap) == 0) {
        p = ifap;
        while(p) {
            if ((p->ifa_addr) && p->ifa_addr->sa_family == AF_INET) {
                bcast_Addr.sin_addr
                 = ((struct sockaddr_in *)(p->ifa_broadaddr))->sin_addr;
                i = sendto(rx_discover_socket, (char *)data, 63, 0,
                  (const struct sockaddr *)&bcast_Addr, sizeof(bcast_Addr));
            }
            usleep(50000);
            p = p->ifa_next;
        }
        freeifaddrs(ifap);
    }
    printf("UDP broadcast number %d %d\n", i, n); n++;
}

void cleanShutdown() {
    hermes_key_down = 0;
    hermes_run_state = STATE_IDLE;
    if (board_id > 0) {
        while( close_udp10() ) { };
    }
    board_id = 0;
}

int get_yes_no(char *prompt, int exitFlag)
{
    char buf[82];
    int c;
    bzero(buf,82);
    fprintf(stdout, "%s : ", prompt);
    fflush(stdout);
    fgets(buf,80,stdin);
    c = buf[0];
    if ((c == 'y') || (c == 'Y')) {
        return(1);
    } else {
        if (exitFlag) {
            cleanShutdown();
            exit(0);
        } else {
            return(0);
        }
    }
}

int get_digit(char *prompt)
{
    char buf[82];
    int  c;
    bzero(buf,82);
    fprintf(stdout, "%s : ", prompt);
    fflush(stdout);
    fgets(buf,80,stdin);
    c = buf[0];
    c = c - '0';
    if (c >= 0 & c <= 9) return(c);
    return(-1);
}

static float lastTemp  =  0.0;
static float lastMA    =  0.0;

void testLoopPrint()
{
    char           buf80[80] = { 0 };
    float t = floor(hermes_temperature);
    float c = floor(1000 * hermes_pa_current);
    // printf("foo %f %f %f %f \n",t,lastTemp,c,lastMA);
    if ((t != lastTemp) || (c != lastMA)) {
	snprintf(buf80, 80, "%.0f C", t);
	fprintf(stdout, "%s , ", buf80);
	snprintf(buf80, 80, "%.0f ma", c);
	fprintf(stdout, "%s", buf80);
	fprintf(stdout, "\n");
    }

    lastTemp  =  t; 
    lastMA    =  c;
}

void testLoop()
{
        int i;
        for(i = 0; i< ( 1*1000*60); i++) {
            HL2Run();
	    testLoopPrint();
	    if (hermes_run_state == END_OF_TESTS) break;
	    if (hermes_run_state == STATE_IDLE  ) break;
	    usleep(1000L);
        }
        testLoopPrint();
}

int main(int argc, char **argv)
{ 
    char           buf80[80] = { 0 };
    static double  time0 = 0;
    static int     power = 0;
    int 	   i, r;

    verbose_output =  1;
    board_id       =  0;
    //
    r = get_yes_no("start ? ", 1);

    HL2GetBoardId();
    snprintf(buf80, 80, "hl2 board id : %d", board_id);
    fprintf(stdout, "%s\n", buf80);
    snprintf(buf80, 80, "code version : %d", code_version);
    fprintf(stdout, "%s\n", buf80);
    fprintf(stdout,     "MAC address  : %s\n", mac_address);
    fprintf(stdout,     "IP address   :  %s\n", ip_address);

    //
    if (board_id > 0) {
        r = get_yes_no("continue?", 1);
    } else {
        exit(0);
    }

    //
    hermes_key_down  =  0;
    hermes_enable_power_amp  =  0;
    hermes_run_state =  STATE_IDLE;
    HL2Run();
    hermes_run_state =  STATE_IDLE;

    printf("1 : Start tests\n");
    printf("2 : Set bias \n");
    printf("3 : Test flatness \n");
    r = get_digit("? ");

    switch(r) {
        case (1):
            printf("1\n");
            hermes_run_state =  STATE_START_TESTS;
	    testLoop();
	    break;
        case (2):
            printf("2\n");
            hermes_run_state =  STATE_START_SET_BIAS;
	    testLoop();
	    break;
        case (3):
            printf("2\n");
            hermes_run_state =  STATE_START_TEST_TX_FLATNESS;
	    testLoop();
	    break;
        default:
            break;
    }

    printf("\nexiting \n");
    cleanShutdown();
    exit(0);

    return(0);
}

// eof
