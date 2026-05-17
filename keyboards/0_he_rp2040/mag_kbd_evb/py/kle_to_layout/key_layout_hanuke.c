#include <stdint.h>
#include "config.h"
#include "src/include/key_layout.h"

// rows = 4
// cols = 8

const key_layout_t key_layout[MATRIX_SIZE] = {
//  {valid, x, y, size}
	// row 0
	{  0, 0, 0, U_1 }, // col 0 idx0
	{  1, 21, 0, U_1 }, // col 1 idx1
	{  1, 25, 0, U_1 }, // col 2 idx2
	{  1, 13, 0, U_1 }, // col 3 idx3
	{  1, 29, 0, U_1 }, // col 4 idx4
	{  1, 9, 0, U_1 }, // col 5 idx5
	{  1, 5, 4, U_1_5 }, // col 6 idx6
	{  1, 5, 0, U_1 }, // col 7 idx7
	// row 1
	{  1, 23, 4, U_1 }, // col 0 idx8
	{  1, 5, 8, U_1_75 }, // col 1 idx9
	{  0, 0, 0, U_1 }, // col 2 idx10
	{  1, 19, 4, U_1 }, // col 3 idx11
	{  1, 16, 8, U_1 }, // col 4 idx12
	{  0, 0, 0, U_1 }, // col 5 idx13
	{  1, 20, 8, U_1 }, // col 6 idx14
	{  1, 11, 4, U_1 }, // col 7 idx15
	// row 2
	{  1, 13, 12, U_1 }, // col 0 idx16
	{  0, 0, 0, U_1 }, // col 1 idx17
	{  1, 21, 12, U_1 }, // col 2 idx18
	{  1, 5, 12, U_2 }, // col 3 idx19
	{  1, 25, 12, U_1 }, // col 4 idx20
	{  1, 28, 4, ISO }, // col 5 idx21
	{  1, 29, 12, U_1 }, // col 6 idx22
	{  1, 24, 8, U_1 }, // col 7 idx23
	// row 3
	{  1, 27, 16, 20 }, // col 0 idx24
	{  1, 0, 16, U_1 }, // col 1 idx25
	{  1, 0, 12, U_1 }, // col 2 idx26
	{  1, 16, 16, U_2_75 }, // col 3 idx27
	{  1, 0, 8, U_1 }, // col 4 idx28
	{  1, 11, 16, U_1_25 }, // col 5 idx29
	{  1, 0, 4, U_1 }, // col 6 idx30
	{  1, 5, 16, U_1_5 }, // col 7 idx31
};

/*
/if you want to use not listed width, enter 4x wanted size[U] integer.

#define ISO        0
#define U_1        4
#define U_1_25     5
#define U_1_5      6
#define U_1_75     7
#define U_2        8
#define U_2_25     9
#define U_2_75     11
#define U_3        12
#define U_4_5      18
#define U_6        24
#define U_6_25     25
#define U_7        28

typedef struct {
    uint8_t valid;
    uint8_t x;
    uint8_t y;
    uint8_t w;
} key_layout_t;
*/