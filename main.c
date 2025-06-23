//------------------------------------------- main.c CODE STARTS -------------------------------------------------------------------------------------
#include <stdio.h>
#include <stdbool.h>
#include "NUC100Series.h"
#include "MCU_init.h"
#include "SYS_init.h"

#define HXT_STATUS 1<<0
#define PLL_STATUS 1<<2
#define PLLCON_FB_DV_VAL 10
#define CPUCLOCKDIVIDE 1
#define TIMER0_COUNTS 100000-1


volatile int decisecond = 0;
volatile int second0 = 0; //seconds in ones
volatile int second1 = 0; //seconds in tens
volatile int minute = 0;
volatile int decisecond_lap[5] = {}; //Initialize blank array to store decisecond counts
volatile int second0_lap[5] = {}; //Initialize blank array to store second in ones counts
volatile int second1_lap[5] = {};	//Initialize blank array to store second in tens counts
volatile int lap_order = 0;
volatile int lap_count = 0;
volatile bool lap_mode = false;
volatile int decisecond_lap_disp, second0_lap_disp, second1_lap_disp;


//Enum declaration as states for FSM
enum fsm_state {Idle_mode = 1, Count_mode = 2, Pause_mode = 3, Check_mode = 4};
enum fsm_state state;


void System_Config(void);
void KeyPadEnable(void);
void GPIO_Config(void);
void Button_Debounce(void);
uint8_t KeyPadScanning(void);
void seventSegment1(int first_7segments_count);
void seventSegment2(int second_7segments_count);
void seventSegment3(int third_7segments_count);
void seventSegment4(int fourth_7segments_count);
      
//Gloabl Array to display on 7segment for NUC140 MCU
int pattern[] = {
               // gedbaf_dot_c
                  0b10000010, //Number 0 // ---a----
                  0b11101110, //Number 1 // |      |
                  0b00000111, //Number 2 // f      b
                  0b01000110, //Number 3 // |      |
                  0b01101010, //Number 4 // ---g----
                  0b01010010, //Number 5 // |      |
                  0b00010010, //Number 6 // e      c
                  0b11100110, //Number 7 // |      |
                  0b00000010, //Number 8 // ---d----
                  0b01000010, //Number 9
                  0b11111111 //Blank LED
                };  
int pattern2[] = {
               // gedbaf_dot_c
                  0b10000000, //Number 0. // ---a----
                  0b11101100, //Number 1. // |      |
                  0b00000101, //Number 2. // f      b
                  0b01000100, //Number 3. // |      |
                  0b01101000, //Number 4. // ---g----
                  0b01010000, //Number 5. // |      |
                  0b00010000, //Number 6. // e      c
                  0b11100100, //Number 7. // |      |
                  0b00000000, //Number 8. // ---d----
                  0b01000000, //Number 9.
                  0b11111111 //Blank LED
                };  

int main(void)
{  			
	uint8_t pressed_key = 0; //Declaration for key_press in matrix
	//Call the created functions
	System_Config();
	KeyPadEnable();	
  	GPIO_Config();
	Button_Debounce();
	state = 1; //Initially work at IDLE mode

	while(1){
		switch(state){
			//IDLE MODE
			case 1:
				//Turn on Led 5, the others are off
				PC->DOUT &= ~(1<<12);
				PC->DOUT |= (1<<13);
				PC->DOUT |= (1<<14);
				//Reset variables at this state for the stopwatch to reset all 7-segements leds to 0 when Key 9 is pressed at Pause mode
				decisecond = 0;
				second0 = 0;
				second1 = 0;
				minute = 0;
						
				PC->DOUT &= ~(1<<12); // Turn on LED 5 after reset
				
				TIMER0->TCSR &= ~(1 << 30); //Stop counting
				//Display 4 digits of 7-segments
				seventSegment1(decisecond);
				seventSegment2(second0);
				seventSegment3(second1);
				seventSegment4(minute);
				
				pressed_key = KeyPadScanning(); //Check for key press
				if(pressed_key == 1){ //If press Key 1
					state = 2; //Switch to COUNT mode							
				}	
				break;
			
				//COUNT MODE
			case 2:
				//Turn on Led 6, the others are off
				PC->DOUT |= (1<<12);
				PC->DOUT &= ~(1<<13);
				PC->DOUT |= (1<<14);
				TIMER0->TCSR |= (1 << 30);	//Start/Continue counting
				//Display 4 digits of 7-segments
				seventSegment1(decisecond);
				seventSegment2(second0);
				seventSegment3(second1);
				seventSegment4(minute);
			
				pressed_key = KeyPadScanning(); //Check for key press
				if(pressed_key == 1){ //If press Key 1
					state = 3; //Switch to PAUSE mode				
				}
				
				else if(pressed_key == 9){ //If press Key 9
					//Assign the variables' values being count to created lap array
					decisecond_lap[lap_order] = decisecond;
					second0_lap[lap_order] = second0;
					second1_lap[lap_order] = second1;
					lap_order++;
					if(lap_order == 5) {
						lap_order = 0; }
					CLK_SysTickDelay(200000); //Delay to reduce button bouncing time to get the proper lap time in an array
					
				}
				break;
				
			//PAUSE MODE	
			case 3:
				//Turn on Led 7, the others are off
				PC->DOUT |= (1<<13);
				PC->DOUT &= ~(1<<14);
				PC->DOUT |= (1<<12);
				TIMER0->TCSR &= ~(1 << 30);	// Stop counting
				//Display 4 digits of 7-segments
				seventSegment1(decisecond);
				seventSegment2(second0);
				seventSegment3(second1);
				seventSegment4(minute);
				
				pressed_key = KeyPadScanning(); //Check for key press
				if(pressed_key == 1){ //If press Key 1
					state = 2; //Switch to COUNT mode	
					
				}
				else if(pressed_key == 9){ //If press Key 9
					state = 1; //Switch to IDLE mode	
					
				}
				else if(pressed_key == 5){ //If press Key 5
					state = 4; 
					//CLK_SysTickDelay(1000);	
				}
				break;
				
		case 4:
			//Dislay lap 2 to 5 when GPB15 is pressed
				if(lap_mode == true){
					decisecond_lap_disp = decisecond_lap[lap_count];
					second0_lap_disp = second0_lap[lap_count];
					second1_lap_disp = second1_lap[lap_count];
					//Display 4 digits of 7-segments with the recorded time values
					seventSegment1(decisecond_lap_disp);
					seventSegment2(second0_lap_disp);
					seventSegment3(second1_lap_disp);
					seventSegment4(lap_count + 1);					
				}
				//Dislay lap 1 when GPB15 is not pressed
				else{ 
					decisecond_lap_disp = decisecond_lap[lap_count];
					second0_lap_disp = second0_lap[lap_count];
					second1_lap_disp = second1_lap[lap_count];
					//Display 4 digits of 7-segments with the recorded time values
					seventSegment1(decisecond_lap_disp);
					seventSegment2(second0_lap_disp);
					seventSegment3(second1_lap_disp);
					seventSegment4(lap_count + 1);
				}
					
				lap_mode = false; 
				
				pressed_key = KeyPadScanning(); //Check for key press
				if(pressed_key == 5){ //If press Key 5
					state = 3; //Switch to PAUSE mode		
				}
				break;
				
		}
	} 		
	}
		

//------------------------------------------- main.c CODE ENDS ---------------------------------------------------------------------------------------

void System_Config(void)
{
	SYS_UnlockReg(); // Unlock protected registers
 
    //Enable clock sources
    CLK->PWRCON |= 1<<0;
    while(!(CLK->CLKSTATUS & HXT_STATUS));
 
 //PLL configuration starts
 CLK->PLLCON &= (~(1<<19)); // 0: PLL input is HXT 12MHz (default). 1: PLL input is HIRC 22MHz
 CLK->PLLCON &= (~(1<<16)); // 0: PLL in normal mode. 1: PLL in power-down mode (default)
 CLK->PLLCON &= (~(0xFFFF << 0));
 CLK->PLLCON |= 0xC230; // PLLCON[15:0] = 0b1100_0010_0011_0000
 CLK->PLLCON &= (~(1<<18)); // 0: enable PLL clock out. 1: disable PLL clock (default)
 while(!(CLK->CLKSTATUS & PLL_STATUS)); // wait until PLLOUT is fully generated and enabled before selecting it for CPU/etc.
 //PLL configuration ends
 //CPU clock source selection configuration starts
 CLK->CLKSEL0 &= (~(0b111<<0));
 CLK->CLKSEL0 |= 0b010; // select PLLOUT for CPU
 CLK->PWRCON &= ~(1<<7);// Normal mode
 CLK->CLKDIV &= (~(0xF<<0)); // ensure that PLLOUT is not further divided
 //CPU clock source selection configuration ends
                                                
//Timer 0 initialization start--------------
//TM0 Clock selection and configuration
CLK->CLKSEL1 &= ~(0b111 << 8);
CLK->APBCLK |= (1 << 2);
//Pre-scale =11
TIMER0->TCSR &= ~(0xFF << 0);
TIMER0->TCSR |= 11 << 0;
//reset Timer 0
TIMER0->TCSR |= (1 << 26);

//define Timer 0 operation mode
TIMER0->TCSR &= ~(0b11 << 27);
TIMER0->TCSR |= (0b01 << 27);
TIMER0->TCSR &= ~(1 << 24);

//TDR to be updated continuously while timer counter is counting
TIMER0->TCSR |= (1 << 16);
//Enable TE bit (bit 29) of TCSR
//The bit will enable the timer interrupt flag TIF
TIMER0->TCSR |= (1 << 29);
//TimeOut = 0.1s --> Counter's TCMPR = 100000-1
TIMER0->TCMPR = TIMER0_COUNTS;


// TM0 interrut setup
NVIC->ISER[0] |= (1 << 8);
NVIC->IP[2] &= (~(3 << 7));
//Timer 0 initialization end----------------
PC->PMD &= ~(0b11<< 30);
PC->PMD |= (0b01<<30);
 
SYS_LockReg(); // Lock protected registers
}

void KeyPadEnable(void){
	GPIO_SetMode(PA, BIT0, GPIO_MODE_QUASI);
	GPIO_SetMode(PA, BIT1, GPIO_MODE_QUASI);
	GPIO_SetMode(PA, BIT2, GPIO_MODE_QUASI);
	GPIO_SetMode(PA, BIT3, GPIO_MODE_QUASI);
	GPIO_SetMode(PA, BIT4, GPIO_MODE_QUASI);
	GPIO_SetMode(PA, BIT5, GPIO_MODE_QUASI);
}
void GPIO_Config(void)
{	
	//Configure GPIO for LED 5,6,7
	PC->PMD &= (~(0b11<< 24));
  PC->PMD |= (0b01 << 24);
	PC->PMD &= (~(0b11<< 26));
  PC->PMD |= (0b01 << 26);
	PC->PMD &= (~(0b11<< 28));
  PC->PMD |= (0b01 << 28);
	//PC->PMD &= (~(0b11<< 30));
  //PC->PMD |= (0b01 << 30);

	
	//Configure GPIO for 7segment
	//Set mode for PC4 to PC7 
  PC->PMD &= (~(0xFF<< 8));		//clear PMD[15:8] 
  PC->PMD |= (0b01010101 << 8);   //Set output push-pull for PC4 to PC7
	//Set mode for PE0 to PE7
	PE->PMD &= (~(0xFFFF<< 0));		//clear PMD[15:0] 
	PE->PMD |= 0b0101010101010101<<0;   //Set output push-pull for PE0 to PE7
	//GPIO Interrupt configuration. B.15 is the interrupt source
  PB->PMD &= (~(0b11 << 30)); // Set pin mode to input
  PB->IMD &= (~(1 << 15)); // Trigger on edge
  PB->IEN |= (1 << 15); // trigger edge type is falling
	//Firstly, turn off all the digits and wait for the next execution
	PC->DOUT &= ~(1<<7);     //Logic 0 to turn off the digit
	PC->DOUT &= ~(1<<6);		//SC3
	PC->DOUT &= ~(1<<5);		//SC2
	PC->DOUT &= ~(1<<4);		//SC1	
	//NVIC interrupt configuration for B.15 interrupt source
  NVIC->ISER[0] |= 1 << 3; // interrupt enable
  NVIC->IP[0] &= (~(0b11 << 30)); // ISR priority
}

void Button_Debounce(void){
	//Button Debounce
			PB->DBEN |= (1 << 15);
			PA->DBEN |= (1 << 0);
			PA->DBEN |= (1 << 1);
			PA->DBEN |= (1 << 2);
			PA->DBEN |= (1 << 3);
			PA->DBEN |= (1 << 4);
			PA->DBEN |= (1 << 5);
			GPIO->DBNCECON &= ~(1 << 4);
			GPIO->DBNCECON &= ~(0xF << 0);
			GPIO->DBNCECON |= (0xC << 0);
	}		

uint8_t KeyPadScanning(void){
	PA0=1; PA1=1; PA2=0; PA3=1; PA4=1; PA5=1;
	if (PA3==0) return 1;
	if (PA4==0) return 4;
	if (PA5==0) return 7;
	PA0=1; PA1=0; PA2=1; PA3=1; PA4=1; PA5=1;
	if (PA3==0) return 2;
	if (PA4==0) return 5;
	if (PA5==0) return 8;
	PA0=0; PA1=1; PA2=1; PA3=1; PA4=1; PA5=1;
	if (PA3==0) return 3;
	if (PA4==0) return 6;
	if (PA5==0) return 9;
	return 0;
}


void seventSegment1(int first_7segments_count){
	//Turn on digit 4 and turn off the others
	PC->DOUT &= ~(1<<7);     
	PC->DOUT &= ~(1<<6);		
	PC->DOUT &= ~(1<<5);		
	PC->DOUT |= (1<<4);
	
	PE->DOUT |= (0xFF); //Firstly, turn off all leds segments
	
	switch(first_7segments_count){
		case 0:
			PE->DOUT &= pattern[0];
			break;
		case 1:
			PE->DOUT &=pattern[1];
			break;
		case 2:
			PE->DOUT &=pattern[2];
			break;
		case 3:
			PE->DOUT &= pattern[3];
			break;
		case 4:
			PE->DOUT &= pattern[4];
			break;
		case 5:
			PE->DOUT &= pattern[5];
			break;
		case 6:
			PE->DOUT &= pattern[6];
			break;
		case 7:
			PE->DOUT &= pattern[7];
			break;
		case 8:
			PE->DOUT &= pattern[8];
			break;
		case 9:
			PE->DOUT &= pattern[9];
			break;
		default:
			PE->DOUT &= pattern[10];
			break;
	}
	CLK_SysTickDelay(900); //Delay for Multiplexing 4 digits of 7-segments with different number at the same time
}

void seventSegment2(int second_7segments_count){
	//Turn on digit 5 and turn off the others
	PC->DOUT &= ~(1<<7);     
	PC->DOUT &= ~(1<<6);		
	PC->DOUT |= (1<<5);		
	PC->DOUT &= ~(1<<4);
	
	PE->DOUT |= (0xFF); //Firstly, turn off all leds segments
	
	switch(second_7segments_count){
		case 0:
			PE->DOUT &= pattern2[0];
			break;
		case 1:
			PE->DOUT &=pattern2[1];
			break;
		case 2:
			PE->DOUT &=pattern2[2];
			break;
		case 3:
			PE->DOUT &= pattern2[3];
			break;
		case 4:
			PE->DOUT &= pattern2[4];
			break;
		case 5:
			PE->DOUT &= pattern2[5];
			break;
		case 6:
			PE->DOUT &= pattern2[6];
			break;
		case 7:
			PE->DOUT &= pattern2[7];
			break;
		case 8:
			PE->DOUT &= pattern2[8];
			break;
		case 9:
			PE->DOUT &= pattern2[9];
			break;
		default:
			PE->DOUT &= pattern2[10];
			break;
	}
	CLK_SysTickDelay(900); //Delay for Multiplexing 4 digits of 7-segments with different number at the same time
}

void seventSegment3(int third_7segments_count){
	//Turn on digit 6 and turn off the others
	PC->DOUT &= ~(1<<7);     
	PC->DOUT |=(1<<6);		
	PC->DOUT &= ~(1<<5);		
	PC->DOUT &= ~(1<<4);
	
	PE->DOUT |= (0xFF); //Firstly, turn off all leds segments
	
	switch(third_7segments_count){
		case 0:
			PE->DOUT &= pattern[0];
			break;
		case 1:
			PE->DOUT &=pattern[1];
			break;
		case 2:
			PE->DOUT &=pattern[2];
			break;
		case 3:
			PE->DOUT &= pattern[3];
			break;
		case 4:
			PE->DOUT &= pattern[4];
			break;
		case 5:
			PE->DOUT &= pattern[5];
			break;
		case 6:
			PE->DOUT &= pattern[6];
			break;
		case 7:
			PE->DOUT &= pattern[7];
			break;
		case 8:
			PE->DOUT &= pattern[8];
			break;
		case 9:
			PE->DOUT &= pattern[9];
			break;
		default:
			PE->DOUT &= pattern[10];
			break;
	}
	CLK_SysTickDelay(900); //Delay for Multiplexing 4 digits of 7-segments with different number at the same time
}

void seventSegment4(int fourth_7segments_count){
	//Turn on digit 7 and turn off the others
	PC->DOUT |= (1<<7);     
	PC->DOUT &= ~(1<<6);		
	PC->DOUT &= ~(1<<5);		
	PC->DOUT &= ~(1<<4);
	
	PE->DOUT |= (0xFF); //Firstly, turn off all leds segments
	
	switch(fourth_7segments_count){
		case 0:
			PE->DOUT &= pattern[0];
			break;
		case 1:
			PE->DOUT &=pattern[1];
			break;
		case 2:
			PE->DOUT &=pattern[2];
			break;
		case 3:
			PE->DOUT &= pattern[3];
			break;
		case 4:
			PE->DOUT &= pattern[4];
			break;
		case 5:
			PE->DOUT &= pattern[5];
			break;
		case 6:
			PE->DOUT &= pattern[6];
			break;
		case 7:
			PE->DOUT &= pattern[7];
			break;
		case 8:
			PE->DOUT &= pattern[8];
			break;
		case 9:
			PE->DOUT &= pattern[9];
			break;
		default:
			PE->DOUT &= pattern[10];
			break;
	}
	CLK_SysTickDelay(900); //Delay for Multiplexing 4 digits of 7-segments with different number at the same time
}
			 
void TMR0_IRQHandler(void){
	//PC->DOUT ^= (1<<15); //Toggle to check 1/10s ISR
      decisecond++;
	if (decisecond > 9){
		second0++;
		decisecond = 0;
	}
	if (second0 > 9){
		second1++;
		second0 = 0;
	}
		
	if(second1 > 5){
		minute++;
		second1= 0;}
	
	if(minute > 8) {
		minute = 0;}
    TIMER0->TISR |= (1 << 0); // Clear flag
}
void EINT1_IRQHandler(void) {
	//Increment when button hit count when it is pressed
	lap_mode = true;
	lap_count++;
	if(lap_count > 4) {
		lap_count = 0;
	}
	PB->ISRC |= (1 << 15);	// Clear interrupt flag
}