/*
 * source.c
 *
 *  Created on: Feb 12, 2025
 *      Author: Greg Valijan
 */
#include "tm4c123gh6pm.h"

void EnableGPIO();
void EnableUART();
void sendChar(char);
void lcd_write_data(unsigned char);
void lcd_write_cmd(unsigned char);
void lcd_print(unsigned char*, unsigned char);
void lcd_init();
void delay_us(int);
char keypad_input();
int string_to_int(const char *str);
int multiply_arr(const char *arr1, const char *arr2);
void print(char* string);

void EnableGPIO()
{
    //Port B
    SYSCTL_RCGCGPIO_R |= 0x2;       //Port B Clock
    GPIO_PORTB_DEN_R |= 0xFF;       //PB0 - PB7 Digital Enable
    GPIO_PORTB_DIR_R |= 0xFF;       //Port B output
    GPIO_PORTB_DATA_R = 0x0;

    //Port C
    SYSCTL_RCGCGPIO_R |= 0x4;       //Port C Clock
    GPIO_PORTC_DEN_R |= 0xF0;       //PC4 - PC7 Digital Enable
    GPIO_PORTC_DIR_R |= 0x0;        //Port C input
    GPIO_PORTC_PDR_R |= 0xF0;       //PC4 - PC7 Weak PD resistor

    //Port A
    SYSCTL_RCGCGPIO_R |= 0x1;       //Port A Clock
    GPIO_PORTA_DEN_R |= 0xC0;       //PA6 - PA7 Digital Enable
    GPIO_PORTA_DIR_R |= 0xC0;       //Port A output
    GPIO_PORTA_DATA_R = 0x0;

    //Port E
    SYSCTL_RCGCGPIO_R |= 0x10;      //Port E Clock
    GPIO_PORTE_DEN_R |= 0x1E;       //PE1 - PE4 Digital Enable For RS, RW, E
    GPIO_PORTE_DIR_R |= 0x1E;       //Port E output
}

void EnableUART()
{
    #define RCGCUART (*((volatile unsigned long *) 0x400FE618))
    #define RCGCGPIO (*((volatile unsigned long *) 0x400FE608))
    #define GPIODEN_A (*((volatile unsigned long *) 0x4000451C))
    #define GPIOAFSEL (*((volatile unsigned long *) 0x40004420))
    #define GPIOPCTL (*((volatile unsigned long *) 0x40004528))
    #define UARTCTL (*((volatile unsigned long *) 0x4000C030))
    #define UARTIBRD (*((volatile unsigned long *) 0x4000C024))
    #define UARTFBRD (*((volatile unsigned long *) 0x4000C028))
    #define UARTLCRH (*((volatile unsigned long *) 0x4000C02C))
    #define UARTCC (*((volatile unsigned long *) 0x4000CFC8))
    #define UARTDR (*((volatile unsigned long *) 0x4000C000)) //Define the UART Data Register
    #define UARTFR (*((volatile unsigned long *) 0x4000C018)) //Define the UART Flag Register

    RCGCUART |= 0x1;        //Enables UART module
    RCGCGPIO |= 0x1;        //Enables  GPIO Clock for Port A
    GPIOAFSEL |= 0x3;       //Enables alt Function selection for PA0 and PA1 by GPIOPCTRL
    GPIOPCTL |= 0x11;       //Sets PA0 and PA1 to their alt functions (UART RX0 and TX0 respectively)
    GPIODEN_A |= 0x3;       //Digital Enable for PA0 and PA1

    UARTCTL = 0x0;          //Temporarily Disable UART

    UARTIBRD = 104;         // Set the integer part of the baud rate
    UARTFBRD = 11;          // Set the fractional part of the baud rate
    UARTLCRH = (0x3<<5);    //Set UART to have 8 data bits (bits 5 and 6 are active), 1 stop bit and 0 parity bits
    UARTCC = 0x0;           //Set UART clock to System Clock (DEFAULT)

    UARTCTL = 0x301;        //Enable UART, Transmit and Receive
}

void sendChar(char data)
{
    while((UARTFR & 0x20) != 0);        //Runs when receive holding register is not full
    UARTDR = data;                      // data into DR
}

void lcd_write_data(unsigned char data){
    GPIO_PORTA_DATA_R |= (1<<6);            //Turn off RS to write to IR
    delay_us(1);
    GPIO_PORTA_DATA_R |= (1<<7);            //Turn on EN
    delay_us(1);
    GPIO_PORTB_DATA_R = data;               //Set Data
    delay_us(1);
    GPIO_PORTA_DATA_R &= ~(1<<7);           //Turn off EN
    delay_us(1);
    GPIO_PORTB_DATA_R = 0x0;                //Clear Data
    delay_us(1);
}

void lcd_write_cmd(unsigned char cmd)
{
    GPIO_PORTA_DATA_R &= ~(1<<6);           //Turn off RS to write to IR
    delay_us(1);
    GPIO_PORTA_DATA_R |= (1<<7);            //Turn on EN
    delay_us(1);
    GPIO_PORTB_DATA_R = cmd;                //Set Data
    delay_us(1);
    GPIO_PORTA_DATA_R &= ~(1<<7);           //Turn off EN
    delay_us(1);
    GPIO_PORTB_DATA_R = 0x0;                //Clear Data
    delay_us(1);
}

void lcd_print(unsigned char *string, unsigned char length)
{
    while(*string){
        delay_us(500);
        lcd_write_data(*(string++));        //while pointer to next char is not NULL, visit next char
    }
}

void lcd_init()
{
    lcd_write_cmd(0x01);                    //Clear Screen
    delay_us(1000);
    lcd_write_cmd(0x02);                    //Return Home
    delay_us(1000);
    lcd_write_cmd(0x06);                    //Increment with Shift off
    delay_us(1000);
    lcd_write_cmd(0x0C);                    //Display on, Cursor and Blink off
    delay_us(1000);
    lcd_write_cmd(0x38);                    //8-bit mode
    delay_us(1000);
    lcd_write_cmd(0x80);                    //First Bit
    delay_us(10000);
}

void delay_us(int n)
{                                               //MS delay
    int i, j;
    for(i = 0; i < n; i++){
        for(j = 0; j < 16; j++){
            continue;
        }
    }
}

char keypad_input()
{
    unsigned char symbol[4][4] = {
        {'1','2','3','A'},
        {'4','5','6','B'},
        {'7','8','9','C'},
        {'*','0','#','D'}
    };

    int i = 0;
    int j = 0;

    //E1-4 is row 1-4       output to pad
    //C4-7 is column 1-4    input from pad
    for(i = 0; i < 4; i++)
    {
        GPIO_PORTE_DATA_R = (1 << i+1);
        for (j = 0; j < 4; j++)
        {
            if(GPIO_PORTC_DATA_R & (1 << j+4))
            {
                return symbol[i][j];
            }
        }
        GPIO_PORTE_DATA_R = 0x0;
    }
    return 0x00;
}

int string_to_int(const char *str)
{                                                        //converts string to int
    int result = 0;
    while (*str != '\0') {
        result = result * 10 + (*str - '0');
        str++;
    }
    return result;
}

int multiply_arr(const char *arr1, const char *arr2)
{                                                       //multiplies two character arrays
    int num1 = string_to_int(arr1);
    int num2 = string_to_int(arr2);

    return num1 * num2;

}

void print(char* string)
{
    while(*string){
        delay_us(400);
        sendChar(*(string++));               //while pointer to next char is not NULL, visit next char
    }
}
