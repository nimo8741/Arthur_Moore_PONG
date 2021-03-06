#include <delays.h>
#include <string.h>
#include "GLCDroutinesEasyPic.h"
#include "Pong_funcs.h"
#include <stdio.h>
#include <stdlib.h>  // these last two are so that we can make random numbers
#include <math.h>
#include <p18f87k22.h>
#include "adc.h"

#define deg_to_rad 0.01745
#define max_y 62
#define max_x 126
#define min_y 1
#define min_x 1
#define scale 2
#define ball_low_byte 0x00
#define ball_high_byte 0xD0    // this is the value to change the speed of the ball.  To speed it up, make this number bigger
#define paddle_min 24 
#define paddle_max 39
#define paddle_tmr_high 0xC8   // this is the value to change the speed of the ball.  To speed it up, make this number bigger 
#define paddle_tmr_low 0x00
#define text 28    // starting position for the BEGIN? text

struct ball_struct ball;
unsigned char player1_points = 0;
unsigned char player2_points = 0;
unsigned char in_box1;
unsigned char in_box2;
unsigned char paddle1min = paddle_min;   // This is the bottom value for player 1's paddle
unsigned char paddle1max = paddle_max;   // This is the top value for player 1's paddle
unsigned char paddle2min = paddle_min;
unsigned char paddle2max = paddle_max;   // Keep in mind these are not constants, just the starting positions
unsigned char paddle1 = 0;   // this means that player 1 has pressed neither button 
unsigned char paddle2 = 0;   // this means that player 2 has pressed neither button
float ADCResult;
unsigned char game;

//////////////////////////////////////////
// Function: Initial_screen
// Inputs: none
// Outputs: none
// What does it do?
// This function initializes the GLCD screen
// with the BEGIN? message and starts the 
// timer0 which will give the random number
// so that the beginning of the game is
// randomized
//////////////////////////////////////////


void Initial_screen(){
    
    unsigned char j;
    unsigned char i;
    in_box1 = 0;
    in_box2 = 0;
    
    game = 0;
    begin_game();            // this asks the begin game question and generates the random number
    //end_game();
    TMR0H = 0xFF;
    TMR0L = 0xFE;    // load the values in to the register to make this take 1 second
    T0CONbits.TMR0ON = 1; // turn timer0 on
    while (~ PORTEbits.RE3) {
        if (INTCONbits.TMR0IF){ // if the timer has rolled over
            TMR0H = 0xFF;       // load numbers for 1 second
            TMR0L = 0x00;
            INTCONbits.TMR0IF = 0;      //Clear flag
        }
        
        // wait until player presses start
    }
    
    // clear board
    ClearGLCD();
}

//////////////////////////////////////////
// Function: Initial_ball
// Inputs: none
// Outputs: none
// What does it do?
// This function uses the timer registers to
// give the ball its starting trajectory as 
// well as position.  Then it prints the
// starting position for the paddles
//////////////////////////////////////////

void Initial_ball() {
    char delta;
    unsigned char rand1, rand2;
    float angle;
    rand2 = TMR0L % 2;  // this uses the timer registers as a pseudo random number generator
    rand1 = TMR0L % 3;
    delta = TMR0L/5;
    if (rand1)
        angle = 180;
    else
        angle = 0;
    if (rand2)
        delta *= -1;
    angle = angle + delta;  // add in this deflection from straight at the 
    ball.x_pos = 64;
    ball.y_pos = 32;
    ball.theta = angle;
    //ball.theta = 0;
    ball.done_waiting = 0;    // this means that the ball is not done waiting
    
    // now reset the position of the paddles
    paddle1min = paddle_min;
    paddle1max = paddle_max;
    paddle2min = paddle_min;
    paddle2max = paddle_max;
    
    // print the paddles to the screen
    print_paddle(1);
    print_paddle(2);
    
}

//////////////////////////////////////////
// Function: update_ball
// Inputs: none
// Outputs: none
// What does it do?
/* This function first deleted the instance
 * of the ball which is on screen.  Then it
 * increments the position of the ball.  If 
 * the ball is at the upper or lower wall, it
 * changes the direction of travel to account
 * for the reflect.  If it is in the same location
 * as one of the paddles, it uses the reflection 
 * scheme laid out in the report.
*/
//////////////////////////////////////////
void update_ball(void){
    unsigned char pixels;
    unsigned char hit_paddle;
    unsigned char temp;
    game = 1;

    // first clear the old pixels
    if (ball.done_waiting){
        unsigned char cur_x, cur_y;

        cur_x = (unsigned char) ball.x_pos;
        cur_y = (unsigned char) ball.y_pos;
        // take care of if the ball has entered the region for the scoreboard
        if (cur_y == 11 || cur_y == 10){
            if (cur_x < 22)
                in_box1 = 1;
            else if(cur_x > 105)
                in_box2 = 1;
        }
        else{
            if (in_box1 == 1 && (cur_y > 13 || cur_x > 22)){  // it just left the box
                in_box1 = 0;
                score_board(1);  // need to reprint the scoreboard because the ball has left the box
            }
            if (in_box2 == 1 && (cur_y > 13 || cur_x < 105)){
                in_box2 = 0;
                score_board(2);
            }
        }
        
        // Delete the pixels from last time
        if ( cur_x == min_x && cur_y >= paddle1min && cur_y <= paddle1max){ // this hit player 1's paddle
            hit_paddle = 1;
        }
        else if (cur_x == max_x && cur_y >= paddle2min && cur_y <= paddle2max){  // this hit player 2's paddle
            hit_paddle = 2;
        }
        else{
            hit_paddle = 0;
        }
        
       pixels = 1 << (cur_y % 8);
        if (!(cur_y % 8)){  // the top pixel is on the previous page
            
            // bottom three pixels
            SetCursor(cur_x, (cur_y / 8) - 1);
            WriteData(0);
            
            // remaining six pixels
            SetCursor(cur_x, cur_y / 8);
            WriteData(0);
            
            
            if (hit_paddle != 1){
                SetCursor(cur_x - 1, (cur_y / 8) - 1);
                WriteData(0);
                
                SetCursor(cur_x - 1, cur_y / 8);
                WriteData(0);
            }
            if (hit_paddle != 2){
                SetCursor(cur_x + 1, (cur_y / 8) - 1);
                WriteData(0);
                
                SetCursor(cur_x + 1, cur_y / 8);
                WriteData(0);
            }
            
        }
        else if (cur_y % 8 == 7){   // the bottom pixel is on the next page
            // bottom six pixels
            SetCursor(cur_x - 1, cur_y / 8);
            WriteData(0);
            SetCursor(cur_x, cur_y / 8);
            WriteData(0);
            SetCursor(cur_x + 1, cur_y / 8);
            WriteData(0);   
            
            // Now the top 3 pixels
            SetCursor(cur_x - 1, (cur_y / 8) + 1);
            WriteData(0);
            SetCursor(cur_x, (cur_y / 8) + 1);
            WriteData(0);
            SetCursor(cur_x + 1, (cur_y / 8) + 1);
            WriteData(0);
            
        }
        else{   // the base case where both pixels are on the same page
            SetCursor(cur_x - 1, cur_y / 8);
            WriteData(0);
            SetCursor(cur_x, cur_y / 8);
            WriteData(0);
            SetCursor(cur_x + 1, cur_y / 8);
            WriteData(0);
        }
        PORTEbits.RE0 = ~PORTEbits.RE0;

       // Now the hit detection on angle updating
       
        if (cur_y == min_y || cur_y == max_y) { // This is on the top or bottom of the screen
            ball.theta *= -1;
        }
        else if ( hit_paddle == 1){ // this hit player 1's paddle
            paddle_reflect(1,cur_y - paddle1min);
            // we need to get the ball out of the way before we updates the paddle
            // otherwise the second row of pixels might disappear
            ball.x_pos += scale * cos(ball.theta * deg_to_rad);
            ball.y_pos += scale * sin(ball.theta * deg_to_rad);
            if (ball.x_pos > max_x)
                ball.x_pos = max_x;
            if (ball.y_pos > 62)
                ball.y_pos = 62;
            if (ball.x_pos < 1)
                ball.x_pos = 1;
            if (ball.y_pos < 1)
                ball.y_pos = 1;
            cur_x = (unsigned char) ball.x_pos;
            cur_y = (unsigned char) ball.y_pos;
            print_paddle(1);
        }
        else if (hit_paddle == 2){  // this hit player 2's paddle
            paddle_reflect(2,cur_y - paddle2min);
            // we need to get the ball out of the way before we updates the paddle
            // otherwise the second row of pixels might disappear
            ball.x_pos += scale * cos(ball.theta * deg_to_rad);
            ball.y_pos += scale * sin(ball.theta * deg_to_rad);
            if (ball.x_pos > max_x)
                ball.x_pos = max_x;
            if (ball.y_pos > 62)
                ball.y_pos = 62;
            if (ball.x_pos < 1)
                ball.x_pos = 1;
            if (ball.y_pos < 1)
                ball.y_pos = 1;
            cur_x = (unsigned char) ball.x_pos;
            cur_y = (unsigned char) ball.y_pos;
            print_paddle(2);
   
        }
        else if (cur_x == min_x) { // this is a goal for player 2
            goal_scored(1);
            score_board(1);
            return;
        } 
        else if (cur_x == max_x){
            goal_scored(0); // goal scored for player 1
            score_board(2);
            return;
        }
        // now update the position of the ball
        ball.x_pos += scale * cos(ball.theta * deg_to_rad);
        ball.y_pos += scale * sin(ball.theta * deg_to_rad);

        if (ball.x_pos > max_x)
            ball.x_pos = max_x;
        if (ball.y_pos > 62)
            ball.y_pos = 62;
        if (ball.x_pos < 1)
            ball.x_pos = 1;
        if (ball.y_pos < 1)
            ball.y_pos = 1;

        cur_x = (unsigned char) ball.x_pos;
        cur_y = (unsigned char) ball.y_pos;

        // Now go through the process of printing the ball.  It is 
        // the same as when we deleted the ball just with a different value to send.
        // now find the three vertical pixels to light up
        pixels = 1 << (cur_y % 8);
        if (!(cur_y % 8)){  // the top pixel is on the previous page
            
            // bottom three pixels
            SetCursor(cur_x - 1, (cur_y / 8) - 1);
            WriteData(0x80);
            SetCursor(cur_x, (cur_y / 8) - 1);
            WriteData(0x80);
            SetCursor(cur_x + 1, (cur_y / 8) - 1);
            WriteData(0x80);
            
            // remaining six pixels
            pixels = pixels | (pixels << 1);
            SetCursor(cur_x - 1, cur_y / 8);
            WriteData(pixels);
            SetCursor(cur_x, cur_y / 8);
            WriteData(pixels);
            SetCursor(cur_x + 1, cur_y / 8);
            WriteData(pixels);        
            
        }
        else if (cur_y % 8 == 7){   // the bottom pixel is on the next page
            // bottom six pixels
            pixels = pixels | (pixels >> 1);
            SetCursor(cur_x - 1, cur_y / 8);
            WriteData(pixels);
            SetCursor(cur_x, cur_y / 8);
            WriteData(pixels);
            SetCursor(cur_x + 1, cur_y / 8);
            WriteData(pixels);   
            
            // Now the top 3 pixels
            SetCursor(cur_x - 1, (cur_y / 8) + 1);
            WriteData(0x01);
            SetCursor(cur_x, (cur_y / 8) + 1);
            WriteData(0x01);
            SetCursor(cur_x + 1, (cur_y / 8) + 1);
            WriteData(0x01);
        }
        else{   // the base case where both pixels are on the same page
            pixels = pixels | (pixels >> 1);
            pixels = pixels | (pixels << 1);
            SetCursor(cur_x - 1, cur_y / 8);
            WriteData(pixels);
            SetCursor(cur_x, cur_y / 8);
            WriteData(pixels);
            SetCursor(cur_x + 1, cur_y / 8);
            WriteData(pixels);
        }
        
        ball.done_waiting = 0;   // the ball now needs to go through another waiting cycle
        T0CONbits.TMR0ON = 1;  // turn on the timer because now we wait
    }
}

//////////////////////////////////////////
// Function: goal_scored
// Inputs: unsigned char player
// Outputs: none
// What does it do?
/* This function increments the score date for
 * when a goal has been scored.  It also clear the
 * screen before reprinting everything
*/
//////////////////////////////////////////
void goal_scored(unsigned char player){
    if (!player)
        player1_points++;
    else if (player)
        player2_points++;
    
    total_paddle_clear();
    
    if (!player)
        player = 1;
    else
        player = 2;
    // now reset the ball 
    ClearGLCD();
    score_board(player);
    Initial_ball();
}

//////////////////////////////////////////
// Function: score_board())
// Inputs: unsigned char player
// Outputs: none
// What does it do?
/* This function function goes through the process of
 * actually changing what the scoreboard displays
*/
//////////////////////////////////////////

void score_board(unsigned char player){
    unsigned char i;
    unsigned char score_1_location;
    unsigned char score_2_location;
    // Score for player 1
    if (player == 1){
        for (i = 1 ; i <= player1_points ; i++ ){
            score_1_location = i * 2 + 5;
            Delay10KTCYx(2); // for some reason it gets weird if I don't wait this long
            SetCursor(score_1_location,0);
            WriteData(0xFE);
        }
    }
    
    // Score for player 2
    if (player == 2){
        for (i = 1 ; i <= player2_points ; i++ ){
            score_2_location = 127 - i * 2 - 5;
            Delay10KTCYx(2); // for some reason it gets weird if I don't wait this long
            SetCursor(score_2_location,0);
            WriteData(0xFE);
        }
    }
}

//////////////////////////////////////////
// Function: update_paddles())
// Inputs: none
// Outputs: none
// What does it do?
/* This function checks to see if the player
 * wants the paddles to move turns on the waiting
 * timer if they do.  It also sets a value accordingly
 * which describes if the paddle should move up
 * or down.
*/
//////////////////////////////////////////
void update_paddles(void) {
    // first check if the buttons for player 1 have been pressed 
    if (!paddle1) {
        if (PORTEbits.RE4 && !PORTEbits.RE5){ // this means that player 1 wants to move down 
            paddle1 = 1;
            T3CONbits.TMR3ON = 1;
            Delay10KTCYx(2);

        }
        else if (PORTEbits.RE5 && !PORTEbits.RE4){ // this means that player 1 wants to move up
            paddle1 = 2;
            T3CONbits.TMR3ON = 1;
            Delay10KTCYx(2);

        }
    }
    if (!paddle2){
        if (PORTJbits.RJ0 && !PORTJbits.RJ1){  // paddle should move down
            paddle2 = 1;
            T5CONbits.TMR5ON = 1;
            Delay10KTCYx(2);
        }
        else if (PORTJbits.RJ1 && !PORTJbits.RJ0){  // paddle should move up
            paddle2 = 2;
            T5CONbits.TMR5ON = 1;
            Delay10KTCYx(2);
        }
    }
}

//////////////////////////////////////////
// Function: TMR1handler()
// Inputs: none
// Outputs: none
// What does it do?
/* This function is part of a low priority 
 * ISR and mostly just sets the value to 
 * say that the ball is done waiting
*/
//////////////////////////////////////////
void TMR1handler() {
    ball.done_waiting = 1;  // this means that the ball has completed its waiting cycle
    TMR1H = ball_high_byte;
    TMR1L = ball_low_byte; 
    // toggle a led 
    PIR1bits.TMR1IF = 0;      //Clear flag and return to polling routine
    // turn off the timer
    T0CONbits.TMR0ON = 0;
}

//////////////////////////////////////////
// Function: TMR3handler()
// Inputs: none
// Outputs: none
// What does it do?
/* This function is part of a low priority 
 * ISR and mostly just sets the value to 
 * say that player 1's paddle is done waiting
*/
//////////////////////////////////////////
void TMR3handler(){
    // stop the timer
    T3CONbits.TMR3ON = 0;
    // move the paddle
    if (paddle1 == 1){
        if (paddle1max < max_y){
            clear_paddle(1);
            paddle1min++;
            paddle1max++;
        }
    }
    else if(paddle1 == 2){
        if (paddle1min > 0){
            clear_paddle(1);
            paddle1min--;
            paddle1max--;
        }
    }
    // reload the register values
    TMR3H = paddle_tmr_high;
    TMR3L = paddle_tmr_low;
    // clear paddle
    paddle1 = 0;
    print_paddle(1);
    // clear the interrupt flag
    PIR2bits.TMR3IF = 0;
}

//////////////////////////////////////////
// Function: TMR5handler()
// Inputs: none
// Outputs: none
// What does it do?
/* This function is part of a low priority 
 * ISR and mostly just sets the value to 
 * say that player 2's paddle is done waiting
*/
//////////////////////////////////////////
void TMR5handler(){
    // stop the timer
    T5CONbits.TMR5ON = 0;
    // move the paddle
    if (paddle2 == 1){
        if (paddle2max < max_y){
            clear_paddle(2);
            paddle2min++;
            paddle2max++;
        }
    }
    else if(paddle2 == 2){
        if (paddle2min > 0){
            clear_paddle(2);
            paddle2min--;
            paddle2max--;
        }
    }
    // reload the register values
    TMR5H = paddle_tmr_high;
    TMR5L = paddle_tmr_low;
    // clear paddle
    paddle2 = 0;
    print_paddle(2);
    // clear the interrupt flag
    PIR5bits.TMR5IF = 0;
}

//////////////////////////////////////////
// Function: print_paddle
// Inputs: unsigned char player
// Outputs: none
// What does it do?
/* This function goes through an turns on
 * 16 pixels which define the current position
 * of the paddle for the specified player
*/
//////////////////////////////////////////
void print_paddle(unsigned char player){
    unsigned char input;
    unsigned char cursor;
    if (player == 1){ // this mean I need to reprint the paddle for player 1
        
        // I will print the lower page first
        cursor = paddle1min / 8;
        input = 0xFF << (paddle1min % 8);
        
        SetCursor(0, cursor);
        WriteData(input);
        // and do a second line to make the paddle a little thicker
        SetCursor(1,cursor);
        WriteData(input);
        
        // now print the upper page
        cursor = paddle1max / 8;
        input = 0xFF >> (7 - (paddle1max % 8));
        SetCursor(0, cursor);
        WriteData(input);
        SetCursor(1, cursor);
        WriteData(input);
        
        // now I will print the possible middle page
        if ((cursor - (paddle1min / 8)) == 2){
            cursor--;
            SetCursor(0,cursor);
            WriteData(0xFF);   // the middle page is guaranteed to be full
            SetCursor(1,cursor);
            WriteData(0xFF);
        }
        
    }
    else if(player == 2){   // this means I need to reprint the paddle for player 2
        cursor = paddle2min / 8;
        input = 0xFF << (paddle2min % 8);
        
        SetCursor(127, cursor);
        WriteData(input);
        // and do a second line to make the paddle a little thicker
        SetCursor(126,cursor);
        WriteData(input);
        
        // now print the upper page
        cursor = paddle2max / 8;
        input = 0xFF >> (7 - (paddle2max % 8));
        SetCursor(127, cursor);
        WriteData(input);
        SetCursor(126, cursor);
        WriteData(input);
        
        // now I will print the possible middle page
        if ((cursor - (paddle2min / 8)) == 2){
            cursor--;
            SetCursor(127,cursor);
            WriteData(0xFF);   // the middle page is guaranteed to be full
            SetCursor(126,cursor);
            WriteData(0xFF);
        }
    }
}

//////////////////////////////////////////
// Function: print_reflect
// Inputs: unsigned char player, unsigned char pos
// Outputs: none
// What does it do?
/* This function computes the new ball direction
 * of travel based on the pixels which it hits
 * on the paddle.  In short, every pixel is another
 * 10 degrees
*/
//////////////////////////////////////////
void paddle_reflect(unsigned char player, unsigned char pos){
    char answer;
    // first figure out the new angle
    if ( pos < 8 ){  // top half of the paddle
        answer = -10*(7-pos);
        ball.theta = (float) answer;
    }
    else if (pos > 7){  // bottom half of the paddle
        answer = 10*(pos - 8);
        ball.theta = (float) answer;
    }
    
    if (player == 2){
        if (ball.theta < 0)
            ball.theta = -180 - ball.theta;
        else 
            ball.theta = 180 - ball.theta;
    }
}

//////////////////////////////////////////
// Function: clear_paddle
// Inputs: unsigned char player
// Outputs: none
// What does it do?
/* This function clears the screen at the
 * current position of the paddle
*/
//////////////////////////////////////////
void clear_paddle(unsigned char player){
    if (player == 1){
        SetCursor(0, paddle1min / 8);
        WriteData(0);
        SetCursor(0, paddle1max / 8);
        WriteData(0);
        SetCursor(1, paddle1min / 8);
        WriteData(0);
        SetCursor(1, paddle1max / 8);
        WriteData(0);
        SetCursor(2, paddle1min / 8);
        WriteData(0);
        SetCursor(2, paddle1max / 8);  // need to clear extra columns for player 1 to get rid of the weird pixels
        WriteData(0);
        SetCursor(3, paddle1min / 8);
        WriteData(0);
        SetCursor(3, paddle1max / 8);
        WriteData(0);
    }
    else if (player == 2){
        SetCursor(126, paddle2min / 8);
        WriteData(0);
        SetCursor(126, paddle2max / 8);
        WriteData(0);
        SetCursor(127, paddle2min / 8);
        WriteData(0);
        SetCursor(127, paddle2max / 8);
        WriteData(0);
    }
}

//////////////////////////////////////////
// Function: total_paddle_clear
// Inputs: none
// Outputs: none
// What does it do?
/* This function clears the entire screen in
 * the first two columns as well as the last
 * two columns
*/
//////////////////////////////////////////
void total_paddle_clear(void){
    unsigned char i;
    for (i = 0; i < 8; i++){
        SetCursor(0, i);
        WriteData(0);
        SetCursor(1,i);
        WriteData(0);
        SetCursor(127,i);
        WriteData(0);
        SetCursor(126,i);
        WriteData(0);
    }
}

//////////////////////////////////////////
// Function: check_reset
// Inputs: none
// Outputs: none
// What does it do?
/* This function checks if either player has
 * gotten to 5 points or if the reset button
 * has been pressed.  Either way, it initiates
 * the end game sequence
*/
//////////////////////////////////////////
void check_reset(void){
    if (player1_points == 5 || player2_points == 5){  // if the game has ended by score
        game = 0;
        end_game();
    }
    
    
    if (PORTEbits.RE3){  // if the reset button has been pressed
        Delay10KTCYx(4);
        if (PORTEbits.RE3){
            Delay10KTCYx(36000);
            Delay10KTCYx(36000);
            if (game == 1){
                game = 0;
                end_game ();
            }
        }
    }
}

//////////////////////////////////////////
// Function: end_game
// Inputs: none
// Outputs: none
// What does it do?
/* This function displays which player won
 * as well as begins the next game so that
 * it can begin anew.
*/
//////////////////////////////////////////
void end_game(void){
    
    unsigned char i;
    unsigned char j;
    
    if (game == 0){
    
        score_board(1);
        score_board(2);

        /////////// P ///////////
        if (player1_points != player2_points){
        i = 41;
        // if there is a definitive winner, print the winning player
        SetCursor(i,3);
        WriteData(0xFF);

        SetCursor(i+1,3);
        WriteData(0x09);
        SetCursor(i+2,3);
        WriteData(0x09);
        SetCursor(i+3,3);
        WriteData(0x09);

        SetCursor(i+4,3);
        WriteData(0x0F);

        /////////// L ///////////

        i = i + 6;

        SetCursor(i,3);
        WriteData(0xFF);

        SetCursor(i+1,3);
        WriteData(0x80);
        SetCursor(i+2,3);
        WriteData(0x80);
        SetCursor(i+3,3);
        WriteData(0x80);

        /////////// A ///////////

        i = i + 5;

        SetCursor(i,3);
        WriteData(0xFF);

        SetCursor(i+1,3);
        WriteData(0x09);
        SetCursor(i+2,3);
        WriteData(0x09);
        SetCursor(i+3,3);
        WriteData(0x09);

        SetCursor(i+4,3);
        WriteData(0xFF);


        /////////// Y ///////////

        i = i + 6;

        SetCursor(i,3);
        WriteData(0x01);
        SetCursor(i+1,3);
        WriteData(0x02);
        SetCursor(i+2,3);
        WriteData(0x04);

        SetCursor(i+3,3);
        WriteData(0xF8);

        SetCursor(i+4,3);
        WriteData(0x04);
        SetCursor(i+5,3);
        WriteData(0x02);
        SetCursor(i+6,3);
        WriteData(0x01);


        /////////// E ///////////

        i = i + 8;

        SetCursor(i,3);
        WriteData(0xFF);

        SetCursor(i+1,3);
        WriteData(0x89);
        SetCursor(i+2,3);
        WriteData(0x89);
        SetCursor(i+3,3);
        WriteData(0x89);

        /////////// R ///////////

        i = i + 5;

        SetCursor(i,3);
        WriteData(0xFF);

        SetCursor(i+1,3);
        WriteData(0x19);
        SetCursor(i+2,3);
        WriteData(0x29);
        SetCursor(i+3,3);
        WriteData(0x49);
        SetCursor(i+4,3);
        WriteData(0x8F);


        /////////// 1/2 ///////////

        i = i + 7;

        if (player1_points > player2_points){
            SetCursor(i,3);
            WriteData(0x82);

            SetCursor(i+1,3);
            WriteData(0x81);

            SetCursor(i+2,3);
            WriteData(0xFF);

            //SetCursor(i+3,3);
            //WriteData(0xFF);

            SetCursor(i+3,3);
            WriteData(0x80);

            SetCursor(i+4,3);
            WriteData(0x80);
        }

        if (player2_points > player1_points){

            SetCursor(i,3);
            WriteData(0x83);

            SetCursor(i+1,3);
            WriteData(0xC1);

            SetCursor(i+2,3);
            WriteData(0xA1);

            SetCursor(i+3,3);
            WriteData(0x91);

            SetCursor(i+4,3);
            WriteData(0x8F);
        }

        /////////// W ///////////

        i = 50;

        SetCursor(i,4);
        WriteData(0xFF);

        SetCursor(i+1,4);
        WriteData(0x80);

        SetCursor(i+2,4);
        WriteData(0x60);

        SetCursor(i+3,4);
        WriteData(0x10);

        SetCursor(i+4,4);
        WriteData(0x60);

        SetCursor(i+5,4);
        WriteData(0x80);

        SetCursor(i+6,4);
        WriteData(0xFF);


        /////////// I ///////////

        i = i + 8;

        SetCursor(i,4);
        WriteData(0x81);

        SetCursor(i+1,4);
        WriteData(0xFF);

        SetCursor(i+2,4);
        WriteData(0x81);

        /////////// N ///////////

        i = i + 4;

        SetCursor(i,4);
        WriteData(0xFF);

        SetCursor(i+1,4);
        WriteData(0x03);

        SetCursor(i+2,4);
        WriteData(0x0C);

        SetCursor(i+3,4);
        WriteData(0x30);

        SetCursor(i+4,4);
        WriteData(0xC0);

        SetCursor(i+5,4);
        WriteData(0xFF);


        /////////// S ///////////

        i = i + 7;

        SetCursor(i,4);
        WriteData(0x8F);

        SetCursor(i+1,4);
        WriteData(0x89);
        SetCursor(i+2,4);
        WriteData(0x89);
        SetCursor(i+3,4);
        WriteData(0x89);

        SetCursor(i+4,4);
        WriteData(0xF9);



        /////////// ExclamationPoint ///////////

        i = i + 6;

        SetCursor(i,4);
        WriteData(0xBF);
        }
        else{
            i = 56;
            // T
            SetCursor(i,4);
            WriteData(1);
            SetCursor(i+1,4);
            WriteData(1);
            SetCursor(i+2,4);
            WriteData(0xFF);
            SetCursor(i+3,4);
            WriteData(1);
            SetCursor(i+4,4);
            WriteData(1);
            
            // I
            i = i + 6;

            SetCursor(i,4);
            WriteData(0x81);

            SetCursor(i+1,4);
            WriteData(0xFF);

            SetCursor(i+2,4);
            WriteData(0x81);
            
            
            // E ///////////////////
            i = i + 4;

            SetCursor(i,4);
            WriteData(0xFF);

            SetCursor(i+1,4);
            WriteData(0x89);
            SetCursor(i+2,4);
            WriteData(0x89);
            SetCursor(i+3,4);
            WriteData(0x89);
        }

        /////////// ResetBoard ////////////

        player1_points = 0;
        player2_points = 0;

        Delay10KTCYx(36000);
        Delay10KTCYx(36000);
        Delay10KTCYx(36000);
        Delay10KTCYx(36000);
        Delay10KTCYx(36000);
        Delay10KTCYx(36000);

        // clear board
        for (j = 3 ; j <= 4 ; j++){
            for (i = 25 ; i <= 127-25  ; i++){
                SetCursor(i,j);
                WriteData(0x00);
            }
        }

        // clear scoreboard
        for (j = 0 ; j < 1 ; j++){
            for (i = 3 ; i <= 127-3  ; i++){
                SetCursor(i,j);
                WriteData(0x00);
            }
        }

        // print begin game
        begin_game();

        game = 0;
        while (~ PORTEbits.RE3) {
            // wait for player to press start
        }

        // clear board
        for (j = 0 ; j <= 8 ; j++){
            for (i = 0 ; i <= 127  ; i++){
                SetCursor(i,j);
                WriteData(0x00);
            }
        }

        // initialize game
        Initial_ball();
        print_paddle(1);
        print_paddle(2);

    }
    
}

//////////////////////////////////////////
// Function: begin_game
// Inputs: none
// Outputs: none
// What does it do?
/* This function prints the BEGIN? message
*/
//////////////////////////////////////////
void begin_game(void){
    unsigned char i;
    unsigned char k;
    unsigned char j;
    unsigned char p;
    unsigned char q;
    // need to display BEGIN?  in letters in the middle of the screen
    
    // First type out the letter B
    SetCursor(text,3);
    WriteData(0xFF);
    SetCursor(text,4);
    WriteData(0xFF);
    
    SetCursor(text+1,3);
    WriteData(0xFF);
    SetCursor(text+1,4);
    WriteData(0xFF);
    
    SetCursor(text+2, 3);
    WriteData(0x83);
    SetCursor(text+2,4);
    WriteData(0xC1);
    
    SetCursor(text+3, 3);
    WriteData(0x83);
    SetCursor(text+3,4);
    WriteData(0xC1);
    
    SetCursor(text+4, 3);
    WriteData(0x83);
    SetCursor(text+4,4);
    WriteData(0xC1);
    
    SetCursor(text+5,3);
    WriteData(0x86);
    SetCursor(text+5,4);
    WriteData(0x61);
    
    SetCursor(text+6,3);
    WriteData(0x86);
    SetCursor(text+6,4);
    WriteData(0x61);
    
    SetCursor(text+7,3);
    WriteData(0xCC);
    SetCursor(text+7,4);
    WriteData(0x33);
    
    SetCursor(text+8,3);
    WriteData(0x7C);
    SetCursor(text+8,4);
    WriteData(0x3E);
    
    SetCursor(text+9,3);
    WriteData(0x30);
    SetCursor(text+9,4);
    WriteData(0x0C);
    //////////////////////////////////////////////////////////////////////
    // Now write out the E
    
    SetCursor(text+13,3);
    WriteData(0xFF);
    SetCursor(text+13,4);
    WriteData(0xFF);
    
    SetCursor(text+14,3);
    WriteData(0xFF);
    SetCursor(text+14,4);
    WriteData(0xFF);
    
    for (i = text + 15; i < text + 23; i++ ){
        SetCursor(i,3);
        WriteData(0x83);
        SetCursor(i,4);
        WriteData(0xC1);
    }
    
    i = text + 26;
    ////////////////////////////////////////////////////////////////////////
    // now print the G
    
    SetCursor(i,3);
    WriteData(0xFC);
    SetCursor(i,4);
    WriteData(0x3F);
    
    SetCursor(i+1,3);
    WriteData(0xFE);
    SetCursor(i+1,4);
    WriteData(0x7F);
    
    SetCursor(i+2,3);
    WriteData(0x07);
    SetCursor(i+2,4);
    WriteData(0xE0);
    
    SetCursor(i+3,3);
    WriteData(0x03);
    SetCursor(i+3,4);
    WriteData(0xC0);
    
    SetCursor(i+4,3);
    WriteData(0x03);
    SetCursor(i+4,4);
    WriteData(0xC0);
    
    SetCursor(i+5,3);
    WriteData(0x83);
    SetCursor(i+5,4);
    WriteData(0xC1);
    
    SetCursor(i+6,3);
    WriteData(0x83);
    SetCursor(i+6,4);
    WriteData(0xC1);
    
    SetCursor(i+7,3);
    WriteData(0x83);
    SetCursor(i+7,4);
    WriteData(0x7F);
    
    SetCursor(i+8,3);
    WriteData(0x87);
    SetCursor(i+8,4);
    WriteData(0x3F);
    
    SetCursor(i+9,3);
    WriteData(0x87);
    SetCursor(i+9,4);
    WriteData(0x01);
    
    ///////////////////////////////////////////////////////////////////
    // Now print the I
    i = i + 17;
    
    SetCursor(i,3);
    WriteData(0xFF);
    SetCursor(i,4);
    WriteData(0xFF);
    
    SetCursor(i-1,3);
    WriteData(0xFF);
    SetCursor(i-1,4);
    WriteData(0xFF);
    
    SetCursor(i-2,3);
    WriteData(0x03);
    SetCursor(i-3,3);
    WriteData(0x03);
    
    SetCursor(i+1,3);
    WriteData(0x03);
    SetCursor(i+2,3);
    WriteData(0x03);
    
    
    SetCursor(i-2,4);
    WriteData(0xC0);
    SetCursor(i-3,4);
    WriteData(0xC0);
    
    SetCursor(i+1,4);
    WriteData(0xC0);
    SetCursor(i+2,4);
    WriteData(0xC0);

    
    ///////////////////////////////////////////////////////////////////
    // Now print the N
    
    i = i + 8;
    
    SetCursor(i,3);
    WriteData(0xFF);
    SetCursor(i,4);
    WriteData(0xFF);
    SetCursor(i-1,3);
    WriteData(0xFF);
    SetCursor(i-1,4);
    WriteData(0xFF);
    
    k = 1;
    p = 1;
    q = 1;
    for (j = i+1 ; j <= i+1+8; j ++){
    
        SetCursor(i+k,3);
        WriteData(p+2*p);
        SetCursor(i+1+k,3);
        WriteData(p+2*p);
        
        q++;
        if (q%2 == 0){
            k++;
        }
        p = p*2;
    }
   
    
    p = 1;
    q = 1;
    k --;
    for (j = i ; j <= i+1+8; j ++){
    
        SetCursor(i+k,4);
        WriteData(p + 2*p);
        SetCursor(i+1+k,4);
        WriteData(p+2*p);
        
        q++;
        if (q%2 == 0){
            k++;
        }
        p = p*2;
    }
    
    k--;
    SetCursor(i+k,3);
    WriteData(0xFF);
    SetCursor(i+k,4);
    WriteData(0xFF);
    SetCursor(i+k+1,3);
    WriteData(0xFF);
    SetCursor(i+k+1,4);
    WriteData(0xFF);
    
    ///////////////////////////////////////////////////////////////////
    // Finally, question mark...
    i = i + 15;
    SetCursor(i,3);
    WriteData(0x04);
    SetCursor(i+1,3);
    WriteData(0x02);
    
 
    for (j = 2; j <=6; j++){
        SetCursor(i+j,3);
        WriteData(0x03);
    }
    
    SetCursor(i+j,3);
    WriteData(0x06);
    SetCursor(i+j+1,3);
    WriteData(0x0C);
    SetCursor(i+j+2,3);
    WriteData(0xF8);
    
    SetCursor(i+j+2,4);
    WriteData(0x01);
    SetCursor(i+j,4);
    WriteData(0x01);
    SetCursor(i+j+1,4);
    WriteData(0x03);
    SetCursor(i+j,4);
    WriteData(0x06);
    SetCursor(i+j-1,4);
    WriteData(0x0C);
    SetCursor(i+j-2,4);
    WriteData(0xBC);
    SetCursor(i+j-3,4);
    WriteData(0xBC);
    
    
}

//////////////////////////////////////////
// Function: TMR0handler
// Inputs: none
// Outputs: none
// What does it do?
/* This function is ultimately useless, it
 * was just put here for completeness and so 
 * that the timer0 could not cause an interrupt
*/
//////////////////////////////////////////
void TMR0handler(void){
   // just clear the flag and move on
    INTCONbits.TMR0IF = 0;
}