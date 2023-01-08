/*
 *
 * This example is configured for a Atmega32 at 16MHz
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include "light_ws2812.h"
#include <util/delay.h>
#include "tetris.h"

// TEST GIT

#define MAXPIX 300
#define COLORLENGTH (MAXPIX/2)
#define FADE (256/COLORLENGTH)
#define row 18
#define column 12
#define brightness 0

struct cRGB colors[8];
struct cRGB led[MAXPIX];
 
void init()
{
	DDRB|=_BV(ws2812_pin);  // (1 << ws2812_pin)
	
	clearTable();
	//menu();
	sliding_menu2();
}

void fall()
{
	
	colors[5].r=000; colors[5].g=150; colors[5].b=255;//light blue (türkis)
	int k;
	for ( k = 0; k < row; k++)
	{
		int i = 209 - 12 * k; // base * 
		led[i] = colors[5];
		ws2812_setleds(led,MAXPIX);
		led[i].r = 0;
		led[i].b = 0;
		led[i].g = 0;
		_delay_ms(1000);
	}
}

int main(void)
{	
	init();
	
	colors[5].r=000; colors[5].g=150; colors[5].b=255;//light blue (türkis)
	
    //Rainbowcolors
    colors[0].r=100 * brightness; colors[0].g=255 * brightness; colors[0].b=000;//yellow
    colors[1].r=255; colors[1].g=255; colors[1].b=255;//green
	colors[5].r=000 * brightness; colors[5].g=150 * brightness; colors[5].b=255 * brightness;//light blue (türkis)
/*
	led[1].r = 33 * brightness;
		
	
	matrix[1][3] = colors[0];
	matrix[1][3 + 1] = colors[0];
	/*
	matrix[1][15] = colors[0];
	matrix[1][3 + column] = colors[0];
	*/
		
	// ERRO matrix[3][0] = brickLib[1].block;
	/*
	for (int i = 0; i < row; i++)
	{
		for (int j = 0; j < column; j++)
		{
			led[i * row + j] = brickLib->block[j][i]; // matrix(x,y), para ser (y,x) trocar i e j de posição
		}
	}*/

/*
	for (int i = 0; i < row; i++)
	{
		for (int j = 0; j < column; j++)
		{
			matrix[j][i] = brickLib[1].pix[i][j]; 
		}
	}*/
	
//	fall();


	byte activeBrickPix = activeBrick.pix;
	
	ws2812_setleds(led,MAXPIX);
	_delay_ms(100);
	
	
	/* TEST!! 
	
	matrix[1][-1] = colors[1];
	convert_matrix();
	
	Verificar se acende após 13--
	*/
	
	
	
	
	while(1)
    {
	
	 
	 
	ws2812_setleds(led,MAXPIX);
	_delay_us(ws2812_resettime);
    }
}