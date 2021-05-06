
// Lab10.c
// Runs on TM4C123
// Jonathan Valvano and Daniel Valvano
// This is a starter project for the EE319K Lab 10

// Last Modified: 1/16/2021 
// http://www.spaceinvaders.de/
// sounds at http://www.classicgaming.cc/classics/spaceinvaders/sounds.php
// http://www.classicgaming.cc/classics/spaceinvaders/playguide.php
/* 
 Copyright 2021 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */
// ******* Possible Hardware I/O connections*******************
// Slide pot pin 1 connected to ground
// Slide pot pin 2 connected to PD2/AIN5
// Slide pot pin 3 connected to +3.3V 
// fire button connected to PE0
// special weapon fire button connected to PE1
// 8*R resistor DAC bit 0 on PB0 (least significant bit)
// 4*R resistor DAC bit 1 on PB1
// 2*R resistor DAC bit 2 on PB2
// 1*R resistor DAC bit 3 on PB3 (most significant bit)
// LED on PB4
// LED on PB5

// VCC   3.3V power to OLED
// GND   ground
// SCL   PD0 I2C clock (add 1.5k resistor from SCL to 3.3V)
// SDA   PD1 I2C data

//************WARNING***********
// The LaunchPad has PB7 connected to PD1, PB6 connected to PD0
// Option 1) do not use PB7 and PB6
// Option 2) remove 0-ohm resistors R9 R10 on LaunchPad
//******************************

#include <stdint.h>
#include "../inc/tm4c123gh6pm.h"
#include "../inc/CortexM.h"
#include "SSD1306.h"
#include "Print.h"
#include "Random.h"
#include "ADC.h"
#include "Images.h"
#include "Sound.h"
#include "Timer0.h"
#include "Timer1.h"
#include "TExaS.h"
#include "Switch.h"
//********************************************************************************
// debuging profile, pick up to 7 unused bits and send to Logic Analyzer
#define PB54                  (*((volatile uint32_t *)0x400050C0)) // bits 5-4
#define PF321                 (*((volatile uint32_t *)0x40025038)) // bits 3-1
// use for debugging profile
#define PF1       (*((volatile uint32_t *)0x40025008))
#define PF2       (*((volatile uint32_t *)0x40025010))
#define PF3       (*((volatile uint32_t *)0x40025020))
#define PB5       (*((volatile uint32_t *)0x40005080)) 
#define PB4       (*((volatile uint32_t *)0x40005040)) 
// TExaSdisplay logic analyzer shows 7 bits 0,PB5,PB4,PF3,PF2,PF1,0 
// edit this to output which pins you use for profiling
// you can output up to 7 pins
void LogicAnalyzerTask(void){
  UART0_DR_R = 0x80|PF321|PB54; // sends at 10kHz
}
void ScopeTask(void){  // called 10k/sec
  UART0_DR_R = (ADC1_SSFIFO3_R>>4); // send ADC to TExaSdisplay
}
// edit this to initialize which pins you use for profiling
void Profile_Init(void){
  SYSCTL_RCGCGPIO_R |= 0x22;      // activate port B,F
  while((SYSCTL_PRGPIO_R&0x20) != 0x20){};
  GPIO_PORTF_DIR_R |=  0x0E;   // output on PF3,2,1 
  GPIO_PORTF_DEN_R |=  0x0E;   // enable digital I/O on PF3,2,1
  GPIO_PORTB_DIR_R |=  0x30;   // output on PB4 PB5
  GPIO_PORTB_DEN_R |=  0x30;   // enable on PB4 PB5  
}
//********************************************************************************

void Delay100ms(uint32_t count); // time delay in 0.1 seconds
typedef enum{dead,alive} status_t;
struct sprite{ //enumerated type alive or dead allows us to know if we want to write it
int32_t x;   //this is the horizontal position
int32_t y;  //this is the vertial position
int32_t vx; //velocity in the x direction
int32_t vy;	 //velocity in the y direction
const uint8_t *image; //pointer to the image array in image.h
status_t life;  //life status important to choose alive or dead
};
typedef struct sprite sprite_t; //defines the type
sprite_t Enemys[18]; //array of 18 enemies 
sprite_t PCar; //sprite of the racing car!
sprite_t col;//explosion 
int collide = 0; //main loops executes while this is 0 - if set to 1 game ends
int speed = 14; //speed initially 20 pixels/second, increases/decreases depending on button input
int NeedToDraw;//1 need to draw 0 not
void Init(void){ int i; //Initializes the spirites - col structure is used for explosion animation later
	col.y = 0; //sets this to 0, at the end this will be used
	col.x = 0;
	col.image  = BigExplosion0;
	col.life = dead;
	
	PCar.y = 40; //initial position of the racer near the middle of the screen, to the left
	PCar.x = 108;
	PCar.image = Maincar;
	PCar.life = alive;
  for(i=0;i<3;i++){ //first three cars 30 vertical units apart
		Enemys[i].x = 10;
		Enemys[i].y = 30*i;
    Enemys[i].image = Alien10pointA;
    Enemys[i].life = alive;
  }
	
  for(i=6;i<18;i++){
    Enemys[i].life = dead; //rest dead
   }
}

void Move(void){int i;
uint32_t adcData; 
	if(PCar.life == alive){
		NeedToDraw = 1; //sets semaphore
		adcData = ADC_In(); //samples ADC data
	//	PCar.y = (64-(((64-8)*adcData)/4095)); //divides by a funky number so it moves in larger intervals - aids gameplay
	PCar.y =	(((64-8)*adcData)/3000+9);
	}
	
uint32_t speeddown;
uint32_t speedup; 
NeedToDraw = 1;
speedup = Switch_In()&0x01; //PE0
speeddown = Switch_In()&0x02; //PE1
	if(speedup == 0x01){ //checks PE0 pressed for speed up
		speed++;
		Sound_Fast();
		if(speed > 15){ //sets cap at 80 pixels per second
			speed = 15;
		}
	}
	 if(speeddown == 0x02){ //checks PE1 pressed for slow down
		speed--;
		Sound_Slow();
			if(speed <1){ //can't be lower than 1
			speed++;
			}
	 }
	 
	for(i=0;i<3;i++){ //for the three enemies
		if(Enemys[i].x >= 127){ //sets the enemy position back after it went all the way
			Enemys[i].x = 0;
			Enemys[i].y += 13; 
		}
	
		if(Enemys[i].y >= 62){ 
			Enemys[i].y = 30*i;
		}
		else{
		Enemys[i].x += speed;
		}
	}
}

 void Collisions(void){int x1, y1, y2, x2;
	x2 = PCar.x + 6;
	y2 = PCar.y + 4;
	for(int i = 0; i<3; i++){
		x1 = Enemys[i].x + 6;
		y1 = Enemys[i].y + 4;
		if((x1 >> 4) == (x2 >> 4) && (y1 >> 3) == (y2 >> 3)){
			col.x = PCar.x;
			col.y = PCar.y;
			col.life = alive;
			collide = 1;
			Sound_Explosion();
		}
	}
}
uint32_t TimeScore = 0; //used
void DDraw(void){int i;
	SSD1306_ClearBuffer();
	if(col.life == alive){
	SSD1306_DrawBMP(col.x - 12,col.y,col.image,0,SSD1306_INVERSE);
	}
	if(PCar.life == alive){
	SSD1306_DrawBMP(PCar.x,PCar.y,PCar.image,0,SSD1306_INVERSE);
	}
	for(i=0;i<18;i++){
		if(Enemys[i].life){
		SSD1306_DrawBMP(Enemys[i].x,Enemys[i].y,Enemys[i].image,0,SSD1306_INVERSE);
		}
	}
//	SSD1306_SetCursor(0,0);
//	SSD1306_OutString("Time alive:   ");
//	SSD1306_SetCursor(2,0);
//	SSD1306_OutUDec(TimeScore);
	SSD1306_OutBuffer();//takes 25ms
	NeedToDraw = 0;//semaphore
}

void SysTick_Init(){
NVIC_ST_CTRL_R = 0;					// disable SysTick for init
NVIC_ST_RELOAD_R = 3999999;	// 20 Hz on 80MHz clock
NVIC_ST_CURRENT_R = 0;			// clear CURRENT
NVIC_ST_CTRL_R = 0x07;

};

void SysTick_Handler(void){
	Move();
	Collisions();
}

void Keep_Score(void){
	if(collide == 0){
		TimeScore++;
	}
	SSD1306_SetCursor(6,0);
	SSD1306_OutUDec(TimeScore);
	NVIC_DIS0_R = 1<<20; //IRQ 20 disabled in NVIC
}

void Score_Init(void){
	Timer1_Init(&Keep_Score, 16000000); //1 Hz
}

void Score_Start(void){
	NVIC_EN0_R = 1<<20; //enables IRQ 20 in NVIC
}

uint32_t lang;
void Language(void){
	SSD1306_OutClear();
	SSD1306_SetCursor(1, 1);
  SSD1306_OutString("Press Left button");
	SSD1306_SetCursor(1, 2);
  SSD1306_OutString("for English");
  SSD1306_SetCursor(1, 4);
  SSD1306_OutString("Presione el boton");
	SSD1306_SetCursor(1, 5);
  SSD1306_OutString("derecho para espanol");
	while(Switch_In() == 0){}
		if(Switch_In() == 0x01){
			lang = 1;
		}
		if(Switch_In() == 0x02){
			lang = 2;
		}
}

int main(void){
  DisableInterrupts();
  // pick one of the following three lines, all three set to 80 MHz
  //PLL_Init();                   // 1) call to have no TExaS debugging
 // TExaS_Init(&LogicAnalyzerTask); // 2) call to activate logic analyzer
  //TExaS_Init(&ScopeTask);       // or 3) call to activate analog scope PD2
  SSD1306_Init(SSD1306_SWITCHCAPVCC);
  SSD1306_OutClear();   
  Random_Init(1);
  Profile_Init(); // PB5,PB4,PF3,PF2,PF1 
  SSD1306_ClearBuffer();
  SSD1306_DrawBMP(2, 62, SpaceInvadersMarquee, 0, SSD1306_WHITE);
  SSD1306_OutBuffer();
  Delay100ms(5);
	Switch_Init(); //Initializes PE0 and PE1 for speeding up and slowing down the car
//	Language();
	SSD1306_OutClear();
	ADC_Init(4); //16 element averaging
	Sound_Init(); //Initializes timer 0 for sound
  Score_Init();
	Init();  //Initializes the positions of the first sprites and structures
	SysTick_Init(); //Initializes an interrupt for drawing 20 times a second
	EnableInterrupts();

  while(collide == 0){
		if(NeedToDraw == 1){	
			DDraw();
			Score_Start();
		}
  }
	
	if(lang == 1){
	SSD1306_OutClear();  
  SSD1306_SetCursor(1, 1);
  SSD1306_OutString("GAME OVER");
  SSD1306_SetCursor(1, 2);
  SSD1306_OutString("Nice try Racer!");
  SSD1306_SetCursor(1, 3);
  SSD1306_OutString("You stayed alive");
  SSD1306_SetCursor(0, 4);
  SSD1306_OutUDec(TimeScore);
	SSD1306_SetCursor(1, 5);
	SSD1306_OutString("seconds");
  }
	else if(lang == 2){
	SSD1306_OutClear();  
  SSD1306_SetCursor(1, 1);
  SSD1306_OutString("Juego Terminado");
  SSD1306_SetCursor(1, 2);
  SSD1306_OutString("Buen intento");
  SSD1306_SetCursor(1, 3);
  SSD1306_OutString("Te quedaste vivo");
  SSD1306_SetCursor(1, 4);
  SSD1306_OutUDec(TimeScore);
	SSD1306_SetCursor(1, 5);
	SSD1306_OutString("segundos");
  }
	else{
	SSD1306_OutClear();  
  SSD1306_SetCursor(1, 1);
  SSD1306_OutString("GAME OVER");
  SSD1306_SetCursor(1, 2);
  SSD1306_OutString("Nice try Racer!");
  SSD1306_SetCursor(1, 3);
  SSD1306_OutString("You stayed alive");
  SSD1306_SetCursor(0, 4);
  SSD1306_OutUDec(TimeScore);
	SSD1306_SetCursor(1, 5);
	SSD1306_OutString("seconds");
	
	
	}
}


// You can't use this timer, it is here for starter code only 
// you must use interrupts to perform delays
void Delay100ms(uint32_t count){uint32_t volatile time;
  while(count>0){
    time = 727240;  // 0.1sec at 80 MHz
    while(time){
	  	time--;
    }
    count--;
  }
}

