#ifndef	TETRIS_H_
#define TETRIS_H_

#include <avr/io.h>
#include <avr/interrupt.h>
#include "ws2812_config.h"
#include "light_ws2812.h"
#include <avr/pgmspace.h>

#define MAXPIX 300
#define COLORLENGTH (MAXPIX/2)
#define FADE (256/COLORLENGTH)
#define row 20
#define row_total 25
#define column 12
#define brightness 0.2
#define row_title 15
#define size_word 5
#define size_piece 4

typedef uint8_t byte;

struct cRGB led[MAXPIX];

int check_off(cRGB);
void clearTable();
void menu();
void sliding_menu();
extern const const int matrix[row_total][column] PROGMEM;
extern struct cRGB colors[10];
extern cRGB led_off;
extern cRGB temp_led;
extern byte tetrisGameOver;
extern byte speed; 
extern struct Brick activeBrick;
extern volatile unsigned char button;
extern volatile int falltime; // 200 * 0.005 = 1s


int checkSidesCollision(struct Brick*);
int checkFieldCollision(struct Brick*);
void temp();
void temp2();
void fallActiveBrick();
void forcedown();
//const char signMessage[] PROGMEM = {"I AM PREDATOR, UNSEEN COMBATANT. CREATED BY THE UNITED STATES DEPART"};

void newActiveBrick(); 
void printField();
void shiftActiveBrick(char);
void rotateActiveBrick();
void moveFieldDownOne(byte);
//int checkSidesCollision(struct Brick);
void addActiveBrickToField();
void checkFullLines();
void showNextPiece();
void clearNext();
void clearField();
byte Pause(byte);

#endif /* TETRIS_H_ */