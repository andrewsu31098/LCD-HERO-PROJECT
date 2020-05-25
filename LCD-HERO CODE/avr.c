#include "avr.h"

void
avr_init(void)
{
  /* WDTCR = 15; uncomment this if you know how Watch Dog Timers work */
}

void
avr_wait(unsigned short microsec)
{
  TCCR0 = 1;
  while (microsec--) {
    TCNT0 = (unsigned char)(256 - (XTAL_FRQ) * 0.000001);
    SET_BIT(TIFR, TOV0);
    WDR();
    while (!GET_BIT(TIFR, TOV0));
  }
  TCCR0 = 0;
}

void waitLongerByTen(int waitTime){
	for (int i = 0; i<10; i++){
		avr_wait(waitTime);
	}
}