#ifndef PINK_H
#define PINK_H

#define PINK_MAX_RANDOM_ROWS   (30)
#define PINK_RANDOM_BITS       (24)
#define PINK_RANDOM_SHIFT      ((sizeof(long)*8)-PINK_RANDOM_BITS)

typedef struct {
    long      pink_Rows[PINK_MAX_RANDOM_ROWS];
    long      pink_RunningSum;   /* Used to optimize summing of generators. */
    int       pink_Index;        /* Incremented each sample. */
    int       pink_IndexMask;    /* Index wrapped by ANDing with this mask. */
    float     pink_Scalar;       /* Used to scale within range of -1.0 to +1.0 */
} PinkNoise;

void InitializePinkNoise(PinkNoise *pink, int numRows);
float GeneratePinkNoise(PinkNoise *pink);

#endif
