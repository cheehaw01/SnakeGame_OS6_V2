//HEADER FILES
#include "mbed.h"
#include "C12832.h"
#include "MMA7660.h"
#include <time.h>
#include <stdlib.h>

// Define segments for 7 segment display.
#define SEG_A   0x80
#define SEG_B   0x40
#define SEG_C   0x20
#define SEG_D   0x10
#define SEG_E   0x08
#define SEG_F   0x04
#define SEG_G   0x02
#define SEG_DP  0x01


//IMPORTANT INITIALIZATIONS
//Serial pc(USBTX, USBRX); deprecated
C12832 lcd(p5, p7, p6, p8, p11);
BusIn joy(p15,p12,p13,p16);
DigitalIn fire(p14);
BusOut leds(LED1,LED2,LED3,LED4);
MMA7660 accel(p28, p27); //I2C Accelerometer
DigitalOut connectionLed(LED1);//Accel OK LED
DigitalOut testLED(p9);
// ADDED (for threading)
Thread thread;
Thread threadBGM;
Thread thread7SegDisplay;
// ADDED (for buzzer)
PwmOut buzzer(p26);
// ADDED (for 7 segments display)
//DigitalOut ssel(p21);
//DigitalOut mosi(p25);
//DigitalOut sclk(p24);
//DigitalOut seg1(p23);
//DigitalOut seg2(p22);
DigitalOut ssel(p21);
DigitalOut mosi(p17);
DigitalOut sclk(p18);
DigitalOut seg1(p29);
DigitalOut seg2(p22);

DigitalOut redPin(p23);
DigitalOut greenPin(p24);
DigitalOut bluePin(p25);

// used pins: 
// p5, p7, p6, p8, p11,     -- LCD
// p12, p13, p14, p15, p16, -- joystick
// p21, p22, p23, p24, p25, -- 7 segments
// p26,                     -- buzzer
// p27, p28,                -- accelerometer

// ADDED (for 7 segments display) - Letter or Number from Combination of segments.
const uint8_t segments[16] = {
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F        ,  //0 = A,B,C,D,E,F
            SEG_B | SEG_C                                ,  //1 = B,C
    SEG_A | SEG_B |         SEG_D | SEG_E |         SEG_G,  //2 = A,B,D,E,G
    SEG_A | SEG_B | SEG_C | SEG_D |                 SEG_G,  //3 = A,B,C,D,G
            SEG_B | SEG_C |                 SEG_F | SEG_G,  //4 = B,C,F,G
    SEG_A |         SEG_C | SEG_D |         SEG_F | SEG_G,  //5 = A,C,D,F,G
    SEG_A |         SEG_C | SEG_D | SEG_E | SEG_F | SEG_G,  //6 = A,C,D,E,F,G
    SEG_A | SEG_B | SEG_C                                ,  //7 = A,B,C
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G,  //8 = A,B,C,D,E,F,G
    SEG_A | SEG_B | SEG_C | SEG_D |         SEG_F | SEG_G,  //9 = A,B,C,D,F,G
    SEG_A | SEG_B | SEG_C |         SEG_E | SEG_F | SEG_G,  //A = A,B,C,E,F,G
                    SEG_C | SEG_D | SEG_E | SEG_F | SEG_G,  //B = C,D,E,F,G
                            SEG_D | SEG_E |         SEG_G,  //C = D,E,G
            SEG_B | SEG_C | SEG_D | SEG_E |         SEG_G,  //D = B,C,D,E,G
    SEG_A |                 SEG_D | SEG_E | SEG_F | SEG_G,  //E = A,D,E,F,G
    SEG_A |                         SEG_E | SEG_F | SEG_G}; //F = A,E,F,G

// functions declaration
bool canGrow(int bodyLength, int bodyLimit);
void grow(int bodyX[], int bodyY[], int bodyLength, int i, int j);
void updateBody(int bodyX[],  int bodyY[], int bodyLength);
void buzzerBGM();
void updateShiftReg(uint8_t segments);
void segmentRefresh();
void setRGBColor(int red, int green, int blue);
void blink();
// global variable
int score = 0;

int main()
{
    //IMPORTANT VARIABLE DECLARATIONS
    srand(time(NULL));
    int r = rand();
    int i=0;
    int dimX = 125;
    int dimY = 25;
    int x=0,y=0;
    lcd.cls();
    lcd.locate(0,3);
    lcd.printf("Welcome to Snake game");
    ThisThread::sleep_for(2s);
    lcd.cls();
    lcd.locate(0, 3);
    lcd.printf("push joystick to change");
    lcd.locate(0, 12);
    lcd.printf("snake direction");
    ThisThread::sleep_for(1500ms); 
    lcd.cls();
    lcd.locate(0, 3);
    lcd.printf("hit joystick 4 fireball");
    ThisThread::sleep_for(1500ms); 
    lcd.cls();
    lcd.locate(62, 16);
    lcd.printf("3");
    ThisThread::sleep_for(1s); 
    lcd.cls();
    lcd.locate(62, 16);
    lcd.printf("2");
    ThisThread::sleep_for(1s); 
    lcd.cls();
    lcd.locate(62, 16);
    lcd.printf("1");
    ThisThread::sleep_for(1s); 
    setRGBColor(1, 1, 0);       // set RGB led to blue color
    int j = 0;
    int cst = 5;
    int fDimX = dimX - cst;
    int fDimY = dimY - cst;
    int flag = 0;
    int p1 = r % fDimX;
    int p2 = r % fDimY;
    //int score = 0;
    int bonus = 0;

    // ADDED - variables
    char direction;             // snake moving direction
    int bodyLimit = 30;         // maximum body length
    int bodyPosX[bodyLimit];    // body position X-axis
    int bodyPosY[bodyLimit];    // body position Y-axis
    int bodyLength = 1;         // current body length
    int prevI;                  // previous head position X-axis
    int prevJ;                  // previous head position Y-axis
    char bodyPattern[] = "O";   // snake body pattern

    // ADDED - start threads
    thread.start(blink);            // blinkLED
    //threadBGM.start(buzzerBGM);   // uncomment to play BGM
    thread7SegDisplay.start(segmentRefresh);

    if (p1 < 10)
        p1 = 10;
    if (p2 < 10)
        p2 = 10;
    while(1) {
        prevI = i;
        prevJ = j;

        //CONDITION FOR CHECKING FOR THE FIREBALL
        if (fire && score>5) {
            leds=0xf;
            flag = 100;
            if(bonus == 0){
                score -= 2;
                bonus = 1;
            }
        } 
        // CONDITIONS TO CHECK FOR JOYSTICK INPUT
        else {
            if (flag == 0){
                leds=joy;
                // moving the snake backwards
                if (joy == 0x4 && direction != 'r'){
                    direction = 'l';
                }
                // moving the snake forward        
                else if (joy == 0x8 && direction != 'l') {
                    direction = 'r';
                }
                // moving the snake up    
                else if (joy == 0x1 && direction != 'd') {
                    direction = 'u';
                }
                // moving the snake down        
                else if (joy == 0x2 && direction != 'u') {
                    direction = 'd';
                }
            }

            // BLOCK ADDED
            // CONTINUOUSLY MOVE ON DIRECTION
            if (direction == 'l') {
                i= i - cst;
                if (i < 0)
                    i = dimX;
            }
            // moving the snake forward        
            else if (direction == 'r') {
                i = (i + cst)%dimX;
            }
            // moving the snake up    
            else if (direction == 'u') {
                j = j - cst;
                if (j < 0)
                    j = dimY-1; // MODIFIED -1
            }
            // moving the snake down        
            else if (direction == 'd') {
                j = (j + cst)%dimY;
            } 
            // BLOCK ADDED


            // LOGIC FOR THE FIREBALL         
            if (flag >= 1) {
                x = (x + accel.x() * 32.0)/1.5;
                y = (y -(accel.y() * 16.0))/1.5;
                lcd.fillcircle(x+63, y+15, 3, 1); //draw bubble
                //lcd.circle(63, 15, 8, 1);
                ThisThread::sleep_for(100ms); //time delay
                printf(" score %d", score);
                flag -=1;
                if (abs(x + 63 - p1) <=cst && abs(y + 15 - p2) <=cst+1) {
                    score +=15;
                    flag = -1;
                }
                if (flag < 0)
                    flag = 0;
                bonus = 0; 
                lcd.fillcircle(x+63, y+15, 3, 0); //erase bubble
            }            
        }

        if (flag == 0) {
            //printing the snake, food and score on the LCD
            lcd.cls();

            // BLOCK ADDED - print body
            if (bodyLength > 1) {
                updateBody(bodyPosX, bodyPosY, bodyLength);
                bodyPosX[0] = prevI;
                bodyPosY[0] = prevJ;

                for (int i = bodyLength - 2; i >= 0; i--) {
                    // pc.printf("the body is at %d %d\n \r", bodyPosX[i], bodyPosY[i]);
                    lcd.locate(bodyPosX[i], bodyPosY[i]);
                    lcd.printf(bodyPattern);
                }
            }     
            // BLOCK ADDED - print body

            lcd.locate(i,j);
            lcd.printf(bodyPattern);
            lcd.locate(0, 22);
            lcd.printf("%d", score);
            //pc.printf("the dot is at %d %d\n \r", p1, p2); 
            printf("snake location %d %d\n \r", i, j);
            lcd.locate(p1, p2);
            lcd.printf(".");

            // CONDITION FOR CHECKING THE SNAKE FOOD COLLISION
            if (abs(i - p1) <=cst && abs(j - p2) <=cst+1) { 
                //pc.printf("the snake is at %d %d\n \r", i, j);
                //pc.printf("the dot is at %d %d\n \r", p1, p2);
                blink();
                score = score + 1; 

                //finding a new random location for food
                r = rand();
                p1 = r%fDimX;
                p2 = r%fDimY;
                //boundary checking
                if (p1 < 10)
                    p1 = 10;
                if (p2 < 10)
                    p2 = 10;
                // lcd.printf(".");

                // BLOCK ADDED - check & increase body length
                bodyLength++;
                if (canGrow(bodyLength, bodyLimit)) {
                    grow(bodyPosX, bodyPosY, bodyLength, prevI, prevJ);
                }
                else {
                    bodyLength = bodyLimit + 1;
                }
                // BLOCK ADDED

            }
            ThisThread::sleep_for(300ms);
        }

    }
}

// FUNCTION ADDED
// function - check whether the snake reach maximum length
bool canGrow(int bodyLength, int bodyLimit) {
    if (bodyLength >= bodyLimit + 2) return false;
    return true;
}

// function - increase body of snake.
void grow(int bodyX[], int bodyY[], int bodyLength, int i, int j) {
    if (bodyLength == 2) {
        bodyX[bodyLength - 2] = i;
        bodyY[bodyLength - 2] = j;
    }
    else {
        bodyX[bodyLength - 1] = bodyX[bodyLength - 2];
        bodyY[bodyLength - 1] = bodyY[bodyLength - 2];
    }
}

// function - update body position of snake
void updateBody(int bodyX[],  int bodyY[], int bodyLength) {
    int tempX, tempY;
    for (int i = 1; i <= bodyLength - 2; i++) {
        tempX = bodyX[i];
        tempY = bodyY[i];
        bodyX[i] = bodyX[0];
        bodyY[i] = bodyY[0];
        bodyX[0] = tempX;
        bodyY[0] = tempY;
    }
}

// function - for output background music
void buzzerBGM() {
    float frequency[] = {659, 554, 659, 554, 440, 494, 554, 587, 494, 659, 554, 440};
    float beat[] = {1,1,1,1,1,0.5,0.5,1,1,1,1,2};
    while (true) {
        for (int i=0; i<=22; i++) {
		    buzzer.period(1/(2*frequency[i]));
		    buzzer=0.5;
		    ThisThread::sleep_for(0.4*beat[i]*1000);
		}
        ThisThread::sleep_for(1s);
    }
}

// function - update shift register
void updateShiftReg(uint8_t segments)
{
    uint8_t bitCnt;

    //Pull SCK and MOSI low, pull SSEL low
    ssel = 0;
    mosi = 0;
    sclk = 0;
    
    //wait 1us
    wait_us(1);
    
    //Loop through all eight bits
    for (bitCnt = 0; bitCnt < 8; bitCnt++)
    {
        //output MOSI value (depends on bit 7 of "segments")
        if (segments & 0x80) {
            mosi = 0;
        } else {
            mosi = 1;
        }
        //wait 1us
        wait_us(1);
        //pull SCK high
        sclk = 1;
        //wait 1us
        wait_us(1);
        //pull SCK low
        sclk = 0;
        
        //shift "segments"
        segments = segments << 1;
    }
    //Pull SSEL high
    ssel = 1;
}

// function - refresh the 7 segments display so different value can be shown at the same time.
void segmentRefresh() {
    bool seg1Switch = true;
    bool seg2Switch = false;
    while (true) {
        seg1Switch = !seg1Switch;
        seg2Switch = !seg2Switch;
        seg1 = seg1Switch;
        seg2 = seg2Switch;
        if (seg1Switch == true)
            updateShiftReg(~segments[(score%100)/10]);
        if (seg2Switch == true)
            updateShiftReg(~segments[score%10]);
        ThisThread::sleep_for(1ms);
    }
}

// function - set RGB LED color
void setRGBColor(int red, int green, int blue) {
    redPin = red;
    greenPin = green;
    bluePin = blue;
}

void blink() {
    while(true) {
        setRGBColor(0, 0, 1);   // set rgb to yellow
        ThisThread::sleep_for(500ms);
        setRGBColor(1, 1, 1);
        ThisThread::sleep_for(500ms);
        setRGBColor(1, 1, 0);   
    }
}
// FUNCTION ADDED