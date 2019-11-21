// This C module generates pink noise continuously.

#ifndef PINK_H
#define PINK_H

#define PINK_MAX_RANDOM_ROWS   (30)
#define PINK_RANDOM_BITS       (24)
#define PINK_RANDOM_SHIFT      ((sizeof(long)*8)-PINK_RANDOM_BITS)

// Container for the noise information.
typedef struct {
    long      pink_Rows[PINK_MAX_RANDOM_ROWS];
    long      pink_RunningSum;   /* Used to optimize summing of generators. */
    int       pink_Index;        /* Incremented each sample. */
    int       pink_IndexMask;    /* Index wrapped by ANDing with this mask. */
    float     pink_Scalar;       /* Used to scale within range of -1.0 to +1.0 */
} PinkNoise;

// This function initializes the data container used to track the noise to play.
void InitializePinkNoise(PinkNoise *pink, int numRows);

// This function returns a generated pink noise value between -1.0 and +1.0.
// Random generation using a seed is used.
// Must have called InitializePinkNoise() first.
float GeneratePinkNoise(PinkNoise *pink);

#endif
