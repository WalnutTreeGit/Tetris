/*
 * main.c
 *
 * Created: 23/11/2022 11:33:19
 * Author : joaopaulo
 */ 
#include <time.h>
#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "light_ws2812.h"
#include "tetris.h"
#include <util/delay.h>
#include <avr/pgmspace.h>
#define pgm_read_byte_far(address_long) __ELPM((\fBuint32_t\fP)(address_long))

cRGB led_off = {.r = 0, .g = 0, .b = 0};
	
volatile unsigned char button;
volatile int countspeed;
volatile int startgame = 0;

byte pause = 0;

// Configure Matrix to always go from left to right, contrary to hardware direction
// Save in flash memory
const const int matrix[row_total][column] PROGMEM = 
{
{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11},
{23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12},
{24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35},
{47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36},
{48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59},
{71, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60},
{72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83},
{95, 94, 93, 92, 91, 90, 89, 88, 87, 86, 85, 84},
{96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107},
{119, 118, 117, 116, 115, 114, 113, 112, 111, 110, 109, 108},
{120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131},
{143, 142, 141, 140, 139, 138, 137, 136, 135, 134, 133, 132},
{144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155},
{167, 166, 165, 164, 163, 162, 161, 160, 159, 158, 157, 156},
{168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179},
{191, 190, 189, 188, 187, 186, 185, 184, 183, 182, 181, 180},
{192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203},
{215, 214, 213, 212, 211, 210, 209, 208, 207, 206, 205, 204},
{216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227},
{239, 238, 237, 236, 235, 234, 233, 232, 231, 230, 229, 228},
{240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251},
{263, 262, 261, 260, 259, 258, 257, 256, 255, 254, 253, 252},
{264, 265, 266, 267, 268, 269, 270, 271, 272, 273, 274, 275},
{287, 286, 285, 284, 283, 282, 281, 280, 279, 278, 277, 276},
{288, 289, 290, 291, 292, 293, 294, 295, 296, 297, 298, 299}};
	
// Configure colors by RGB code
struct cRGB colors[10]=
{
	{.r = 150 * brightness, .g = 150 * brightness, .b = 150 * brightness}, // 0 white
	{.r = 255 * brightness, .g = 000 * brightness, .b = 000 * brightness}, // 1 red
	{.r = 255 * brightness, .g = 100 * brightness, .b = 000 * brightness}, // 2 orange
	{.r = 255 * brightness, .g = 255 * brightness, .b = 000 * brightness}, // 3 "yellow"
	{.r = 000 * brightness, .g = 255 * brightness, .b = 000 * brightness}, // 4 green
	{.r = 000 * brightness, .g = 100 * brightness, .b = 255 * brightness}, // 5 light blue (t�rkis)
	{.r = 000 * brightness, .g = 000 * brightness, .b = 255 * brightness}, // 6 blue
	{.r = 100 * brightness, .g = 000 * brightness, .b = 255 * brightness}, // 7 violet
	{.r = 255 * brightness, .g = 000 * brightness, .b = 102 * brightness}, // 8 pink
	{.r = 166 * brightness, .g = 077 * brightness, .b = 255 * brightness}  // 9 light violet
};	
	
volatile unsigned int count_1000 = 0, flag_LED = 1, duty = 0;
unsigned int aux = 0;
float aux1 = 0;
	
typedef struct USARTRX
{
	char receiver_buffer;
	unsigned char status;
	unsigned char receive:	1;
	unsigned char error:	1;
	
}USARTRX_st;

volatile USARTRX_st rxUSART = {0, 0, 0, 0};
char transmit_buffer[10];

void init()
{
	DDRB|=_BV(ws2812_pin);  // (1 << ws2812_pin)
	clearTable();
	menu();
	_delay_ms(ws2812_resettime);
	
	
	//Config CTC LED 1Hz
	DDRB |= (1<<DDB1);		// Config PORTB Pin B1 as Output
	//DDRB = 0b00000010;	// Another way of doing the previous line

	PORTB = 0b00000010;		// Pin B1 ON
	//PORTD |= (1<<PORTD5);

	OCR0A = 39;				// OCR0A = (T * fclk_IO / Prescaler) - 1 ~= 39; T = 0.005s = 5ms; fclk_IO = 8000000Hz = 8MHz; Prescaler = 1024

	TCCR0A = 0b00000010;	// TCCR0A - CTC in Normal mode
	//TCCR0A |= (1<<WGM01);

	TCCR0B = 0b11000101;	// TCCR0B - Force Output Compare A & B; Prescaler 1024
	//TCCR0B |= (1<<FOC0A)|(1<<FOC0B)|(1<<CS02)|(1<<CS00);

	TIMSK0 |= (1<<OCIE0A);	// TIMSK0 - Output Compare Match A Interrupt Enabled
	//TIMSK0 = 0b00000010;
	
	
	//Config Bluetooth
	UBRR0H = 0;
	UBRR0L = 103;									// Set baud rate to 9600 with f = 8MHz
	UCSR0A = (1<<U2X0);								//Asynchronous Double speed
	UCSR0B |= (1<<RXCIE0)|(1<<RXEN0)|(1<<TXEN0);	// Interrupt enable, Enable receive and transmit
	UCSR0C |= (1<<UCSZ01)|(1<<UCSZ00);				//8-bit configuration
	
	sei();	
}

ISR(USART_RX_vect)
{
	rxUSART.status = UCSR0A;
	
	if ( rxUSART.status & ((1<<FE0) | (1<<DOR0) | (1>>UPE0)))
	rxUSART.error = 1;
	
	rxUSART.receiver_buffer = UDR0;
	rxUSART.receive = 1;
	
	button = rxUSART.receiver_buffer;
}

ISR(TIMER0_COMPA_vect) {	// Timer 0
	count_1000++;
	countspeed++;
	
	if (count_1000 == 100)	// When Timer 0 counts to 100, time = 0.005 * 100 = 0.5s = 500ms
	{
		count_1000 = 0;		// Resets Timer

		if (flag_LED == 1)	// If LED ON
		{
			PORTB = 0b00000000;
			flag_LED = 0;	// Turns LED OFF
		}
		else				// If LED OFF
		{
			PORTB = 0b00000010;
			flag_LED = 1;	// Turns LED ON
		}
	}
	
	// 200 * 0.005 * 200 = 1s
	if (countspeed >= falltime && startgame == 1 && pause == 0) 
	{
		shiftActiveBrick('d');
		countspeed = 0;						// Reset Timer
	}
}

int main(void)
{
	while(1)
	{
		init();
		_delay_ms(1000);
		
		// Waits for a button to be pressed for game to start
		while (button != 'a' && button != 's' && button != 'd' && button != 'j'  && button != 'k'  && button != 'l' );
		startgame = 1;
		clearTable();
		
		// Resets button so it does not perform action
		button = ' ';
		newActiveBrick();
		while(1)
		{
			// Checks for pressed button
			switch (button)
			{
				// Left
				case 'a':
				shiftActiveBrick('l');
				break;

				// Right
				case 'd':
				shiftActiveBrick('r');
				break;
			
				// Down
				case 's':
				shiftActiveBrick('d');
				break;
			
				// Rotate
				case 'j':
				rotateActiveBrick();
				break;
			
				// Force down
				case 'k':
				forcedown();
				break;
			
				// Pause
				case 'p':
				pause += 1;
				pause = Pause(pause);
				break;
			}
			checkFullLines();
			button = ' ';
			_delay_ms(50);
			if (tetrisGameOver) 
			{
				falltime = 200;
				clearTable();
				clearField();
				tetrisGameOver = 0;
				startgame = 0;
				break;
			}
		}
	}
}