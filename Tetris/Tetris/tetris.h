#include <avr/io.h>
#include <avr/interrupt.h>
#include "light_ws2812.h"
#include <util/delay.h>

#define MAXPIX 300
#define COLORLENGTH (MAXPIX/2)
#define FADE (256/COLORLENGTH)
#define row 18
#define column 12
#define brightness 0.25		//PERGUNTAR DO PROF, como fazer define de lá aparecer aqui
#define row_title 15
#define size_word 5
#define size_piece 4
int space_between = 1; 

struct cRGB colors[8];
struct cRGB led[MAXPIX];
cRGB matrix[column][row];
cRGB led_off;				//PERGUNTAR DO PROF, como inicializar a 0 .r .g .b para que sejam globais, mesma coisa com colors[]
typedef uint8_t byte;



/*

cRGB colors[0].r=150 * brightness, colors[0].g=150 * brightness, colors[0].b=150 * brightness;
cRGB colors[1].r=255 * brightness, colors[1].g=000 * brightness, colors[1].b=000 * brightness;//red
cRGB colors[2].r=255 * brightness, colors[2].g=100 * brightness, colors[2].b=000 * brightness;//orange
cRGB colors[3].r=100 * brightness, colors[3].g=255 * brightness, colors[3].b=000 * brightness;//yellow
cRGB colors[4].r=000 * brightness, colors[4].g=255 * brightness, colors[4].b=000 * brightness;//green
cRGB colors[5].r=000 * brightness, colors[5].g=100 * brightness, colors[5].b=255 * brightness;//light blue (türkis)
cRGB colors[6].r=000 * brightness, colors[6].g=000 * brightness, colors[6].b=255 * brightness;//blue
cRGB colors[7].r=100 * brightness, colors[7].g=000 * brightness, colors[7].b=255 * brightness;//violet */

typedef struct Brick{
	byte enabled;//Brick is disabled when it has landed
	byte xpos, ypos;
	byte yOffset;//Y-offset to use when placing brick at top of field
	byte siz;
	byte pix[4][4];
	byte color;
} Brick;
Brick activeBrick;

typedef struct Blocks {
	byte yOffset;//Y-offset to use when placing brick at top of field
	byte siz;
	cRGB pix[4][4];
} Blocks_t;

Blocks_t brickLib[7] = {
	{
		1,//yoffset when adding brick to field
		4,
		{ {0, 0, 0, 0},
		{0, 1, 1, 0},
		{0, 1, 1, 0},
		{0, 0, 0, 0}
	}
},
{
	0,
	4,
	{ {0, 1, 0, 0},
	{0, 1, 0, 0},
	{0, 1, 0, 0},
	{0, 1, 0, 0}
}
},
{
	1,
	3,
	{ {0, 0, 0, 0},
	{1, 1, 1, 0},
	{0, 0, 1, 0},
	{0, 0, 0, 0}
}
},
{
	1,
	3,
	{ {0, 0, 1, 0},
	{1, 1, 1, 0},
	{0, 0, 0, 0},
	{0, 0, 0, 0}
}
},
{
	1,
	3,
	{ {0, 0, 0, 0},
	{1, 1, 1, 0},
	{0, 1, 0, 0},
	{0, 0, 0, 0}
}
},
{
	1,
	3,
	{ {0, 1, 1, 0},
	{1, 1, 0, 0},
	{0, 0, 0, 0},
	{0, 0, 0, 0}
}
},
{
	1,
	3,
	{ {1, 1, 0, 0},
	{0, 1, 1, 0},
	{0, 0, 0, 0},
	{0, 0, 0, 0}
}
}
};

void clearTable() 
{
	uint16_t i;
	for(i = 300; i > 0; i--)
	{
		led[i-1].r=0;led[i-1].g=0;led[i-1].b=0;
	}
	ws2812_setleds(led,300);
	_delay_us(ws2812_resettime);
}

void convert_matrix() // 
{
	for (int i = 0; i < row; i++)
	{
		for (int j = 0; j < column; j++)
		{
			led[i * row + j] = matrix[i][j]; // matrix(y,x), para ser (x,y) trocar i e j de posição
		}
	}
	_delay_us(ws2812_resettime);
}

void menu()
{
	colors[0].r=150 * brightness; colors[0].g=150 * brightness; colors[0].b=150 * brightness;
	colors[1].r=255 * brightness; colors[1].g=000 * brightness; colors[1].b=000 * brightness;//red
	colors[2].r=255 * brightness; colors[2].g=100 * brightness; colors[2].b=000 * brightness;//orange
	colors[3].r=100 * brightness; colors[3].g=255 * brightness; colors[3].b=000 * brightness;//yellow
	colors[4].r=000 * brightness; colors[4].g=255 * brightness; colors[4].b=000 * brightness;//green
	colors[5].r=000 * brightness; colors[5].g=100 * brightness; colors[5].b=255 * brightness;//light blue (türkis)
	colors[6].r=000 * brightness; colors[6].g=000 * brightness; colors[6].b=255 * brightness;//blue
	colors[7].r=100 * brightness; colors[7].g=000 * brightness; colors[7].b=255 * brightness;//violet
	
	for (int i = 0; i <= 3; i++)  
	{
		matrix[0][i] = colors[1]; // I deitada
	}
	
	for (int i = 0; i <= 2; i++) 
	{
		matrix[1][i] = colors[2];
		matrix[2][1] = colors[2]; // Triângulo 
	}
	
	for (int i = 4; i <= 5; i++)  
	{
		matrix[0][i] = colors[3];
		matrix[2][i - 1] = colors[3]; // Z do lado de I
	}
	
	for (int i = 5; i <= 7; i++)  
	{
		matrix[0][i] = colors[4];
		matrix[1][5] = colors[4];
		matrix[2][5] = colors[4];// L em pé
	}
	
	for (int i = 9; i <= 11; i++)  
	{
		matrix[0][i] = colors[4];
		matrix[1][11] = colors[4];// L deitado
		
		matrix[1][i] = colors[5];
	}
	convert_matrix();
}

byte I_piece[row * size_piece] = 
{
	0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,
	1,1,1,1,0,0,0,0,0,0,0,0,
};

/*
	//Compare mechanism, BROKEN
	
	for (int i = 0; i < size_piece; i++)
	{
		for (int j = 0; j < row; j++ )
		{
			if (I_piece[i * row + j])
			{
				matrix[row_title - i][j] = colors[7];
			}
		}
	}*/


void sliding_menu()
{
	colors[0].r=150 * brightness; colors[0].g=150 * brightness; colors[0].b=150 * brightness;
	colors[1].r=255 * brightness; colors[1].g=000 * brightness; colors[1].b=000 * brightness;//red
	colors[2].r=255 * brightness; colors[2].g=100 * brightness; colors[2].b=000 * brightness;//orange
	colors[3].r=100 * brightness; colors[3].g=255 * brightness; colors[3].b=000 * brightness;//yellow
	colors[4].r=000 * brightness; colors[4].g=255 * brightness; colors[4].b=000 * brightness;//green
	colors[5].r=000 * brightness; colors[5].g=100 * brightness; colors[5].b=255 * brightness;//light blue (türkis)
	colors[6].r=000 * brightness; colors[6].g=000 * brightness; colors[6].b=255 * brightness;//blue
	colors[7].r=100 * brightness; colors[7].g=000 * brightness; colors[7].b=255 * brightness;//violet
	
	led_off.r = 0; led_off.g = 0; led_off.b = 0;
	
	
	byte i = 0;
	// byte j = 0;
	while (1)
	{
		// T
		byte offset = 1;
		
		//matrix[row_title][offset + i] = colors[7];			//IMPAR
		//	//matrix[row_title][offset + i - 1] = led_off;
		
		//matrix[row_title][2 + i] = colors[7];			//IMPAR
		//	//matrix[row_title][2 + i - 1] = led_off;
		
		//matrix[row_title][3 + i] = colors[7];			//IMPAR
		//matrix[row_title][offset + i - 1] = led_off;
		
		
		matrix[row_title - 1][2 - i] = colors[5];				//PAR
		matrix[row_title - 1][2 - i + 1] = led_off;	
		
		//matrix[row_title - 2][2 + i] = colors[7];		//IMPAR
		//matrix[row_title - 2][2 + i - 1] = led_off;
		
		matrix[row_title - 3][2 - i] = colors[5];				//PAR
		matrix[row_title - 3][2 - i + 1] = led_off;	
		
		//matrix[row_title - 4][2 + i] = colors[7];		//IMPAR
		//matrix[row_title - 4][2 + i - 1] = led_off;
		
		
		// E
		offset += 3 + 1; 
		
		for (int e = 0; e < 3; e++)
		{
			matrix[row_title][offset + i + e] = colors[2];					// IMPAR
			matrix[row_title][offset + i - 1 + e] = led_off;
			
			matrix[row_title - 2][offset + i + e] = colors[2];					// IMPAR
			matrix[row_title - 2][offset + i - 1 + e] = led_off;			
			
			matrix[row_title - 4][offset + i + e] = colors[2];					// IMPAR
			matrix[row_title - 4][offset + i - 1 + e] = led_off;
			
		}
		
		matrix[row_title - 1][offset + i ] = colors[4];			// PAR
		matrix[row_title - 1][offset + i - 1 ] = led_off;
		
		matrix[row_title - 3][offset + i ] = colors[4];			// PAR
		matrix[row_title - 3][offset + i - 1 ] = led_off;
		
		
		
		// T
		offset = offset + 3 + 1;
		
		for (byte t = 0; t < 3; t++)
		{
			matrix[row_title][offset + i + t] = colors[7];			//IMPAR
			matrix[row_title][offset + i - 1 + t] = led_off;
			
			matrix[row_title][1 + 1 + i] = colors[7];			//IMPAR
			matrix[row_title][2 + i - 1] = led_off;
			
			matrix[row_title][3 + i] = colors[7];			//IMPAR
			matrix[row_title][3 + i - 1] = led_off;
		}
		
		
		
		
		
		
		matrix[row_title - 1][2 - i] = colors[5];				//PAR
		matrix[row_title - 1][2 - i + 1] = led_off;
		
		matrix[row_title - 2][2 + i] = colors[7];		//IMPAR
		matrix[row_title - 2][2 + i - 1] = led_off;
		
		matrix[row_title - 3][2 - i] = colors[5];				//PAR
		matrix[row_title - 3][2 - i + 1] = led_off;
		
		matrix[row_title - 4][2 + i] = colors[7];		//IMPAR
		matrix[row_title - 4][2 + i - 1] = led_off;
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		i++;
		
		if ( i == column ) 
		{
			i = 0;
		}
		
		convert_matrix();
		ws2812_setleds(led,MAXPIX);
		_delay_ms(1000);
	}
	
	
	
	
	
	
	
	
	
	convert_matrix();
}

void sliding_menu2()
{
	colors[0].r=150 * brightness; colors[0].g=150 * brightness; colors[0].b=150 * brightness;//white
	colors[1].r=255 * brightness; colors[1].g=000 * brightness; colors[1].b=000 * brightness;//red
	colors[2].r=255 * brightness; colors[2].g=100 * brightness; colors[2].b=000 * brightness;//orange
	colors[3].r=100 * brightness; colors[3].g=255 * brightness; colors[3].b=000 * brightness;//yellow
	colors[4].r=000 * brightness; colors[4].g=255 * brightness; colors[4].b=000 * brightness;//green
	colors[5].r=000 * brightness; colors[5].g=100 * brightness; colors[5].b=255 * brightness;//light blue (türkis)
	colors[6].r=000 * brightness; colors[6].g=000 * brightness; colors[6].b=255 * brightness;//blue
	colors[7].r=100 * brightness; colors[7].g=000 * brightness; colors[7].b=255 * brightness;//violet
	colors[8].r=255 * brightness; colors[7].g=000 * brightness; colors[7].b=102 * brightness;//pink
	colors[9].r=166 * brightness; colors[7].g=077 * brightness; colors[7].b=255 * brightness;//light violet
	
	led_off.r = 0; led_off.g = 0; led_off.b = 0;
	
	byte i = 0;
	// byte j = 0;
	while (1)
	{
		// T  
		
		int offset_impar = 1;
		int offset_par = (column - 1) - offset_impar;
		
		for (byte T = 0; T < 5; T++)
		{
			matrix[row_title][offset_impar + i + T] = colors[7];		// IMPAR
		}
		matrix[row_title][offset_impar + i - 1] = led_off;
	
		offset_impar += 2; // 3
		offset_par = (column - 1) - offset_impar;
		
		matrix[row_title - 1][offset_par - i] = colors[5];			// PAR
		matrix[row_title - 1][offset_par - i + 1] = led_off;
		
		matrix[row_title - 2][offset_impar + i] = colors[7];			// IMPAR
		matrix[row_title - 2][offset_impar + i - 1] = led_off;
		
		matrix[row_title - 3][offset_par - i] = colors[5];			// PAR
		matrix[row_title - 3][offset_par - i + 1] = led_off;
		
		matrix[row_title - 4][offset_impar + i] = colors[7];			// IMPAR
		matrix[row_title - 4][offset_impar + i - 1] = led_off;
		
		// E
		
		offset_impar = -3; 
		offset_par = (column - 1) + 1; // 12
		
		for (int e = 0; e < 3; e++)
		{
			matrix[row_title][offset_impar + i + e] = colors[2];		// IMPAR
		
			matrix[row_title - 2][offset_impar + i + e] = colors[2];	// IMPAR
			
			matrix[row_title - 4][offset_impar + i + e] = colors[2];	// IMPAR
		}
		matrix[row_title][offset_impar + i - 1] = led_off;
		matrix[row_title - 2][offset_impar + i - 1] = led_off;
		matrix[row_title - 4][offset_impar + i - 1] = led_off;
		
		matrix[row_title - 1][offset_par - i ] = colors[4];			// PAR
		matrix[row_title - 1][offset_par - i - 1 ] = led_off;
		
		matrix[row_title - 3][offset_par - i ] = colors[4];			// PAR
		matrix[row_title - 3][offset_par - i - 1 ] = led_off;
		
		// T
		
		offset_impar = -9;
		offset_par = (column - 1) - offset_impar;
		
		for (byte T = 0; T < 5; T++)
		{
			matrix[row_title][offset_impar + i + T] = colors[7];		// IMPAR
		}
		matrix[row_title][offset_impar + i - 1] = led_off;
		
		offset_impar += 2; // -7
		offset_par = (column - 1) - offset_impar;
		
		matrix[row_title - 1][offset_par - i] = colors[5];			// PAR
		matrix[row_title - 1][offset_par - i + 1] = led_off;
		
		matrix[row_title - 2][offset_impar + i] = colors[7];			// IMPAR
		matrix[row_title - 2][offset_impar + i - 1] = led_off;
		
		matrix[row_title - 3][offset_par - i] = colors[5];			// PAR
		matrix[row_title - 3][offset_par - i + 1] = led_off;
		
		matrix[row_title - 4][offset_impar + i] = colors[7];			// IMPAR
		matrix[row_title - 4][offset_impar + i - 1] = led_off;
		
		// R
		
		offset_impar = -13; //último na parte de cima de R
		offset_par = (column - 1) - offset_impar ; //24
		
		for (byte r = 0; r < 3; r++)
		{
			matrix[row_title][offset_impar + i + r] = colors[3];		// IMPAR	
		
			if (r < 2)
			{
				matrix[row_title - 2][offset_impar + i + r] = colors[3];// IMPAR	
			}
		}
		matrix[row_title][offset_impar + i - 1] = led_off;
		matrix[row_title - 2][offset_impar + i - 1] = led_off;
		
		matrix[row_title - 1][offset_par - i] = colors[6];			// PAR
		matrix[row_title - 1][offset_par - i + 1] = led_off;
		
		matrix[row_title - 1][offset_par - i - 2] = colors[6];		// PAR
		matrix[row_title - 1][offset_par - i - 2 + 1] = led_off;
		
		matrix[row_title - 4][offset_impar + i - 1] = colors[3];		// IMPAR
		matrix[row_title - 4][offset_impar + i - 1 - 1] = led_off;
		
		matrix[row_title - 4][offset_impar + i + 2] = colors[3];		// IMPAR
		matrix[row_title - 4][offset_impar + i + 2 - 1] = led_off;
		
		// I
		
		offset_impar = -16;
		offset_par = (column - 1) - offset_impar ; //27
		
		for ( byte I = 0; I < 5; I++)
		{
			if (I%2)
			{
				matrix[row_title - I][offset_par - i] = colors[0];	// PAR
				matrix[row_title - I][offset_par + i + 1] = led_off;
			}
			else 
			{
				matrix[row_title - I][offset_impar + i] = colors[0];	// IMPAR
				matrix[row_title - I][offset_impar + i - 1] = led_off;
			}	
		}
		
		// S
		
		offset_impar = -20;
		offset_par = (column - 1) - offset_impar ; //31
		
		for (int s = 0; s < 3; s++)
		{
			matrix[row_title][offset_impar + i + s] = colors[8];		// IMPAR
			
			matrix[row_title - 2][offset_impar + i + s] = colors[8];	// IMPAR
			
			matrix[row_title - 4][offset_impar + i + s] = colors[8];	// IMPAR
		}
		matrix[row_title][offset_impar + i - 1] = led_off;
		matrix[row_title - 2][offset_impar + i - 1] = led_off;
		matrix[row_title - 4][offset_impar + i - 1] = led_off;
		
		matrix[row_title - 1][offset_par - i - 2] = colors[4];		// PAR
		matrix[row_title - 1][offset_par - i - 2 - 1] = led_off;
		
		matrix[row_title - 3][offset_par - i] = colors[4];			// PAR
		matrix[row_title - 3][offset_par - i - 1] = led_off;
		
		i++;
		
		if ( i == (column - 1) + 31 ) 
		{
			i = 0;
		}
		convert_matrix();
		ws2812_setleds(led,MAXPIX);
		_delay_ms(1000);
	}
}
