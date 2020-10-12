#pragma once
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/power.h>

ISR(WDT_vect){}

void deepSleep(int seconds)
{
  Serial.println(String(F("going to deep sleep for ")) + seconds + F(" seconds"));
  Serial.flush();
  for (int i = 0; i < seconds; i+=9)
  {
    wdt_disable();
  
    // disable ADC
    int oldADCSRA = ADCSRA;
    ADCSRA = 0;  
  
    /* Setup des Watchdog Timers */
    MCUSR &= ~(1<<WDRF);             /* WDT reset flag loeschen */
    WDTCSR |= (1<<WDCE) | (1<<WDE);  /* WDCE setzen, Zugriff auf Presclaler etc. */
    WDTCSR = 1<<WDP0 | 1<<WDP3;      /* Prescaler auf 8.0 s */
    WDTCSR |= 1<<WDIE;               /* WDT Interrupt freigeben */
    wdt_reset();  // pat the dog
    
    set_sleep_mode (SLEEP_MODE_PWR_DOWN);  
    sleep_enable();
    
    power_adc_disable();    /* Analog-Eingaenge abschalten */
    power_spi_disable();    /* SPI abschalten */
    power_timer0_disable(); /* Timer0 abschalten */
    power_timer2_disable(); /* Timer0 abschalten */
    power_twi_disable();    /* TWI abschalten */
    
    //sleep_cpu ();
    sleep_mode();
    // cancel sleep as a precaution
    sleep_disable();
    power_all_enable();     /* Komponenten wieder aktivieren */

    ADCSRA = oldADCSRA;
  }
}
