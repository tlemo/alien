#pragma once

#define NUM_THREADS_PER_BLOCK 64
#define NUM_BLOCKS 64
#define NUM_CLUSTERS 5000/*160000/100*/

#define CELL_MAX_BONDS 6
#define CELL_MAX_DISTANCE 1.3
#define MAX_COLLIDING_CLUSTERS 10
#define PROTECTION_TIMESTEPS 60

#define DEG_TO_RAD 3.1415926535897932384626433832795/180.0
#define RAD_TO_DEG 180.0/3.1415926535897932384626433832795

#define FP_PRECISION 0.00001

