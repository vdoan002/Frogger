/*
 * GccApplication1.cpp
 *
 * Created: 2/25/2016 11:51:20 AM
 *  Author: student
 */ 


#include <avr/io.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include "io.c"
#include "timer.h"

unsigned char sequence[8] = {1, 2, 4, 6, 3, 3, 5, 7};
unsigned char sequence_length = 8;
unsigned char note_length[8] = {1,0,2,1,1,3,1,4};

//SHARED VARIABLES//
unsigned char fail = 0;
unsigned char score = '0';
unsigned char start = 0;
unsigned char button = 0;
unsigned char button_press = 0;
unsigned char lives = 0;
unsigned char coin = 0;
unsigned char scoretext[6] = {'S', 'c', 'o', 'r', 'e', ':'};
const unsigned char frog_start_row = 0x01;
const unsigned char frog_start_col = 0x10;
typedef struct _task {
	//Task's current state, period, and the time elapsed
	// since the last tick
	signed char state;
	unsigned long int period;
	unsigned long int elapsedTime;
	//Task tick function
	int (*TickFct)(int);
} task;


unsigned char SetBit(unsigned char x, unsigned char k, unsigned char b){
	return (b ? x | (0x01 << k) : x & ~(0x01 << k));
}
unsigned char GetBit(unsigned char x, unsigned char k){
	return ((x & (0x01 << k)) != 0);
}


unsigned long int findGCD (unsigned long int a, unsigned long int b)
{
	unsigned long int c;
	while(1){
		c = a%b;
		if(c==0){return b;}
		a = b;
		b = c;
	}
	return 0;
}

void transmit_data(unsigned char data, unsigned char data2) {
	int i;
	for (i = 0; i < 8 ; ++i) {
		// Sets SRCLR to 1 allowing data to be set
		// Also clears SRCLK in preparation of sending data
		PORTB = 0x08;
		// set SER = next bit of data to be sent.
		PORTB |= ((data >> i) & 0x01);
		// set SRCLK = 1. Rising edge shifts next bit of data into the shift register
		PORTB |= 0x02;
	}
	for (i = 0; i < 8 ; ++i) {
		// Sets SRCLR to 1 allowing data to be set
		// Also clears SRCLK in preparation of sending data
		PORTB = 0x18;
		// set SER = next bit of data to be sent.
		PORTB |= ((data2 >> i) & 0x01);
		// set SRCLK = 1. Rising edge shifts next bit of data into the shift register
		PORTB |= 0x02;
	}
	// set RCLK = 1. Rising edge copies data from “Shift” register to “Storage” register
	PORTB |= 0x04;
	// clears all lines in preparation of a new transmission
	PORTB = 0x00;
}

double play_note(unsigned char note)
{
	double freq = 0;
	if(note == 0)
	{
		freq = 261.63;
	}
	else if(note == 1)
	{
		freq = 293.66;
	}
	else if(note == 2)
	{
		freq = 329.63;
	}
	else if(note == 3)
	{
		freq = 349.23;
	}
	else if(note == 4)
	{
		freq = 392.00;
	}
	else if(note == 5)
	{
		freq = 440.00;
	}
	else if(note == 6)
	{
		freq = 493.88;
	}
	else if(note == 7)
	{
		freq = 523.25;
	}
	
	return freq;
}


void set_PWM(double frequency) {
	
	
	// Keeps track of the currently set frequency
	// Will only update the registers when the frequency
	// changes, plays music uninterrupted.
	static double current_frequency;
	if (frequency != current_frequency) {

		if (!frequency) TCCR3B &= 0x08; //stops timer/counter
		else TCCR3B |= 0x03; // resumes/continues timer/counter
		
		// prevents OCR3A from overflowing, using prescaler 64
		// 0.954 is smallest frequency that will not result in overflow
		if (frequency < 0.954) OCR3A = 0xFFFF;
		
		// prevents OCR3A from underflowing, using prescaler 64					// 31250 is largest frequency that will not result in underflow
		else if (frequency > 31250) OCR3A = 0x0000;
		
		// set OCR3A based on desired frequency
		else OCR3A = (short)(8000000 / (128 * frequency)) - 1;

		TCNT3 = 0; // resets counter
		current_frequency = frequency;
	}
}

void PWM_on() {
	TCCR3A = (1 << COM3A0);
	// COM3A0: Toggle PB6 on compare match between counter and OCR3A
	TCCR3B = (1 << WGM32) | (1 << CS31) | (1 << CS30);
	// WGM32: When counter (TCNT3) matches OCR3A, reset counter
	// CS31 & CS30: Set a prescaler of 64
	set_PWM(0);
}

void PWM_off() {
	TCCR3A = 0x00;
	TCCR3B = 0x00;
}

enum SM1_States { SM1_wait, SM1_Begin, SM1_HighScore };
int SMTick1(int state) {
	// Local Variables
	//State machine transitions
	switch (state) {
		// Wait for button press
		case SM1_wait:
		if(!GetBit(button,4)) //start game
		{
/*			button_press = 0;*/
			start = 1;
			lives = 3;
			score = '0';
			state = SM1_Begin;
		}
		break;
		// Button remains pressed
		case SM1_Begin:
		if(fail) //fail game
		{
			start = 0;
			state = SM1_HighScore;
			LCD_ClearScreen();
		}
		break;
		case SM1_HighScore:
		if(!GetBit(button,4)) //reset game
		{
/*			button_press = 0;*/
			fail = 0;
			state = SM1_wait;
			LCD_ClearScreen();
		}
		break;
		// default: Initial state
		default:
		state = SM1_wait;
		break;
	}
	//State machine actions
	switch(state) {
		case SM1_wait: 
		LCD_DisplayString(1, "    Frogger!     Press to start");
		break;
		case SM1_Begin:
		LCD_ClearScreen();
		LCD_DisplayString(1, "Score: ");
		LCD_Cursor(7);
		LCD_WriteData(score);
		break;
		case SM1_HighScore:
		LCD_ClearScreen();
		LCD_DisplayString(1,"   Game Over!       Score: ");
		LCD_Cursor(28);
		LCD_WriteData(score);
		LCD_Cursor(1);
		break;
		default: break;
	}
	return state;
}

enum SM2_States { SM2_wait, SM2_Begin };
int SMTick2(int state) {
	// Local Variables
	static unsigned char led_sample = 0x81;
	static unsigned char shift = 0;
	const unsigned int led_speed = 500;
	static unsigned int led_tick = 0;
	
	static unsigned char coin_row;
	static unsigned char coin_col;
	
	static unsigned char frog_row = frog_start_row;
	static unsigned char frog_col = frog_start_col;
	const unsigned char frog_speed = 160;
	static unsigned char frog_tick = 0;
	
	static unsigned char temp = 0;
	static unsigned char stage = 0;
	
	const unsigned char start_ob1_row = 0x02;
	const unsigned char start_ob1_col = 0x81;
	static unsigned char ob1_row = start_ob1_row;
	static unsigned char ob1_col = start_ob1_col;
	const unsigned char ob1_speed = 200;
	static unsigned int ob1_tick = 0;
	
	const unsigned char start_ob2_row = 0x08;
	const unsigned char start_ob2_col = 0x11;
	static unsigned char ob2_row = start_ob2_row;
	static unsigned char ob2_col = start_ob2_col;
	const unsigned char ob2_speed = 200;
	static unsigned int ob2_tick = 0;
	
	const unsigned char start_ob3_row = 0x10;
	const unsigned char start_ob3_col = 0x43;
	static unsigned char ob3_row = start_ob3_row;
	static unsigned char ob3_col = start_ob3_col;
	const unsigned int ob3_speed = 300;
	static unsigned int ob3_tick = 0;
	
	const unsigned char start_ob4_row = 0x40;
	const unsigned char start_ob4_col = 0xB4;
	static unsigned char ob4_row = start_ob4_row;
	static unsigned char ob4_col = start_ob4_col;
	const unsigned int ob4_speed = 400;
	static unsigned int ob4_tick = 0;
	
	//State machine transitions
	switch (state) {
		// Wait for button press
		case SM2_wait:
		if(start){
		frog_row = frog_start_row;
		frog_col = frog_start_col;
		
		ob1_row = start_ob1_row;
		ob2_row = start_ob2_row;
		ob3_row = start_ob3_row;
		ob4_row = start_ob4_row;
		
		stage=rand()%5;		
		
		if(stage == 0){
			
			ob1_col = start_ob1_col;
			
			ob2_col = start_ob2_col;

			ob3_col = start_ob3_col;
			
			ob4_col = start_ob4_col;
		}
		
		else if(stage == 1){
			ob1_col = start_ob2_col;
			
			ob2_col = start_ob4_col;

			ob3_col = start_ob1_col;
			
			ob4_col = start_ob3_col;
		}
		else if(stage == 2){
			ob1_col = start_ob3_col;
			
			ob2_col = start_ob4_col;

			ob3_col = start_ob1_col;
			
			ob4_col = start_ob2_col;
		}
		else if(stage == 3){
			ob1_col = start_ob2_col;
			
			ob2_col = start_ob3_col;

			ob3_col = start_ob1_col;
			
			ob4_col = start_ob4_col;
		}
		else if(stage == 4){
			ob1_col = start_ob3_col;
			
			ob2_col = start_ob1_col;

			ob3_col = start_ob4_col;
			
			ob4_col = start_ob2_col;
		}
		else if(stage == 5){
			ob1_col = start_ob4_col;
			
			ob2_col = start_ob3_col;

			ob3_col = start_ob2_col;
			
			ob4_col = start_ob1_col;
		}
		coin_row = (2<<(rand()%6));
		coin_col = (1<<(rand()%8));
		state = SM2_Begin;
		}
		break;
		// Button remains pressed
		case SM2_Begin:
		if(fail){
			state = SM2_wait;
		}
		break;
		// default: Initial state
		default:
		state = SM2_wait;
		break;
	}
	//State machine actions
	switch(state) {
		case SM2_wait:
			transmit_data(~led_sample,led_sample);
			if(led_tick == (led_speed-1)){
			led_sample = (led_sample<<shift) | (led_sample>>shift);
			shift++;
			if(shift == 10){
				led_sample = 0x81;
				shift = 0;
			}
			}
			led_tick++;
			if(led_tick == led_speed) led_tick = 0;
		break;
		case SM2_Begin: //Game Begins
				if(lives == 3)
				{
					PORTA = ~0x4F;
				}
				if(lives == 2)
				{
					PORTA = ~0x5B;
				}
				if(lives == 1)
				{
					PORTA = ~0x06;
				}
				transmit_data(~ob1_col,ob1_row);
				transmit_data(~ob2_col,ob2_row);
				transmit_data(~ob3_col,ob3_row);
				transmit_data(~ob4_col,ob4_row);
				transmit_data(~frog_col,frog_row);
				transmit_data(~coin_col,coin_row);
			if(frog_tick == (frog_speed-1)){
			if(!GetBit(button,0)) frog_col = frog_col<<1;
			else if(!GetBit(button,1)) frog_row = frog_row<<1;
			else if((!GetBit(button,2))  && frog_row != frog_start_row) frog_row = frog_row>>1;
			else if(!GetBit(button,3)) frog_col = frog_col>>1;
			}
			if((frog_col == coin_col) && (frog_row == coin_row))
			{
				coin = 1;
				coin_row = 0;
				coin_col = 0;
			}
			//check falling off edge
			if(frog_col == 0)
			{
				lives--;
				frog_row = frog_start_row;
				frog_col = frog_start_col;
			}
			else if(frog_row == 0x80)
			{
				if(coin){
					score++;
					coin = 0;
				}
				state = SM2_wait;
			}
			
			//moves to right
			if(ob1_tick == (ob1_speed-1)){
			temp = GetBit(ob1_col, 0);
			ob1_col >>= 1;
			ob1_col = (ob1_col) | (temp<<7);
			}
			if((frog_row == ob1_row)){
				if((frog_col & ob1_col) != 0){
					lives--;
					frog_row = frog_start_row;
					frog_col = frog_start_col;	
				}
			}
			
			//moves to left
			if(ob2_tick == (ob2_speed-1)){
			temp = GetBit(ob2_col, 7);
			ob2_col <<= 1;
			ob2_col = (ob2_col) | (temp);
			}
			if((frog_row == ob2_row)){
				if((frog_col & ob2_col) != 0){
					lives--;
					frog_row = frog_start_row;
					frog_col = frog_start_col;
				}
			}
			
			//moves to right
			if(ob3_tick == (ob3_speed-1)){
			temp = GetBit(ob3_col, 0);
			ob3_col >>= 1;
			ob3_col = (ob3_col) | (temp<<7);
			}
			if((frog_row == ob3_row)){
				if((frog_col & ob3_col) != 0){
					lives--;
					frog_row = frog_start_row;
					frog_col = frog_start_col;	
					}
			}
			
			//moves to left
			if(ob4_tick == (ob4_speed-1)){
			temp = GetBit(ob4_col, 7);
			ob4_col <<= 1;
			ob4_col = (ob4_col) | (temp);
			}
			if((frog_row == ob4_row)){
				if((frog_col & ob4_col) != 0){
					lives--;
					frog_row = frog_start_row;
					frog_col = frog_start_col;
				}
			}
			
			//Speed of obsticals
			frog_tick++;
			if(frog_tick == frog_speed)
				frog_tick = 0;
			ob1_tick++;
			if(ob1_tick == ob1_speed)
				ob1_tick = 0;
			ob2_tick++;
			if(ob2_tick == ob2_speed)
				ob2_tick = 0;
			ob3_tick++;
			if(ob3_tick == ob3_speed)
				ob3_tick = 0;
			ob4_tick++;
			if(ob4_tick == ob4_speed)
				ob4_tick = 0;
			
			if(lives == 0)
			fail = 1;
		break;
		default: break;
	}
	return state;
}

enum SM3_States {SM3_wait, SM3_Press};
int SMTick3(int state) {
		
	switch (state) {
		case SM3_wait:

		if(button_press == 0){
			if(!GetBit(button,0))
			{
				button_press = 1;
				state = SM3_Press;
			}
			if(!GetBit(button,1))
			{
				button_press = 2;
				state = SM3_Press;
			}
			if(!GetBit(button,2))
			{
				button_press = 3;
				state = SM3_Press;
			}
			if(!GetBit(button,3))
			{
				button_press = 4;
				state = SM3_Press;
			}
			if(!GetBit(button,4))
			{
				button_press = 5;
				state = SM3_Press;
			}
		}
		break;
		case SM3_Press:
		if(button == 0xFF)
			state = SM3_wait;
		default:
		state = SM3_wait;
		break;
	}
	
	switch (state) {
		case SM3_wait:
		break;
		case SM3_Press:
		break;
		default:
		break;
	}
}

int main(void)
{
	
	DDRA = 0xFF; PORTA = 0x00;
	DDRB = 0xFF; PORTB = 0x00;
	DDRC = 0xFF; PORTC = 0x00;
	DDRD = 0xC0; PORTD = 0x3F;
	
	// Period for the tasks
	unsigned long int SMTick1_calc = 1000;
	unsigned long int SMTick2_calc = 1;
	unsigned long int SMTick3_calc = 50;
	//Calculating GCD
	unsigned long int tmpGCD = 1;
	
	//Greatest common divisor for all tasks
	// or smallest time unit for tasks.
	unsigned long int GCD = tmpGCD;
	
	//Recalculate GCD periods for scheduler
	unsigned long int SMTick1_period = SMTick1_calc/GCD;
	unsigned long int SMTick2_period = SMTick2_calc/GCD;
	unsigned long int SMTick3_period = SMTick3_calc/GCD;
	//Declare an array of tasks
	static task task1, task2, task3;
	task *tasks[] = {&task1, &task2, &task3};
	const unsigned short numTasks =
	sizeof(tasks)/sizeof(task*);
	
	// Task 1
	task1.state = -1;
	task1.period = SMTick1_period;
	task1.elapsedTime = SMTick1_period;
	task1.TickFct = &SMTick1;
	
	// Task 2
	task2.state = -1;
	task2.period = SMTick2_period;
	task2.elapsedTime = SMTick2_period;
	task2.TickFct = &SMTick2;
	
 	// Task 3
 	task3.state = -1;
 	task3.period = SMTick3_period;
 	task3.elapsedTime = SMTick3_period;
 	task3.TickFct = &SMTick3;
		
	
	TimerSet(GCD);
	TimerOn();
	

	LCD_init();
	while(1) {
		// Scheduler code
		for (unsigned char i = 0; i < numTasks; i++ ) {
			button = PIND;
			// Task is ready to tick
			if ( tasks[i]->elapsedTime ==
			tasks[i]->period ) {
				// Setting next state for task
				tasks[i]->state =
				tasks[i]->TickFct(tasks[i]->state);
				// Reset elapsed time for next tick.
				tasks[i]->elapsedTime = 0;
			}
			tasks[i]->elapsedTime += 1;
		}
		while(!TimerFlag);
		TimerFlag = 0;
	}
	
	return 0;
}