/* 
 * File:   Pong_funcs.h
 * Author: Nick
 *
 * Created on November 18, 2017, 2:42 PM
 */

#include <p18cxxx.h>

#ifndef PONG_FUNCS_H
#define	PONG_FUNCS_H

// now define the struct for the the ball
struct ball_struct {
   float  x_pos;
   float  y_pos;
   float theta;
   unsigned char done_waiting;
};  

/******************************************************************************
 *     Function Name:	Initial_ball
 *     Parameters:      void
 *     Description:		This function initializes the x and y pos
 *
 ******************************************************************************/
void Initial_screen(void);

void Initial_ball(void);

void check_reset(void);

// ADD FUNCTION DESCRIPTION HERE
void update_ball(void);

void goal_scored(unsigned char player);

// add function description here
void update_paddles(void);

void TMR1handler(void);

void TMR3handler(void);

void TMR5handler(void);

void print_paddle(unsigned char player);

void paddle_reflect(unsigned char player, unsigned char pos);

void clear_paddle(unsigned char player);

void total_paddle_clear(void);

void score_board(void);

void begin_game(void);

void end_game(void);


void TMR0handler(void);
#endif	/* PONG_FUNCS_H */

