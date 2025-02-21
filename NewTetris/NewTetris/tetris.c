/*
*
* tetris.c
*
*/
#include "tetris.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include "light_ws2812.h"
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

//Field is "imaginary", it is not the board, it can be if printField() is called, the the *board* becomes the field pix 

const int PROGMEM randombrick[28] = {4, 0, 3, 5, 1, 6, 3, 1, 4, 3, 2, 0, 6, 5, 0, 5, 2, 6, 1, 3, 4, 0, 6, 1, 3, 5, 2, 4}; // random numbers from 0-6
const int PROGMEM randomcolor[40] = {4, 0, 7, 3, 6, 2, 5, 9, 8, 1, 8, 9, 2, 4, 0, 1, 6, 7, 5, 3, 8, 1, 3, 2, 5, 7, 4, 0, 9, 6, 0, 5, 9, 8, 2, 1, 7, 6, 3, 4}; // random numbers from 0-9
	
volatile int falltime = 200;	// 200 * 0.005 = 1s
byte random6 = 0;
byte random9 = 0;
byte tetrisGameOver = 0;
byte selectedBrick = 0;
byte selectedColor = 0;
byte I_rot = 0;

typedef struct Field {
	byte pix[row][column];		//Field matrix
	byte color[row][column];	// Color for each spot
} Field;
Field field;

typedef struct Brick {
  byte enabled;					//Brick is disabled when it has landed
  uint8_t xpos, ypos;
  byte yOffset;					//Y-offset to use when placing brick at top of field
  byte siz;
  byte pix[4][4];				//Matrix for each Brick's design
  byte color;
} Brick;
Brick activeBrick;
Brick tmpBrick;

typedef struct AbstractBrick {
	byte yOffset;				//Y-offset to use when placing brick at top of field
	byte siz;					// Size will be used for reading Brick's pix and rotating
	int pix[4][4];				//For each Brick's design
}AbstractBrick; 

AbstractBrick brickLib[8] = {
	{
		1,						// Y offset when adding brick to field
		2,						// size
		{ 
			{1, 1, 0, 0},		// brick design
			{1, 1, 0, 0},
			{0, 0, 0, 0},
			{0, 0, 0, 0}
		}
	},
	{
		3,
		4,
		{ 
			{1, 0, 0, 0},
			{1, 0, 0, 0},
			{1, 0, 0, 0},
			{1, 0, 0, 0}
		}
	},
	{
		1,
		3,
		{ 
			{1, 1, 1, 0},
			{0, 0, 1, 0},
			{0, 0, 0, 0},
			{0, 0, 0, 0}
		}
	},
	{
		1,
		3,
		{ 
			{0, 0, 1, 0},
			{1, 1, 1, 0},
			{0, 0, 0, 0},
			{0, 0, 0, 0}
		}
	},
	{
		1,
		3,
		{ 
			{1, 1, 1, 0}, 
			{0, 1, 0, 0}, 
			{0, 0, 0, 0},
			{0, 0, 0, 0}
		}
	},
	{
		1,
		3,
		{ 
			{0, 1, 1, 0},
			{1, 1, 0, 0},
			{0, 0, 0, 0},
			{0, 0, 0, 0}
		}
	},
	{
		1,
		3,
		{ 
			{1, 1, 0, 0},
			{0, 1, 1, 0},
			{0, 0, 0, 0},
			{0, 0, 0, 0}
		}
	},
	{
		1,
		4,
		{
			{1, 1, 1, 1},
			{0, 0, 0, 0},
			{0, 0, 0, 0},
			{0, 0, 0, 0}
		}
	},
};


// Pauses game
byte Pause(byte paus)
{
	if (paus > 1)
	{
		paus = 0;
		printField();
		return paus;
	}
	clearTable();
	return paus;
}

void addActiveBrickToField()
{
	byte fx, fy;					// used to check if brick is within bounds, no trespassing outside
	for (byte by = 0; by < 4; by++)
	{
		for (byte bx = 0; bx < 4; bx++)
		{
			fx = activeBrick.xpos + bx;
			fy = activeBrick.ypos + by;
			
			//Check if Hitbox inside playing field
			if (fx >= 0 && fy >= 0 && fx < column && fy < row && activeBrick.pix[by][bx]) { 
				field.pix[fy][fx] = activeBrick.pix[by][bx];
				field.color[fy][fx] = activeBrick.color;
				activeBrick.pix[by][bx] = 0;
			}
		}
	}
	I_rot = 0;
	_delay_us(ws2812_resettime);
}

//Check vertical collision with sides of field
int checkSidesCollision(struct Brick* brick) 
{	
	byte fx; 
	for (byte by = 0; by < 4; by++) 
	{
		for (byte bx = 0; bx < 4; bx++) 
		{
			if ((*brick).pix[by][bx]) 
			{
				fx = (*brick).xpos + bx;		//Determine the brick.pix position on the field
				if (fx < 0 || fx >= column) 
				{
					return 1;
				}
			}
		}
	}
	return 0;
}

int checkFieldCollision(struct Brick* brick)
{
	   byte bx, by;
	   byte fx, fy;
	   for (by = 0; by < 4; by++) 
	   {
		   for (bx = 0; bx < 4; bx++) 
		   {
			   fx = (*brick).xpos + bx;
			   fy = (*brick).ypos + by;
			   
			   // Check if next led is on
			   if (( (*brick).pix[by][bx] == 1) && ( field.pix[fy][fx] == 1)) 
			   {
				   return 1;
			   }
		   }
	   }
	   return 0;
  }
	
//Clear space where the next brick is displayed
void clearNext()
{
	for (int x = 7; x < 12; x++)
	{
		for (int y = 20; y < 24; y++)
		{
			field.pix[y][x] = 0;
		}
	}
}

//Temporarily changes brick to next brick so it can be displayed at the top
void showNextPiece()
{
	//Reads value from flash memory for brick's shape and color
	selectedBrick = pgm_read_byte(&randombrick[random6]); // 0-6, 7 pieces
	selectedColor = pgm_read_byte(&randomcolor[random9]); // 0-9, 10 colors
	
	activeBrick.siz = brickLib[selectedBrick].siz;
	activeBrick.yOffset = brickLib[selectedBrick].yOffset;

	activeBrick.color = selectedColor;
	
	//Position where the next brick is displayed
	activeBrick.ypos = 20;
	activeBrick.xpos = 9;
	
	for (byte y = 0; y < 4; y++)
	{
		for (byte x = 0; x < 4; x++)
		{
			activeBrick.pix[x][y] = (brickLib[selectedBrick]).pix[x][y];
		}
	}
	addActiveBrickToField();
	_delay_us(ws2812_resettime);
}

void newActiveBrick() 
{
	//Reads value from flash memory for brick's shape and color
	selectedBrick = pgm_read_byte(&randombrick[random6]); // 0-6, 7 pieces
	selectedColor = pgm_read_byte(&randomcolor[random9]); // 0-9, 10 colors
	
	random6++;
	random9++;
	
	if (random6 > 6) random6 = 0;
	if (random9 > 9) random9 = 0; 
	
	activeBrick.siz = brickLib[selectedBrick].siz;
	activeBrick.yOffset = brickLib[selectedBrick].yOffset;
	
	activeBrick.xpos = column / 2 - activeBrick.siz / 2;		// To center
	
	activeBrick.ypos = 20 - 1 - activeBrick.yOffset;			//Top of the screen
	
	activeBrick.enabled = 1;

	activeBrick.color = selectedColor; 
	 
	// Copy pix array of selected Brick
	for (byte y = 0; y < 4; y++) 
	{
		for (byte x = 0; x < 4; x++) 
		{
			activeBrick.pix[y][x] = (brickLib[selectedBrick]).pix[y][x];
		}
	}
	
	// Temporary save active Brick's state before showing next brick
	Brick realBrick;
	realBrick = activeBrick;
	showNextPiece();
	activeBrick = realBrick;
	printField();
	clearNext();
	
	if (checkFieldCollision(&activeBrick)) 
	{
		tetrisGameOver = 1;
	}	
}

void printField() 
{
	byte activeBrickSpecificPix = 0;
		
	int x = 0;
	int y = 0;
				
	for (x = 0; x < column; x++) 
	{
		for (y = 0; y < row; y++) 
		{
			//Check if activebrick is on the screen
			if (activeBrick.enabled && (x >= activeBrick.xpos) && (x < (activeBrick.xpos + (activeBrick.siz)))
			&& (y >= activeBrick.ypos) && (y < (activeBrick.ypos + (activeBrick.siz)))) 
			{ 		
				activeBrickSpecificPix = activeBrick.pix[y - activeBrick.ypos][x - activeBrick.xpos]; // 1 or 0
			}
			
			if (field.pix[y][x]) 
			{
				led[pgm_read_byte(&matrix[y][x])] = colors[field.color[y][x]];	// if led is on, color remains the same so it does not get turned off
			}
			else if (activeBrickSpecificPix)									// Print Active Brick's pix 
			{
				led[pgm_read_byte(&matrix[y][x])] = colors[activeBrick.color];
			}		
			else 
			{
				led[pgm_read_byte(&matrix[y][x])] = led_off;					// Remaining unused blocks are off	
			}
			activeBrickSpecificPix = 0;
		}
	}
	ws2812_setleds(led,MAXPIX);
	_delay_us(ws2812_resettime);
}

void rotateActiveBrick() 
{
	//Copy active brick pix array to temporary pix array
	for (byte y = 0; y < 4; y++) 
	{
		for (byte x = 0; x < 4; x++) 
		{
			tmpBrick.pix[y][x] = activeBrick.pix[y][x];
		}
	}
	tmpBrick.xpos = activeBrick.xpos;
	tmpBrick.ypos = activeBrick.ypos;
	tmpBrick.siz = activeBrick.siz;

	//Depending on size of the active brick, we will rotate differently
	if (activeBrick.siz == 3) 
	{
		//Perform rotation around center pix
		tmpBrick.pix[0][0] = activeBrick.pix[0][2];
		tmpBrick.pix[0][1] = activeBrick.pix[1][2];
		tmpBrick.pix[0][2] = activeBrick.pix[2][2];
		tmpBrick.pix[1][0] = activeBrick.pix[0][1];
		tmpBrick.pix[1][1] = activeBrick.pix[1][1];
		tmpBrick.pix[1][2] = activeBrick.pix[2][1];
		tmpBrick.pix[2][0] = activeBrick.pix[0][0];
		tmpBrick.pix[2][1] = activeBrick.pix[1][0];
		tmpBrick.pix[2][2] = activeBrick.pix[2][0];
		//Keep other parts of temporary block clear
		tmpBrick.pix[0][3] = 0;
		tmpBrick.pix[1][3] = 0;
		tmpBrick.pix[2][3] = 0;
		tmpBrick.pix[3][3] = 0;
		tmpBrick.pix[3][2] = 0;
		tmpBrick.pix[3][1] = 0;
		tmpBrick.pix[3][0] = 0;
	} 
	// Square does not rotate
	else if (selectedBrick == 0)
	{
		return;
	}
	// I piece will change pix
	else if (activeBrick.siz == 4)
	{
		if (I_rot == 0)
		{
			for (byte y = 0; y < 4; y++)
			{
				for (byte x = 0; x < 4; x++)
				{
					tmpBrick.pix[y][x] = (brickLib[7]).pix[y][x];
				}
			}
			I_rot = 1;
		}
		else
		{
			for (byte y = 0; y < 4; y++)
			{
				for (byte x = 0; x < 4; x++)
				{
					tmpBrick.pix[y][x] = (brickLib[1]).pix[y][x];
				}
			}
			I_rot = 0;
		}
	}
	
	//Check for collision, in case of collision, discard the rotated temporary brick
	if ((!checkSidesCollision(&tmpBrick)) && (!checkFieldCollision(&tmpBrick)))
	{
		//Copy temporary brick pix array to active pix array
		for (byte y = 0; y < 4; y++) 
		{
			for (byte x = 0; x < 4; x++) 
			{
				activeBrick.pix[y][x] = tmpBrick.pix[y][x];
			}
		}
	}
	printField();
}

//Shift brick left/right/down by one if possible
void shiftActiveBrick(char dir) 
{
	char down = 'd';
	char left = 'l';
	char right = 'r';
	
	//Change position of active brick 
	if (dir == left) 
	{
		activeBrick.xpos--;
	} 
	else if (dir == right) 
	{
		activeBrick.xpos++;
}
	else if (dir == down) 
	{
		activeBrick.ypos--;
		
		// If brick has reached the bottom, stop and generate new brick
		if (activeBrick.ypos == 0) 
		{
			if (!checkFieldCollision(&activeBrick))
			{
				addActiveBrickToField();
				activeBrick.enabled = 0;
				printField();
				clearNext();
				newActiveBrick();
			}
		}
	}
	
	//Check for collision
	//In case of collision, go back to previous position
	if ((checkSidesCollision(&activeBrick)) || (checkFieldCollision(&activeBrick)))
	{
		if (dir == left)
		{
			activeBrick.xpos++;
		}
		else if (dir == right)
		{
			activeBrick.xpos--;
		}
		else if (dir == down)
		{	
			activeBrick.ypos += 1;
			addActiveBrickToField();
			activeBrick.enabled = 0;
			printField();
			newActiveBrick();
		}
	}
	printField();
	_delay_us(ws2812_resettime);
}

// CHECK ON TABLE	
void moveFieldDownOne(byte startRow) 
{
	//Top row has nothing on top to move
	if (startRow == (row - 1))  
	{ 
		return;
	}
	byte x, y;
	
	//Copy top row to bottom
	for (y = startRow; y < row - 1; y++)
	{
		for (x = 0; x < column; x++) 
		{
			field.pix[y][x] = field.pix[y + 1][x];
			field.color[y][x] = field.color[y + 1][x];
		}
	}
}

void checkFullLines() 
{
	int x, y;
	int minY = 0;
	for (y = (row - 1); y >= minY; y--) 
	{
		//Add number of leds that are on
		byte rowSum = 0;
		for (x = 0; x < 12; x++) 
		{
			rowSum = rowSum + (field.pix[y][x]);
		}
		if (rowSum >= 12) 
		{
			//Found full row, animate its removal
			for (x = 0; x < 12; x++) 
			{
				field.pix[y][x] = 0;
				printField();
				_delay_us(10 * ws2812_resettime);
			}
			//Move all upper rows down by one
			moveFieldDownOne(y);
			y++; minY++;
			printField();
			_delay_ms(100);

			//Increase brick fall speed
			if (falltime > 40)
			{
				falltime -= 40; // Decrease by 40 * 0.005 = 200ms
			}
		}
	}
}

void clearField()
{
	for (int x = 0; x < column; x++)
	{
		for (int y = 0; y < row; y++)
		{
			field.pix[y][x] = 0;
		}
	}
}

void clearTable()
{
	for(int i = MAXPIX; i > 0; i--)
	{
		led[i-1].r=0;led[i-1].g=0;led[i-1].b=0;
	}
	ws2812_setleds(led, MAXPIX);
	_delay_us(ws2812_resettime);
}

//This menu will show tetris pieces at the bottom of the screen before the game starts
void menu()
{	
	// I on its side
	for (int i = 0; i <= 3; i++)  
	{
		led[pgm_read_byte(&(matrix[0][i]))] = colors[0]; 
	}
	
	// Orange T 
	for (int i = 0; i <= 2; i++) 
	{
		led[pgm_read_byte(&(matrix[1][i]))] = colors[1];
		led[pgm_read_byte(&(matrix[2][1]))] = colors[1]; 
	}
	
	// Upside down L
		led[pgm_read_byte(&(matrix[0][4]))] = colors[2];
		led[pgm_read_byte(&(matrix[1][4]))] = colors[2];
		led[pgm_read_byte(&(matrix[2][4]))] = colors[2];
		
		led[pgm_read_byte(&(matrix[2][3]))] = colors[2];
		
	// Upside down T
	for (int i = 1; i <= 3; i++)
	{
		led[pgm_read_byte(&(matrix[3][i]))] = colors[7];
		led[pgm_read_byte(&(matrix[2][2]))] = colors[7];
	}
		
	// L on its side
	for (int i = 5; i <= 7; i++)  
	{
		led[pgm_read_byte(&(matrix[0][i]))] = colors[4];
		led[pgm_read_byte(&(matrix[1][5]))] = colors[4];	
	}
	
	// Rotated s
		led[pgm_read_byte(&(matrix[3][5]))] = colors[5];
		led[pgm_read_byte(&(matrix[2][5]))] = colors[5];
		led[pgm_read_byte(&(matrix[2][6]))] = colors[5];
		led[pgm_read_byte(&(matrix[1][6]))] = colors[5];
		
	// Flat I
	for (int i = 6; i <= 9; i++)
	{
		led[pgm_read_byte(&(matrix[3][i]))] = colors[3];
	}
	
	// L on its side 
	for (int i = 9; i <= 11; i++)  
	{
		led[pgm_read_byte(&(matrix[0][i]))] = colors[8];
		led[pgm_read_byte(&(matrix[1][11]))] = colors[8];
		
		// blue Q 
		if (i < 11)
		{
		led[pgm_read_byte(&(matrix[1][i]))] = colors[9];
		led[pgm_read_byte(&(matrix[2][i]))] = colors[9];	
		}
	}
	ws2812_setleds(led,MAXPIX);
}

//Piece goes all the way down until it reached the bottom or another piece
void forcedown()
{
	while (!(activeBrick.ypos == 0) || !checkFieldCollision(&activeBrick))
	{
		activeBrick.ypos--;
		
		if (checkFieldCollision(&activeBrick))
		{
			activeBrick.ypos++;			//Go back up one
			addActiveBrickToField();
			activeBrick.enabled = 0;	//Disable brick, it is no longer moving
			clearNext();
		}
		
		if (activeBrick.ypos == 0)
		{
			addActiveBrickToField();
			activeBrick.enabled = 0;
			printField();
			newActiveBrick();
			clearNext();
			break;
		}
		printField();
	}
}

int check_off(cRGB led)
{
	if ((led.r == 0) && (led.g == 0) && (led.b == 0))
	{
		return 1;
	}
	return 0;
}
