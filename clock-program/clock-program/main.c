/*
 * clock-program.c
 *
 * Created: 1/29/2026 6:46:44 PM
 * Author : Kushal Prasad Joshi
 *
 * Description:
 * This program implements a digital clock (MM:SS) using ATmega32.
 * A 4-digit multiplexed 7-segment display is driven using PORTB and PORTC.
 * Timer1 is configured in CTC mode to generate a 1-second interrupt.
 */

#define F_CPU 1000000UL   // CPU frequency = 1 MHz
// Using internal RC oscillator with CKDIV8 enabled
// This definition is REQUIRED for correct delay calculations

#include <avr/io.h>        // AVR register definitions
#include <avr/interrupt.h>// Interrupt handling
#include <util/delay.h>   // Delay functions (_delay_ms)

/*
 * volatile is VERY IMPORTANT here.
 *
 * Why volatile?
 * -------------
 * 'seconds' and 'minutes' are modified inside an ISR (Interrupt Service Routine)
 * and also read inside the main program.
 *
 * Without 'volatile', the compiler may assume these variables never change
 * unexpectedly and may optimize them incorrectly.
 *
 * volatile tells the compiler:
 * "These variables can change at any time - always read them from memory."
 */
volatile uint8_t seconds = 0;
volatile uint8_t minutes = 0;

/*
 * Lookup table for 7-segment display
 *
 * Each value represents segments a-g for digits 0-9
 * Bit pattern corresponds to:
 * bit0 = a, bit1 = b, ..., bit6 = g
 *
 * Example:
 * 0x3F -> 0b00111111 -> displays digit '0'
 */
uint8_t seg_code[10] = {
	0x3F,  // 0
	0x06,  // 1
	0x5B,  // 2
	0x4F,  // 3
	0x66,  // 4
	0x6D,  // 5
	0x7D,  // 6
	0x07,  // 7
	0x7F,  // 8
	0x6F   // 9
};

/*
 * TIMER1 Compare Match Interrupt Service Routine
 *
 * This ISR is executed every 1 second.
 * It increments seconds and minutes like a real clock.
 */
ISR(TIMER1_COMPA_vect)
{
	seconds++;                 // Increment seconds every interrupt

	if (seconds == 60) {        // If 60 seconds reached
		seconds = 0;            // Reset seconds
		minutes++;              // Increment minutes

		if (minutes == 60)      // If 60 minutes reached
			minutes = 0;        // Reset minutes
	}
}

/*
 * Timer1 initialization function
 *
 * Configures Timer1 to generate an interrupt every 1 second
 */
void timer1_init(void)
{
	TCCR1B |= (1 << WGM12);     // Configure Timer1 in CTC mode
	// CTC = Clear Timer on Compare Match

	OCR1A = 976;               // Compare value for 1 second
	/*
	 * Calculation:
	 * Timer frequency = 1 MHz / 1024 = 976.56 Hz
	 * 1 second ? 976 counts
	 */

	TCCR1B |= (1 << CS12) | (1 << CS10);
	// Prescaler = 1024

	TIMSK |= (1 << OCIE1A);    // Enable Timer1 Compare Match A interrupt
}

/*
 * Function to display a single digit at a given position
 *
 * digit : number to display (0-9)
 * pos   : digit position (0 to 3)
 *
 * Multiplexing technique is used:
 * - One digit is enabled at a time
 * - Digits are switched fast enough to appear continuous
 */
void display_digit(uint8_t digit, uint8_t pos)
{
	// Activate only the required display (PC0-PC3 are active LOW)
	PORTC = (PORTC & 0xF0) | (~(1 << pos) & 0x0F);

	// Output segment pattern to PORTB
	PORTB = seg_code[digit];

	// Small delay for persistence of vision
	_delay_ms(5);
}

int main(void)
{
	/*
	 * PORTB:
	 * PB0-PB6 -> segments a-g (output)
	 */
	DDRB = 0x7F;   // 0b01111111 ? PB0-PB6 as output

	/*
	 * PORTC:
	 * PC0-PC3 -> digit enable lines (output)
	 */
	DDRC = 0x0F;   // 0b00001111 ? PC0-PC3 as output

	// Initial states
	PORTB = 0x00;  // Turn OFF all segments
	PORTC = 0x0F;  // Disable all displays (active LOW)

	timer1_init(); // Initialize Timer1
	sei();          // Enable global interrupts

	while (1)
	{
		/*
		 * Display format:
		 * [MM][SS]
		 *
		 * Digit positions:
		 * pos 0 ? minutes tens
		 * pos 1 ? minutes units
		 * pos 2 ? seconds tens
		 * pos 3 ? seconds units
		 */

		display_digit(minutes / 10, 0);
		display_digit(minutes % 10, 1);
		display_digit(seconds / 10, 2);
		display_digit(seconds % 10, 3);
	}
}
