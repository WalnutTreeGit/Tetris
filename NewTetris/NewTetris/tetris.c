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

byte random6 = 0;
byte random9 = 0;

const int PROGMEM randombrick[28] = {4, 0, 3, 5, 1, 6, 3, 1, 4, 3, 2, 0, 6, 5, 0, 5, 2, 6, 1, 3, 4, 0, 6, 1, 3, 5, 2, 4}; // random numbers from 0-6
const int PROGMEM randomcolor[40] = {4, 0, 7, 3, 6, 2, 5, 9, 8, 1, 8, 9, 2, 4, 0, 1, 6, 7, 5, 3, 8, 1, 3, 2, 5, 7, 4, 0, 9, 6, 0, 5, 9, 8, 2, 1, 7, 6, 3, 4}; // random numbers from 0-9
const int PROGMEM test[7]  = {0,1,2,3,4,5,6};
const int brickSpeed[] = {1000,800,600,400,200};
	
volatile int falltime = 200; // 200 * 0.005 = 1s

 byte t_rot = 0;
 byte I_rot = 0;
	
//int speed = 1000;
	
byte tetrisGameOver = 0;
byte selectedBrick = 0;
byte selectedColor = 0;
byte printcolor;

byte nbRowsThisLevel;
uint16_t nbRowsTotal;

byte pause = 1;


typedef struct Field {
	byte pix[row][column + 1]; //Field matrix, Make field one larger so that collision detection with bottom of field can be done in a uniform way
	byte color[row][column]; // Color for each spot
} Field;
Field field;

typedef struct Brick {
  byte enabled;				//Brick is disabled when it has landed
  uint8_t xpos, ypos;
  byte yOffset;				//Y-offset to use when placing brick at top of field
  byte siz;
  byte pix[4][4];			//Matrix for each Brick's design

  byte color;
} Brick;
Brick activeBrick;

Brick tmpBrick;
//Brick tempActiveBrick;


typedef struct AbstractBrick {
	byte yOffset;				//Y-offset to use when placing brick at top of field
	byte siz;					// Size will be used for reading Brick's pix and rotating
	int pix[4][4];				//For each Brick's design
}AbstractBrick; 

/*
AbstractBrick pieceI[2] = {
	{
		1,
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
		4,
		{
			{0, 0, 0, 0},
			{0, 0, 0, 0},
			{0, 0, 0, 0},
			{1, 1, 1, 1}
		}
	}
};
*/

AbstractBrick brickLib[9] = {
	{
		1,//yoffset when adding brick to field
		2, // size
		{ 
			{1, 1, 0, 0}, // brick design
			{1, 1, 0, 0},
			{0, 0, 0, 0},
			{0, 0, 0, 0}
		}
	},
	{
		0,
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
			{1, 1, 1, 0}, // 1,0,0,0
			{0, 1, 0, 0}, // 1,1,1,0
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
	{
		1,
		3,
		{
			{0, 1, 0, 0},
			{1, 1, 1, 0},
			{0, 0, 0, 0},
			{0, 0, 0, 0}
		}
	},
};


//edit to show bars
void Pause()
{
	clearTable();
	while (!pause);//Paused until pressed again
	{
		if (button == 'l')
		{
			pause = 1;
		}
	}
	pause = 0;
	//printpause
	printField();
}

void temp()
{
	/*
	int y = 3;
	for (int x = 0; x < column; x++)
	{
		field.pix[y][x] = 1;
		field.color[y][x] = 1;
	}
	*/
	activeBrick.xpos--;
	printField();
}

void temp2()
{
	int y = 4;
	for (int x = 0; x < column; x++)
	{
		field.pix[y][x] = 1;
		field.color[y][x] = 1;
	}
	printField();
}

void addActiveBrickToField() // Fixar bloco e suas propriedades ao ponto específico do field.
{
	byte fx, fy; // used to check if brick is within bounds, no trespassing outside
	for (byte by = 0; by < 4; by++)
	{
		for (byte bx = 0; bx < 4; bx++)
		{
			fx = activeBrick.xpos + bx;
			fy = activeBrick.ypos + by;

			if (fx >= 0 && fy >= 0 && fx < column && fy < row && activeBrick.pix[by][bx]) { //Check if inside playing field
				//field.pix[fy][fx] = field.pix[fy][fx] || activeBrick.pix[by][bx];
				field.pix[fy][fx] = activeBrick.pix[by][bx];
				//field.color[fy][fx] = selectedColor;
				field.color[fy][fx] = activeBrick.color;
				activeBrick.pix[by][bx] = 0;
			}
		}
	}
	I_rot = 0;
	_delay_us(ws2812_resettime);
}

int checkSidesCollision(struct Brick* brick) 
{	
	//Check vertical collision with sides of field
	byte fx; //,fy;
	for (byte by = 0; by < 4; by++) 
	{
		for (byte bx = 0; bx < 4; bx++) 
		{
			if ((*brick).pix[by][bx]) 
			{
				fx = (*brick).xpos + bx;//Determine the brick.pix position on the field
				//fy = (*brick).ypos + by;
				if (fx < 0 || fx >= column) 
				{
					return 1;
				}
			}
		}
	}
	return 0;
}

int checkBottom()
{
	if (activeBrick.ypos == 255)
	{
		activeBrick.ypos = 0;
		addActiveBrickToField();
		activeBrick.enabled = 0;
		return 1;
	}
	return 0;
}

int checkFieldCollision(struct Brick* brick)
{
	/*
	byte bx, by; // variable++
	byte fx, fy; // next position of block
	for (by = 0; by < 4; by++) 
	{
		for (bx = 0; bx < 4; bx++) 
		{
			fx = (brick)->xpos + bx;
			fy = (brick)->ypos - by;
			
			if (brick->ypos  == 255) // shiftbrick will do .ypos-- making it 255 once it reaches 0
			{
				led[4].g = 11;
				ws2812_setleds(led, MAXPIX);
				_delay_us(ws2812_resettime);
				
				return 1;
			}
			
			
			else if ( (((*brick).pix[bx][by] == 1) && (field.pix[fx][fy] == 1))) // 255 because of the overflow caused by shiftbrick()
			{
				
				led[1].b = 11;
				ws2812_setleds(led, MAXPIX);
				_delay_us(ws2812_resettime);
				
				return 1;
			}
		}
	}
	return 0;
	*/
	/*
	  byte bx, by;
	  byte fx, fy;
	  for (by = 0; by < 4; by++) 
	  {
		  for (bx = 0; bx < 4; bx++) 
		  {
			  fx = (*brick).xpos + bx;
			  fy = (*brick).ypos + by;
			  
			  
			  if ((*brick).ypos == 255)
			  {
				  activeBrick.ypos = 0;
				  activeBrick.enabled = 0;
				  return 1;
			  }
			  
			  
			  
			  
			  if (( (*brick).pix[bx][by] == 1)
			  && ( field.pix[fx][fy] == 1)) 
			  {
				  
				  led[3].r = 11;
				  ws2812_setleds(led, MAXPIX);
				  _delay_us(ws2812_resettime);
				  return 1;
			  }
		  }
	  }
	  */
	   byte bx, by;
	   byte fx, fy;
	   for (by = 0; by < 4; by++) 
	   {
		   for (bx = 0; bx < 4; bx++) 
		   {
			   fx = (*brick).xpos + bx;
			   fy = (*brick).ypos + by;
			   
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
	selectedBrick = pgm_read_byte(&randombrick[random6]); //rand() % 7; // 0-6, 7 pieces
	selectedColor = pgm_read_byte(&randomcolor[random9]); //rand() % 10; // 0-9, 10 colors
	
	activeBrick.siz = brickLib[selectedBrick].siz;
	activeBrick.yOffset = brickLib[selectedBrick].yOffset;

	activeBrick.color = selectedColor; // matrix = color;
	
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
	
	/*
	Brick nextBrick;
	int next = pgm_read_byte(&randombrick[random6]);
	
	int nextColor = pgm_read_byte(&randomcolor[random9]);
	
	nextBrick.siz = brickLib[next].siz;
	nextBrick.yOffset = brickLib[next].yOffset;
	
	nextBrick.enabled = 1;
	
	nextBrick.color = nextColor;
	
	nextBrick.xpos = 0;
	nextBrick.ypos = 0;
	
	Brick tempbrick;
	tempbrick = activeBrick;
	
	activeBrick = nextBrick;
	*/
}

void newActiveBrick() 
{
	//Reads value from flash memory for brick's shape and color
	selectedBrick = pgm_read_byte(&randombrick[random6]); // 0-6, 7 pieces
	selectedColor = pgm_read_byte(&randomcolor[random9]); // 0-9, 10 colors
	//printcolor = selectedColor;
	
	random6++;
	random9++;
	
	if (random6 > 6) random6 = 0;
	if (random9 > 9) random9 = 0; 
	
	activeBrick.siz = brickLib[selectedBrick].siz;
	activeBrick.yOffset = brickLib[selectedBrick].yOffset;
	
	//activeBrick.xpos = column / 2 - activeBrick.siz / 2; // //To center
	activeBrick.xpos = 6;
	
	//activeBrick.ypos = 21 - 1 - activeBrick.yOffset;			//Top of the screen
	activeBrick.ypos = 10;
	
	activeBrick.enabled = 1;

	activeBrick.color = selectedColor; // matrix = color;
	//activeBrick.color = 0;
	//1 + selectedColor;
	 
	//Copy pix array of selected Brick
	for (byte y = 0; y < 4; y++) 
	{
		for (byte x = 0; x < 4; x++) 
		{
			activeBrick.pix[y][x] = (brickLib[selectedBrick]).pix[y][x];
		}
	}
	
	Brick realBrick;
	realBrick = activeBrick;
	//showNextPiece();
	activeBrick = realBrick;
	
	printField();
	//clearNext();
	
	if (checkFieldCollision(&activeBrick)) 
	{
		tetrisGameOver = 1;
	}	
}

void printField() 
{
	byte activeBrickSpecificPix = 0;
	//byte tempActiveBrickSpecificPix = 0;
	
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
																		 //- activeBrick.yOffset
				//tempActiveBrickSpecificPix = tempActiveBrick.pix[y - tempActiveBrick.ypos][x - tempActiveBrick.xpos]; // 1 or 0
				/*
				if (tempActiveBrickSpecificPix)
				{
					led[pgm_read_byte(&matrix[y][x])] = led_off;
				}*/
			}
			
			if (field.pix[y][x]) 
			{
				led[pgm_read_byte(&matrix[y][x])] = colors[field.color[y][x]];	// if led is on, color remains the same so it does not get turned off
			}
			else if (activeBrickSpecificPix) // Print Active Brick
			{
				led[pgm_read_byte(&matrix[y][x])] = colors[activeBrick.color];
			}		
			else 
			{
				led[pgm_read_byte(&matrix[y][x])] = led_off; // remaining unused blocks are off	
			}
			activeBrickSpecificPix = 0;
		}
	}
	ws2812_setleds(led,MAXPIX);
	_delay_us(ws2812_resettime);
}

// CHECK ON TABLE
void rotateActiveBrick() {
	//Copy active brick pix array to temporary pix array
	
	for (byte y = 0; y < 4; y++) 
	{
		for (byte x = 0; x < 4; x++) 
		{
			tmpBrick.pix[y][x] = activeBrick.pix[y][x]; // IMPORTANTE, verificar x,y e y,x na mesa
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
		
		
		t_rot++;
		
		if (t_rot == 2)
		{
			for (byte y = 0; y < 4; y++)
			{
				for (byte x = 0; x < 4; x++)
				{
					tmpBrick.pix[y][x] = (brickLib[8]).pix[y][x];
				}
			}
			//tmpBrick.ypos--;
			//tmpBrick.ypos = tmpBrick.ypos - 1;	
			//activeBrick.ypos = activeBrick.ypos - 1;
			//activeBrick.siz = 2;
			//printField();
			//activeBrick.siz = 3;
		}
		
		
		if (t_rot > 3)
		{
			t_rot = 0;
		}
	} 
	
	else if (selectedBrick == 0)
	{
		return;
	}
	
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
		
		
		
	/*	
	else if (activeBrick.siz == 4) 
	{
		if (I_rot == 0)				//em pé
		{
			n = 3;					// rodar 3 vezes
			I_rot = 1;				// deitado
		}
		else						// deitado
		{
			n = 1;					// rodar uma vez
			I_rot = 0;				// em pé
		}
		
		for (int i = 0; i < n; i++)
		{
			//Perform rotation around center "cross"
			tmpBrick.pix[0][0] = activeBrick.pix[0][3];
			tmpBrick.pix[0][1] = activeBrick.pix[1][3];
			tmpBrick.pix[0][2] = activeBrick.pix[2][3];
			tmpBrick.pix[0][3] = activeBrick.pix[3][3];
			tmpBrick.pix[1][0] = activeBrick.pix[0][2];
			tmpBrick.pix[1][1] = activeBrick.pix[1][2];
			tmpBrick.pix[1][2] = activeBrick.pix[2][2];
			tmpBrick.pix[1][3] = activeBrick.pix[3][2];
			tmpBrick.pix[2][0] = activeBrick.pix[0][1];
			tmpBrick.pix[2][1] = activeBrick.pix[1][1];
			tmpBrick.pix[2][2] = activeBrick.pix[2][1];
			tmpBrick.pix[2][3] = activeBrick.pix[3][1];
			tmpBrick.pix[3][0] = activeBrick.pix[0][0];
			tmpBrick.pix[3][1] = activeBrick.pix[1][0];
			tmpBrick.pix[3][2] = activeBrick.pix[2][0];
			tmpBrick.pix[3][3] = activeBrick.pix[3][0];
			
			for (byte y = 0; y < 4; y++)
			{
				for (byte x = 0; x < 4; x++)
				{
					activeBrick.pix[y][x] = tmpBrick.pix[y][x];
					//activeBrick.ypos = tmpBrick.ypos;
				}
			}
		}
	}
	*/
	
	//Now validate by checking collision.
	//Collision possibilities:
	//      -Brick now sticks outside field
	//      -Brick now sticks inside fixed bricks of field
	//In case of collision, we just discard the rotated temporary brick
	
	if ((!checkSidesCollision(&tmpBrick)) && (!checkFieldCollision(&tmpBrick)))
	{
		//Copy temporary brick pix array to active pix array
		
		for (byte y = 0; y < 4; y++) 
		{
			for (byte x = 0; x < 4; x++) 
			{
				activeBrick.pix[y][x] = tmpBrick.pix[y][x];
				//activeBrick.ypos = tmpBrick.ypos;
			}
		}
		
		/*
		if (activeBrick.siz == 4)
		{
			activeBrick.ypos = tmpBrick.ypos + 2;
		}
		*/
	}
	
	//activeBrick.ypos = tmpBrick.ypos;
	printField(); // ?
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
		
		if (activeBrick.ypos == 0) // If brick has reached the bottom, stop and generate new brick
		{
			if (!checkFieldCollision(&activeBrick))
			{
				addActiveBrickToField();
				activeBrick.enabled = 0;
				printField();
				
				//clearNext();
				newActiveBrick();
				//clearNext();
			}
		}
	}
	/*
	if (activeBrick.ypos == 0)
	{
		addActiveBrickToField();
		activeBrick.enabled = 0;//Disable brick, it is no longer moving
		printField();
		
		led[2].b = 33;
		ws2812_setleds(led, MAXPIX);
		_delay_us(ws2812_resettime);
		_delay_ms(2000);
	}
	*/
	
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
	if (startRow == (row - 1))  //Top row has nothing on top to move...
	{ 
		return;
	}
	
	byte x, y;
	//Copy top row to bottom
	for (y = startRow; y < row - 1; y++) // y = startRow - 1;
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

			nbRowsThisLevel++; nbRowsTotal++;
				//Increase brick fall speed
				if (falltime > 40)
				{
					falltime -= 20;
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
	for (int i = 0; i <= 3; i++)  
	{
		led[pgm_read_byte(&(matrix[0][i]))] = colors[1]; // I on its side
	}
	
	for (int i = 0; i <= 2; i++) 
	{
		led[pgm_read_byte(&(matrix[1][i]))] = colors[2];
		led[pgm_read_byte(&(matrix[2][1]))] = colors[2]; // orange T 
	}
	/*
	for (int i = 3; i <= 4; i++)
	{
		led[pgm_read_byte(&(matrix[1][i]))] = colors[6];
		led[pgm_read_byte(&(matrix[1][i]))] = colors[6];
	}
	*/
	for (int i = 4; i <= 5; i++)  
	{
		led[pgm_read_byte(&(matrix[0][i]))] = colors[3];
		led[pgm_read_byte(&(matrix[2][i - 1]))] = colors[3]; // Z next to I
	}
	
	for (int i = 5; i <= 7; i++)  
	{
		led[pgm_read_byte(&(matrix[0][i]))] = colors[4];
		led[pgm_read_byte(&(matrix[1][5]))] = colors[4];	// L on its side
		//led[pgm_read_byte(&(matrix[2][5]))] = colors[4];// L em pé
	}
	
	for (int i = 9; i <= 11; i++)  
	{
		led[pgm_read_byte(&(matrix[0][i]))] = colors[4];
		led[pgm_read_byte(&(matrix[1][11]))] = colors[4];	// green L on its side 
		
		if (i < 11)
		{
		led[pgm_read_byte(&(matrix[1][i]))] = colors[5];
		led[pgm_read_byte(&(matrix[2][i]))] = colors[5];	// blue Q 
		}
	}
	
	//led[0] = colors[3];
	ws2812_setleds(led,MAXPIX);
}

//Make Tetris Pieces Fall, speed depends on number of rows cleared
// try to add to read from flash
/*
void fallActiveBrick()
{
	switch (speed)
	{
		case 0:
		_delay_ms(1000);
		shiftActiveBrick('d');
		break;

		case 1:
		_delay_ms(800);
		shiftActiveBrick('d');
		break;
		
		case 2:
		_delay_ms(600);
		shiftActiveBrick('d');
		break;
		
		case 3:
		_delay_ms(400);
		shiftActiveBrick('d');
		break;
		
		case 4:
		_delay_ms(200);
		shiftActiveBrick('d');
		break;
		
		default:
		_delay_ms(200);
		shiftActiveBrick('d');
	}
}
*/

//Piece goes all the way down until it reached the bottom or another piece
void forcedown()
{
	while (!(activeBrick.ypos == 0) || !checkFieldCollision(&activeBrick))
	{
		activeBrick.ypos--;
		
		if (checkFieldCollision(&activeBrick))
		{
			activeBrick.ypos++;		//Go back up one
			addActiveBrickToField();
			activeBrick.enabled = 0;//Disable brick, it is no longer moving
			//clearNext();
		}
		
		
		if (activeBrick.ypos == 0)
		{
			addActiveBrickToField();
			activeBrick.enabled = 0;
			printField();
			newActiveBrick();
			//clearNext();
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
	}
*/
void sliding_menu()
{
	byte i = 0;
	while (1)
	{
		// T  
		
		int offset_impar = 1;
		int offset_par = (column - 1) - offset_impar;
		
		for (byte T = 0; T < 5; T++)
		{
			led[pgm_read_byte(&(matrix[row_title][offset_impar + i + T]))] = colors[7];		// IMPAR
		}
		led[pgm_read_byte(&(matrix[row_title][offset_impar + i - 1]))] = led_off;
	
		offset_impar += 2; // 3
		offset_par = (column - 1) - offset_impar;
		
		led[pgm_read_byte(&(matrix[row_title - 1][offset_par - i]))] = colors[5];			// PAR
		led[pgm_read_byte(&(matrix[row_title - 1][offset_par - i + 1]))] = led_off;
		
		led[pgm_read_byte(&(matrix[row_title - 2][offset_impar + i]))] = colors[7];			// IMPAR
		led[pgm_read_byte(&(matrix[row_title - 2][offset_impar + i - 1]))] = led_off;
		
		led[pgm_read_byte(&(matrix[row_title - 3][offset_par - i]))] = colors[5];			// PAR
		led[pgm_read_byte(&(matrix[row_title - 3][offset_par - i + 1]))] = led_off;
		
		led[pgm_read_byte(&(matrix[row_title - 4][offset_impar + i]))] = colors[7];			// IMPAR
		led[pgm_read_byte(&(matrix[row_title - 4][offset_impar + i - 1]))] = led_off;
		
		// E
		
		offset_impar = -3; 
		offset_par = (column - 1) + 1; // 12
		
		for (int e = 0; e < 3; e++)
		{
			led[pgm_read_byte(&(matrix[row_title][offset_impar + i + e]))] = colors[2];		// IMPAR
		
			led[pgm_read_byte(&(matrix[row_title - 2][offset_impar + i + e]))] = colors[2];	// IMPAR
			
			led[pgm_read_byte(&(matrix[row_title - 4][offset_impar + i + e]))] = colors[2];	// IMPAR
		}
		led[pgm_read_byte(&(matrix[row_title][offset_impar + i - 1]))] = led_off;
		led[pgm_read_byte(&(matrix[row_title - 2][offset_impar + i - 1]))] = led_off;
		led[pgm_read_byte(&(matrix[row_title - 4][offset_impar + i - 1]))] = led_off;
		
		led[pgm_read_byte(&(matrix[row_title - 1][offset_par - i]))] = colors[4];			// PAR
		led[pgm_read_byte(&(matrix[row_title - 1][offset_par - i - 1 ]))] = led_off;
		
		led[pgm_read_byte(&(matrix[row_title - 3][offset_par - i]))] = colors[4];			// PAR
		led[pgm_read_byte(&(matrix[row_title - 3][offset_par - i - 1]))] = led_off;
		
		// T
		
		offset_impar = -9;
		offset_par = (column - 1) - offset_impar;
		
		for (byte T = 0; T < 5; T++)
		{
			led[pgm_read_byte(&(matrix[row_title][offset_impar + i + T]))] = colors[7];		// IMPAR
		}
		led[pgm_read_byte(&(matrix[row_title][offset_impar + i - 1]))] = led_off;
		
		offset_impar += 2; // -7
		offset_par = (column - 1) - offset_impar;
		
		led[pgm_read_byte(&(matrix[row_title - 1][offset_par - i]))] = colors[5];			// PAR
		led[pgm_read_byte(&(matrix[row_title - 1][offset_par - i + 1]))] = led_off;
		
		led[pgm_read_byte(&(matrix[row_title - 2][offset_impar + i]))] = colors[7];			// IMPAR
		led[pgm_read_byte(&(matrix[row_title - 2][offset_impar + i - 1]))] = led_off;
		
		led[pgm_read_byte(&(matrix[row_title - 3][offset_par - i]))] = colors[5];			// PAR
		led[pgm_read_byte(&(matrix[row_title - 3][offset_par - i + 1]))] = led_off;
		
		led[pgm_read_byte(&(matrix[row_title - 4][offset_impar + i]))] = colors[7];			// IMPAR
		led[pgm_read_byte(&(matrix[row_title - 4][offset_impar + i - 1]))] = led_off;
		
		// R
		
		offset_impar = -13; //último na parte de cima de R
		offset_par = (column - 1) - offset_impar ; //24
		
		for (byte r = 0; r < 3; r++)
		{
			led[pgm_read_byte(&(matrix[row_title][offset_impar + i + r]))] = colors[3];		// IMPAR	
		
			if (r < 2)
			{
				led[pgm_read_byte(&(matrix[row_title - 2][offset_impar + i + r]))] = colors[3];// IMPAR	
			}
		}
		led[pgm_read_byte(&(matrix[row_title][offset_impar + i - 1]))] = led_off;
		led[pgm_read_byte(&(matrix[row_title - 2][offset_impar + i - 1]))] = led_off;
		
		led[pgm_read_byte(&(matrix[row_title - 1][offset_par - i]))] = colors[6];			// PAR
		led[pgm_read_byte(&(matrix[row_title - 1][offset_par - i + 1]))] = led_off;
		
		led[pgm_read_byte(&(matrix[row_title - 1][offset_par - i - 2]))] = colors[6];		// PAR
		led[pgm_read_byte(&(matrix[row_title - 1][offset_par - i - 2 + 1]))] = led_off;
		
		led[pgm_read_byte(&(matrix[row_title - 4][offset_impar + i - 1]))] = colors[3];		// IMPAR
		led[pgm_read_byte(&(matrix[row_title - 4][offset_impar + i - 1 - 1]))] = led_off;
		
		led[pgm_read_byte(&(matrix[row_title - 4][offset_impar + i + 2]))] = colors[3];		// IMPAR
		led[pgm_read_byte(&(matrix[row_title - 4][offset_impar + i + 2 - 1]))] = led_off;
		
		// I
		
		offset_impar = -16;
		offset_par = (column - 1) - offset_impar ; //27
		
		for ( byte I = 0; I < 5; I++)
		{
			if (I%2)
			{
				led[pgm_read_byte(&(matrix[row_title - I][offset_par - i]))] = colors[0];	// PAR
				led[pgm_read_byte(&(matrix[row_title - I][offset_par + i + 1]))] = led_off;
			}
			else 
			{
				led[pgm_read_byte(&(matrix[row_title - I][offset_impar + i]))] = colors[0];	// IMPAR
				led[pgm_read_byte(&(matrix[row_title - I][offset_impar + i - 1]))] = led_off;
			}	
		}
		
		// S
		
		offset_impar = -20;
		offset_par = (column - 1) - offset_impar ; //31
		
		for (int s = 0; s < 3; s++)
		{
			led[pgm_read_byte(&(matrix[row_title][offset_impar + i + s]))] = colors[8];		// IMPAR
			
			led[pgm_read_byte(&(matrix[row_title - 2][offset_impar + i + s]))] = colors[8];	// IMPAR
			
			led[pgm_read_byte(&matrix[row_title - 4][offset_impar + i + s])] = colors[8];	// IMPAR
		}
		led[pgm_read_byte(&matrix[row_title][offset_impar + i - 1])] = led_off;
		led[pgm_read_byte(&matrix[row_title - 2][offset_impar + i - 1])] = led_off;
		led[pgm_read_byte(&matrix[row_title - 4][offset_impar + i - 1])] = led_off;
		
		led[pgm_read_byte(&matrix[row_title - 1][offset_par - i - 2])] = colors[4];		// PAR
		led[pgm_read_byte(&matrix[row_title - 1][offset_par - i - 2 - 1])] = led_off;
		
		led[pgm_read_byte(&matrix[row_title - 3][offset_par - i])] = colors[4];			// PAR
		led[pgm_read_byte(&matrix[row_title - 3][offset_par - i - 1])] = led_off;
		
		i++;
		
		if ( i ==  31 ) // collunm  - 1 + 31
		{
			i = 0;
		}
		ws2812_setleds(led,MAXPIX);
		_delay_ms(500);
	}
}
