/* Name: main.c
 * Project: 
 * Author: 
 * Creation Date: 
 * Tabsize: 
 */


#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>  /* for sei() */
#include <util/delay.h>     /* for _delay_ms() */

#include <avr/pgmspace.h>   /* required by usbdrv.h */
#include "usbdrv.h"

/* F_CPU = 12000000; For 5.33us timer clock, use 1/64 prescalar */
#define TCCR0_VAL 				0x3

/* Max number of time samples to collect */
#define MAX_SAMPLES				250

#define RED_LED					PD0
#define RED_LED_DDR				DDRD
#define RED_LED_PORT			PORTD

#define RED_LED_OUT()			(RED_LED_DDR |= (1 << RED_LED))
#define RED_LED_OFF()			(RED_LED_PORT |= (1 << RED_LED))
#define RED_LED_ON()			(RED_LED_PORT &= ~(1 << RED_LED))
#define RED_LED_TOGGLE()		(RED_LED_PORT ^= (1 << RED_LED))


/* ------------------------------------------------------------------------- */
/* ----------------------------- USB interface ----------------------------- */
/* ------------------------------------------------------------------------- */

PROGMEM const char usbHidReportDescriptor[21] = {    /* USB report descriptor */
    0x06, 0x00, 0xff,              // USAGE_PAGE (Generic Desktop)
    0x09, 0x01,                    // USAGE (Vendor Usage 1)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x00,              //   LOGICAL_MAXIMUM (255)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x95, 0x08,                    //   REPORT_COUNT (8)
    0x09, 0x00,                    //   USAGE (Undefined)
    0x81, 0x02,              		//   INPUT (Data,Var,Abs)
    0xc0                           // END_COLLECTION
};

 
/* HID Input report structure */
typedef struct {
	uint8_t start;
	uint8_t id;
	uint16_t duration;
} report_t;


static report_t report_buf[2];
static uint8_t ledState;

volatile static uint8_t timer0_ovf;
volatile static uint8_t ovf_2;
volatile static uint8_t id;
volatile static uint8_t edge;
volatile static uint8_t idle = 1;
static uint16_t duration[MAX_SAMPLES];
static uint8_t in_ptr;
static uint8_t out_ptr;

/* ------------------------------------------------------------------------- */

usbMsgLen_t usbFunctionSetup(uchar data[8])
{
usbRequest_t    *rq = (void *)data;

    if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS){    /* class request type */
        if(rq->bRequest == USBRQ_HID_GET_REPORT){  /* wValue: ReportType (highbyte), ReportID (lowbyte) */
			usbMsgPtr = (void *)&report_buf;
			return sizeof(report_buf);
        }
		else if(rq->bRequest == USBRQ_HID_SET_REPORT) {
			return USB_NO_MSG; /* pass on to usbFunctionWrite() */
		}
    }else{
        /* no vendor specific requests implemented */
    }
    return 0;   /* default for not implemented requests: return no data back to host */
}

usbMsgLen_t	usbFunctionWrite(uint8_t *data, uint8_t len)
{
	if(ledState == data[0]) { /* same value as present */
		return 1; 
	}
	else {
		ledState = data[0];
	}
	return 1;
}
/* ------------------------------------------------------------------------- */


static void Timer0_Init(void)
{
	TIMSK |= (1 << 0);  // Enable Timer0 Overflow interrupt

	/* Start Timer0 */
	TCCR0 = TCCR0_VAL;
}

static void rc_init(void)
{
	Timer0_Init();
	
	/* External INT1 */
	MCUCR |= (1 << ISC11); /* Falling edge generates interrupt */
	MCUCR &= ~(1 << ISC10);
	GICR |= (1 << INT1); /* Enable INT1 interrupt */

	edge = 0; /* Initialiy look for falling edge */
	timer0_ovf = 0;
	idle = 1;
}




int main(void)
{
	uchar   i;

    wdt_enable(WDTO_1S);
    /* Even if you don't use the watchdog, turn it off here. On newer devices,
     * the status of the watchdog (on/off, period) is PRESERVED OVER RESET!
     */
    /* RESET status: all port bits are inputs without pull-up.
     * That's the way we need D+ and D-. Therefore we don't need any
     * additional hardware initialization.
     */
    usbInit();
	rc_init();
    usbDeviceDisconnect();  /* enforce re-enumeration, do this while interrupts are disabled! */
    i = 0;
    while(--i) {             /* fake USB disconnect for > 250 ms */
        wdt_reset();
        _delay_ms(1);
    }
    usbDeviceConnect();
	RED_LED_OUT();
	RED_LED_OFF();
    sei();
    for(;;) {                /* main event loop */
        wdt_reset();
        usbPoll();
		if(usbInterruptIsReady()) {
			report_buf[0].start = idle;
			if(out_ptr < in_ptr) {
				report_buf[0].duration = duration[out_ptr++];
				id++;
			}
			report_buf[0].id = id;
			report_buf[1].start = idle;
			if(out_ptr < in_ptr) {
				report_buf[1].duration = duration[out_ptr++];
				id++;
			}
			report_buf[1].id = id;
			usbSetInterrupt((void *)&report_buf[0], sizeof(report_buf));
		}
		if(idle == 1) {
			id = 0;
			in_ptr = 0;
			out_ptr = 0;
		}
    }
}


/* ISR for INT1 external interrupt */
ISR(INT1_vect)
{
	uint8_t count;
	int16_t dur;
	
	RED_LED_TOGGLE();
	if(!idle) {
		count = TCNT0;
		if(in_ptr < MAX_SAMPLES) {
			if((timer0_ovf > 0x80) || (ovf_2)) {
				timer0_ovf = 0x7F;
				count = 0xFF;
			}
			dur = (int16_t)((timer0_ovf << 8) + count);
			if(edge == 1) { /* This is rising edge, so store duration as negative */
				dur = -dur;
			}
			duration[in_ptr++] = dur;
		}
	}
	idle = 0;
	TCNT0 = 0;
	timer0_ovf = 0;
	ovf_2 = 0;
	if(edge == 0) {
		/* This was falling edge; next rising edge */
		MCUCR |= (1 << ISC10);
		edge = 1;
	}
	else {
		/* This was rising edge; next falling edge */
		MCUCR &= ~(1 << ISC10); 
		edge = 0;
	}
}

/* ISR for Timer0 overflow interrupt */
ISR(TIMER0_OVF_vect)
{
	timer0_ovf++;
	if(!timer0_ovf) { /* second overflow */
		ovf_2++;
		RED_LED_OFF();
	}
	
	/* Idle for ~1.5s (256 x 256 x 5.33us x 4) -> Wrap buffer to intial state
		This is the time given to send all of the buffer to USB (256 samples)
		Now capture a new sequence starting with a falling edge
	*/
	if(ovf_2 == 4) {
		idle = 1;
		edge = 0;
		MCUCR &= ~(1 << ISC10);
	}
}


/* ------------------------------------------------------------------------- */
