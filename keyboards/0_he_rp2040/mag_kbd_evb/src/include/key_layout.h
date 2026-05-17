#ifndef KEY_LAYOUT_H
#define KEY_LAYOUT_H

#include <stdint.h>
#include "config.h"

//if you want to use not listed width, enter 4x wanted size[U] integer.
#define ISO		0
#define U_1		4
#define U_1_25	5
#define U_1_5	6
#define U_1_75	7
#define U_2		8
#define U_2_25	9
#define U_2_75	11
#define U_3		12
#define U_4_5	18
#define U_6		24
#define U_6_25	25
#define U_7		28

typedef struct {
	uint8_t valid;	//valid:1, invalid:0
	uint8_t x;		//key layout x
	uint8_t y;		//key layout y
	uint8_t w;		//key width ,type
} key_layout_t;

extern const key_layout_t key_layout[MATRIX_SIZE];

#endif