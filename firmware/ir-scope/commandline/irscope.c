
/*
General Description:
This program receives data from custom USB device whenever data is ready at the device
The data is printed on console
*/

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#ifndef __MINGW32_VERSION
#   include <syslog.h>
#   include <sys/errno.h>
#endif
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>
#include <math.h>
#include <usb.h>    /* this is libusb, see http://libusb.sourceforge.net/ */
#include <math.h>

#include "opendevice.h"
#include "../firmware/usbconfig.h"

#define CMD_READ    1
/* These are the vendor specific SETUP commands implemented by our USB device */

#ifdef __MINGW32_VERSION
#define sleep(x)    _sleep(x)
#endif

/* Multimedia keys report */
typedef struct {
	uint8_t start;
	uint8_t id;
	int16_t duration;
} report_t;



static char *filename = "sensor.csv";
static FILE *fileFp = NULL;


static void usage(char *name)
{
    fprintf(stderr, "usage: %s [-test] [-f csv-file] [-c calibration-file] [-v <vendor-id>] [-d <device-id>] [-debug]\n", name);
}


/* ------------------------------------------------------------------------- */

static void signalReopenFile(int signalNr)
{
#ifndef __MINGW32_VERSION   /* MinGW does not emulate signals */
    signal(SIGHUP, signalReopenFile);   /* re-schedule for this signal */
#endif
    if(fileFp != NULL){
        fclose(fileFp);
        fileFp = NULL;
    }
    fileFp = fopen(filename, "a");
    if(fileFp == NULL){
        printf("error opening file \"%s\" for append: [%d] %s\n", filename, errno, strerror(errno));
        exit(1);
    }
}

static char *getOptionArg(int *index, int argc, char **argv)
{
    (*index)++;
    if(*index >= argc){
        fprintf(stderr, "argument for option \"%s\" missing.\n", argv[*index - 1]);
        usage(argv[0]);
        exit(1);
    }
    return argv[*index];
}


int main(int argc, char **argv)
{
usb_dev_handle      *handle;
unsigned char       buffer[8], expect_id = 0;
int 				vid, pid;
const unsigned char rawVid[2] = {USB_CFG_VENDOR_ID}, rawPid[2] = {USB_CFG_DEVICE_ID};
char                vendor[] = {USB_CFG_VENDOR_NAME, 0}, product[] = {USB_CFG_DEVICE_NAME, 0};
bool				started = false;
report_t *			pSample;
int 				nBytes;
int 				count = 0;
int 				i;
int 				sticks;
bool 				neg_duration;

	usb_init();
	
    for(i=1;i<argc;i++){
		if(strcmp(argv[i], "-f") == 0){
            filename = getOptionArg(&i, argc, argv);
        }else{
            fprintf(stderr, "option \"%s\" not recognized.\n", argv[i]);
            usage(argv[0]);
            exit(1);
        }
    }
	
	/* compute VID/PID from usbconfig.h so that there is a central source of information */
	vid = rawVid[1]*256 + rawVid[0];
	pid = rawPid[1]*256 + rawPid[0];
	
restart:    /* we jump back here if an error occurred */
	printf("\nWaiting for device...");	
	/* The following function is in opendevice.c: */
    if(usbOpenDevice(&handle, vid, vendor, pid, product, NULL, NULL, NULL) != 0){
        fprintf(stderr, "Could not find USB device \"%s\" with vid=0x%x pid=0x%x\n", product, vid, pid);
        exit(1);
    }
	printf("Device detected\n");
    if(usb_set_configuration(handle, 1) < 0){
        printf("error setting USB configuration: %s\n", usb_strerror());
    }
    if(usb_claim_interface(handle, 0) < 0){
        printf("error setting USB interface: %s\n", usb_strerror());
    }
   
    signalReopenFile(1);    /* open file */
    for(;;) {
        /* wait for interrupt, set timeout to more than a week */
        nBytes = usb_interrupt_read(handle, USB_ENDPOINT_IN | 1 , (char *)buffer, sizeof(buffer), 700000 * 1000);
        if(nBytes < 0) {
            printf("error in USB interrupt read: %s\n", usb_strerror());
            goto usbErrorOccurred;
        }
		if(nBytes < 4) {
                printf("data format error, only %d bytes received (4 expected)\n", nBytes);
		}else {
			pSample = (report_t *)&buffer[0];
			for(i = 0; i < 2; i++) {  /* Report contains 2 samples - process each one */
				if(pSample[i].start == 0) { /* first time 'start' changes from 1 to 0 */
					if(started == false) {
						started = true;
						expect_id = 1;
						count = 0;
						printf("\n\n\n");
					}
					if(pSample[i].id > 0) {
						if(pSample[i].id == expect_id) {
						#if 0
							if(pSample[i].id & 0x1) {
								printf("-");
							}
							else {
								printf("+");
							}
						#endif
							//printf("%d  ", (int)((float)pSample[i].duration * 5.33));
							if(abs(pSample[i].duration) * 5 < 400) {
								sticks = 1;
							}
							else {
								sticks = (abs(pSample[i].duration) * 5)/400;
							}
							neg_duration = (pSample[i].duration < 0) ? true : false;
							putchar('|');
							while(sticks--) {
								if(neg_duration) {
									putchar('_');
								}
								else {
									putchar('`');
								}
							}
							count++;
							expect_id++;
						}
						else if(pSample[i].id == (expect_id-1)) {
							printf(".");
						}
					}
				}
				else if(pSample[i].start == 1) {
					if(started == true) { /* first time 'start'changes from 0 to 1 */
						printf("\n%d Received\n", count);
						started = false;
					}
				}
			}
        }
    }
usbErrorOccurred:
    usb_close(handle);
    sleep(5);
    goto restart;
    return 0;
}

/* ------------------------------------------------------------------------- */
