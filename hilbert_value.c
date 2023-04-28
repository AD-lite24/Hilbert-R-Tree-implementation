#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>


// Assumes coordinates are 32-bit signed integers
#define COORD_BITS 2                   // Number of bits used for each coordinate
#define HILBERT_BITS (COORD_BITS * 2) // Total number of bits in Hilbert curve

typedef struct rectangle rectangle;
typedef struct rectangle *RECTANGLE;
typedef struct element element;
typedef struct element *ELEMENT;

struct element
{
    int x;
    int y;
};

struct rectangle
{
    element low, high;
    int hilbertValue;
};

// Convert a coordinate to a binary number
uint32_t coord_to_binary(int32_t coord)
{
    return (uint32_t)(coord - INT32_MIN);
}

// Interleave the bits of two binary numbers
uint32_t interleave_bits(uint32_t x, uint32_t y)
{
    uint32_t z = 0;
    for (int i = 0; i < COORD_BITS; i++)
    {
        z |= ((x & (1 << i)) << i) | ((y & (1 << i)) << (i + 1));
    }
    return z;
}

// Compute the Hilbert value of a binary number from a 16 order curve
uint32_t hilbert_value(uint32_t z)
{
    uint32_t h = 0;
    for (int i = HILBERT_BITS - 1; i >= 0; i--)
    {
        h ^= ((z >> i) & 1) << (i * 2 + 1);
        if ((i & 1) == 0)
        {
            uint32_t t = ((h >> 2) & 0x3) | ((h & 0x3) << 2);
            h = (h & ~0x3) | t;
        }
    }
    return h;
}

void rot(int n, int *x, int *y, int rx, int ry)
{
    if (ry == 0)
    {
        if (rx == 1)
        {
            *x = n - 1 - *x;
            *y = n - 1 - *y;
        }

        // Swap x and y
        int t = *x;
        *x = *y;
        *y = t;
    }
}

int xy2d(int n, int x, int y)
{
    int rx, ry, s, d = 0;
    for (s = n / 2; s > 0; s /= 2)
    {
        rx = (x & s) > 0;
        ry = (y & s) > 0;
        d += s * s * ((3 * rx) ^ ry);
        rot(n, &x, &y, rx, ry);
    }
    return d;
}

uint32_t hilbert_rect_center(RECTANGLE r)
{
    int32_t xmid = (r->low.x + r->high.x) / 2;
    int32_t ymid = (r->low.y + r->high.y) / 2;
    // uint32_t x = coord_to_binary(xmid);
    // uint32_t y = coord_to_binary(ymid);
    // uint32_t z = interleave_bits(x, y);
    // return hilbert_value(z);
    return xy2d(16, xmid, ymid);
}

RECTANGLE createRectangle(int lowx, int lowy, int highx, int highy)
{
    RECTANGLE newRec = (RECTANGLE)malloc(sizeof(rectangle));
    newRec->high.x = highx;
    newRec->low.x = lowx;
    newRec->low.y = lowy;
    newRec->high.y = highy;
    newRec->hilbertValue = hilbert_rect_center(newRec);
    return newRec;
}

int main()
{   
    //Create rectangle with coords (0,0) and (4,4) = 160
    //Create rectangle with coords (0,0) and (2,4) = 136
    //Create rectangle with coords (1,1) and (1,1) = 10
    RECTANGLE rect = createRectangle(8, 15, 8, 15);
    printf("%d", rect->hilbertValue);
}
