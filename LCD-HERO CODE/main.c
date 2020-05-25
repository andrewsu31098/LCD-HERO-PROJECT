/*
 * project3.c
 *
 * Created: 2/15/2020 9:21:04 PM
 * Author : Andrew
 */ 

#include <avr/io.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include "avr.h"
#include "lcd.h"

// Note Declarations
// G4 G4-S A4 A4-S B4 C5 C5-S D5 D5-S E5 F5 F5-S G5 G5-S
#define PAUSE 101
#define CL -9
#define CLS -8
#define DL -7
#define DLS -6
#define EL -5
#define FL -4
#define FLS -3
#define GL -2
#define GLS -1
#define A 0
#define AS 1
#define B 2
#define C 3
#define CS 4
#define D 5
#define DS 6
#define E 7
#define F 8
#define FS 9
#define G 10
#define GS 11
#define AH 12
// Duration Declarations in msec
#define WHOLE 1
#define HALF 2
#define QUARTER 4
#define EIGHTH 8

// MUSIC STRUCTURES
typedef struct music_note{
	int freq;
	int duration;
} note;

typedef struct song_sheet{
	int songCounter;
	int insertCounter;
	int songLen;
	note SONGNOTES[50];
} song;
song FURELISE = {-1,0,27,{{E,EIGHTH},{DS,EIGHTH},{E,EIGHTH},{DS,EIGHTH},{E,EIGHTH},{B,EIGHTH},{D,EIGHTH},{C,EIGHTH},{A,QUARTER}
						,{CL,EIGHTH},{EL,EIGHTH},{A,EIGHTH},{B,QUARTER}
						,{EL,EIGHTH},{GLS,EIGHTH},{B,EIGHTH},{C,QUARTER}
						,{EL,EIGHTH},{E,EIGHTH},{DS,EIGHTH},{E,EIGHTH},{DS,EIGHTH},{E,EIGHTH},{B,EIGHTH},{D,EIGHTH},{C,EIGHTH},{A,QUARTER}
	}};

// GMODEL STRUCTURE
typedef struct noteGameModel {
	int index;
	int score;
	// Boolean variables
	int playTrue;
	// if (firstNote) { play waitingCount iters of curNote }
	int waitCounter;
	char noteArray[8];
} gameModel;

// KEYPAD
int isPressed(int row, int col){
	DDRC = 0x00;
	PORTC = 0x00;
	SET_BIT(PORTC,row);
	SET_BIT(DDRC,col+4);
	avr_wait(1);
	if (!GET_BIT(PINC,row)){
		return 1;
	}
	return 0;
}
int getKey(){
	int r,c;
	for (r = 0; r<4; r++){
		for (c = 0; c<4; c++){
			if(isPressed(r,c)){
				return (r*4)+c+1;
			}
		}
	}
	return 0;
}

// Song Playing
void SPK_ON(int microsec){
	SET_BIT(PORTB,3);
	avr_wait(microsec);
}
void SPK_OFF(int microsec){
	CLR_BIT(PORTB,3);
	avr_wait(microsec);
}
// One iteration = two sixteenth Notes = 1 eighth note
void playEighthNote(song * iSong){
	
	note iNote = iSong->SONGNOTES[iSong->songCounter];
	if (iNote.freq != PAUSE){

		// PITCH
		float frequency = pow(2,iNote.freq/12.0)*440;
		// TEMPO
		// duration = #cycles to play. i.e) 440 cycles / 8 = 55 cycles = 1/8 second = sixteenth note duration (whole note = 2 seconds)
		int duration = 4*frequency/8;
		//duration = 2*frequency/iNote.duration;
		
		// DUTY CYCLE - VOLUME
		// How long it takes for a period to occur in microseconds
		float period = 0.5/frequency;
		period *= 1000000;
		
		int timeOn, timeOff;
		timeOn = period*0.5; timeOff = period*0.5;
		
		for (int i = 0; i<duration; i++){
			SPK_ON(timeOn);
			SPK_OFF(timeOff);
		}
	}
	else {
		int duration = 25000;
		waitLongerByTen(duration);
	}
}


int songEnded(song * iSong){
	if (iSong->songCounter>=iSong->songLen-1)
		return 1;
	return 0;
}
int noMoreNotes(gameModel * iModel, song * iSong){
	return (iSong->insertCounter >= iSong->songLen);
}
// LCD DISPLAYING
void startScreen(){
	lcd_clr();
	lcd_puts2("LCD-Hero!");
	lcd_pos(1,0);
	lcd_puts2("Press 1 to start");
}
void updateLCD(gameModel * iModel){
	lcd_clr();
	lcd_put(iModel->noteArray[iModel->index]); 
	lcd_put('|');
	for(int i = 1; i<8; i++){
		int index = (iModel->index + i)%8;
		lcd_put(iModel->noteArray[index]);
	}
	lcd_pos(1,0);
	char scoreLine[30];
	sprintf(scoreLine,"Score: %d",iModel->score);
	lcd_puts2(scoreLine); 
}	
void checkLCD(gameModel * iModel, int rightAnswer){
	updateLCD(iModel);
	if (rightAnswer){
		lcd_pos(0,0);
		lcd_put('O');	
	}
	else{
		lcd_pos(0,0);
		lcd_put('X');
	} 
}
// ADC Noise Reading for Randomness
unsigned int get_noise(void){
	// Sets up info, i.e (AVCC, Voltage range, ADLAR, Single ended input no gain)
	ADMUX = 64;
	// Enables ADC, starts conversion
	SET_BIT(ADCSRA,7); SET_BIT(ADCSRA,6);
	while (!GET_BIT(ADCSRA,4)){}
	// Get result, top 2 bits ADCH last 8 bits ADCL
	unsigned int result = ADCL;
	unsigned int throwaway = ADCH;
	
	return result;
}
// Game Model Functions
unsigned int xorshift( )
{
	unsigned int xs = get_noise();
	xs ^= xs << 7;
	xs ^= xs >> 9;
	xs ^= xs << 8;
	return xs;
}
int noteExpected(gameModel * iModel){
	if (iModel->noteArray[iModel->index] == '-')
		return 0;
	return 1;
}
int checkAnswer(char note, int key){
	int correct = 0;
	switch (note){
		case 'A':
			if (key==4) correct = 1;
			break;
		case 'B':
			if (key==8) correct = 1;
			break;
		case 'C':
			if (key==12) correct = 1;
			break;
		case 'D':
			if (key==16) correct = 1;
			break;
		default:
			correct = 0;
			break;
	}
	return correct;
}
void updateScore(gameModel * iModel, int right){
	if (right) (iModel->score)++;
}
void addWaitCounter(gameModel * iModel, song * iSong){
	note myNote = iSong->SONGNOTES[iSong->insertCounter];
	(iSong->insertCounter)++;
	switch (myNote.duration){
		case WHOLE:
			iModel->waitCounter = 8;
			break;
		case HALF:
			iModel->waitCounter = 4;
			break;
		case QUARTER:
			iModel->waitCounter = 2;
			break;
		case EIGHTH:
			iModel->waitCounter = 1;
			break;
		default:
			iModel->waitCounter = 1;
			break;
	}
}
void insertRandomNote(gameModel * iModel){
	int newIndex = (iModel->index + 7)%8;
	unsigned int random = xorshift();
	random = random % 4;
	switch (random){
		case 0:
			iModel->noteArray[newIndex] = 'A';
			break;
		case 1:
			iModel->noteArray[newIndex] = 'B';
			break;
		case 2:
			iModel->noteArray[newIndex] = 'C';
			break;
		case 3:
			iModel->noteArray[newIndex] = 'D';
			break;
		default:
			iModel->noteArray[newIndex] = 'Z';
			break;		
	}
	
}
void insertBlank(gameModel * iModel){
	int newIndex = (iModel->index + 7)%8;
	iModel->noteArray[newIndex] = '-';
}
void updatePlayer(gameModel * iModel, song * iSong){
	if (noteExpected(iModel))
		(iSong->songCounter)++;
}
void updateInserter(gameModel * iModel, song * iSong){
	//"Shift" array to left;
	iModel->index = (iModel->index + 1)%8;
	// Decrement wait per iteration
	if (iModel->waitCounter > 0){
		(iModel->waitCounter)--;
	}
	// Insert blanks if end of song reached
	if (noMoreNotes(iModel, iSong)){
		insertBlank(iModel);
	}
	// Insert blanks to represent pauses after notes
	else if (iModel->waitCounter > 0){
		insertBlank(iModel);
	}
	// Insert letter to represent a new note
	else if (iModel->waitCounter <= 0){
		insertRandomNote(iModel);
		addWaitCounter(iModel, iSong);
	}
}

void musicAction(gameModel * iModel, song * iSong){
	if (iModel->playTrue){
		playEighthNote(iSong);
	}
	else{
		waitLongerByTen(20000);
	}
}
void updateGame(gameModel * iModel, song * iSong, int * lastNoteDuration){
	updateInserter(iModel, iSong);
	updatePlayer(iModel, iSong);
	updateLCD(iModel);
	
	int key = getKey();
	if (noteExpected(iModel)){
		int rightAnswer = checkAnswer(iModel->noteArray[iModel->index],key);
		updateScore(iModel, rightAnswer);
		checkLCD(iModel, rightAnswer);
		iModel->playTrue = rightAnswer;
	}
	if (songEnded(iSong)){
		if (*lastNoteDuration > 0)
			(*lastNoteDuration)--;
		else 
			iModel->playTrue = 0;
	}
	musicAction(iModel, iSong);
	
}

int main(void)
{
	SET_BIT(DDRB,3);
    /* Replace with your application code */
	int runGame = 0; 
	int lastNoteDuration = 8/(FURELISE.SONGNOTES[FURELISE.songLen-1].duration);
	gameModel myModel = {0, 0, 0, 0,{'-','-','-','-','-','-','-','-'}};
	
	lcd_init();
	startScreen();
    while (1) 
    {
		int key = getKey();
		if (key==1){
			runGame = 1;
		}
		if (runGame){
			updateGame(&myModel,&FURELISE, &lastNoteDuration);	
		}
    }
}

