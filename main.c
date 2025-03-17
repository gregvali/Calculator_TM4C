
/**
 * main.c
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
int multiply_arr(const char *arr1, const char *arr2);
void print(char* string);

enum State                  //State Enumerations
{
    INITIAL,
    START,
    STATEA,
    STATEB,
    DISPLAY
};

int main(void)
{
    EnableGPIO();
    EnableUART();
    lcd_init();
    unsigned char c = '\0';
    char astr[16];
    char bstr[16];
    unsigned char str[16];
    int result;
    int i;
    int length;

    enum State myState = INITIAL;

    while(1) {
        if(myState == INITIAL){
            sendChar('i');              //Used UART to debug
            sendChar('\n');
            lcd_write_cmd(0x01);        //Clear screen
            delay_us(1600);                //1.6ms delay after clearing
            if(c == '#'){
                myState = DISPLAY;
            }
            else if(c == 'C') {
                myState = INITIAL;
            }
            else {
                myState = START;
            }
        }
        else if(myState == START){
            sendChar('s');
            sendChar('\n');
            result = 0;
            length = 0;
            lcd_write_cmd(0x02);        //Return home
            for(i = 0; i < 16; i++){    //RESET ARRAYS
                lcd_write_data(' ');    //Clear top row
                astr[i] = '\0';
                bstr[i] = '\0';
                str[i] = '\0';
                delay_us(500);
            }
            lcd_write_cmd(0x02);
            i = 0;
            if(c == '#'){
                myState = DISPLAY;
            }
            else if(c == 'C') {
                myState = INITIAL;
            }
            else {
                myState = STATEA;
            }
        }
        else if(myState == STATEA){
            sendChar('a');
            sendChar('\n');
            if(c == '#'){
                myState = DISPLAY;
            }
            else if(c == 'C') {
                myState = INITIAL;
            }
            else if(c == '*' || length == 8){      //If a is too long, go to B
                myState = STATEB;
                length = 0;
                lcd_write_cmd(0x01);                //Clear and return home
                delay_us(1600);
                lcd_write_cmd(0x02);
                delay_us(1600);
            }
            else{
                if(c!=0x00 && c!= 'A' && c!= 'B' && c!= 'D') {
                    lcd_write_data(c);              //Write digit and save it to array
                    astr[length] = c;
                    sendChar(c);
                    length = length + 1;            //Increment length
                }
            }
        }
        else if(myState == STATEB){
            sendChar('b');
            sendChar('\n');
            if(c == '#'){
                myState = DISPLAY;
            }
            else if(c == 'C') {
                myState = INITIAL;
            }
            else if(length == 8){
                myState = DISPLAY;
                length = 0;
                lcd_write_cmd(0x01);
                delay_us(1600);
            }
            else{
                if(c!=0x00 && c!= 'A' && c!= 'B' && c!= 'D' && c!='*') {
                    lcd_write_data(c);              //Write data and increment length
                    bstr[length] = c;
                    sendChar(c);
                    length = length + 1;
                }
            }
        }
        else if(myState == DISPLAY){
            sendChar('d');
            sendChar('\n');
            if(c == '#'){
                myState = DISPLAY;
            }
            else if(c == 'C') {
                myState = INITIAL;
            }
            else {
                lcd_write_cmd(0x01);
                delay_us(1600);
                lcd_write_cmd(0xCF);                    //Move to bottom right

                result = multiply_arr(astr, bstr);      //Multiply strings
                while (result > 0 && i < 16) {
                    str[i++] = (result % 10) + '0';     //Store the digit as a character
                    result /= 10;
                }
                lcd_write_cmd(0x04);
                lcd_print(str,i);                   //Print string with decrement
                print(str);
                lcd_write_cmd(0x06);                    //reset to increment
                delay_us(250000);
                lcd_write_cmd(0x01);
                delay_us(1600);
                myState = START;
            }
        }
        else{
            lcd_print("how?", 4);
        }
        c = keypad_input();

        delay_us(20000);
    }
}
