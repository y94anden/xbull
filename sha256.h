#ifndef SHA256_H__
#define SHA256_H__

#include <avr/pgmspace.h>
#include <stdint.h>
#include <string.h>

typedef uint32_t uint32;
typedef int32_t int32;

#define USE_MATRIX_SHA256
#define SHA256_HASHLEN 32
#define PS_SUCCESS         0   // Just guessing...

#ifndef min
#define min(x,y) (x < y ? x : y)
#endif

#ifndef max
#define max(x,y) (x > y ? x : y)
#endif



struct psSha256_t
{
    uint32 lengthHi;
    uint32 lengthLo;
    uint32 state[8], curlen;
    unsigned char buf[64];
};

#define STORE32H(x, y) { \
    (y)[0] = (unsigned char) (((x) >> 24) & 255);     \
    (y)[1] = (unsigned char) (((x) >> 16) & 255);     \
    (y)[2] = (unsigned char) (((x) >> 8) & 255);      \
    (y)[3] = (unsigned char) ((x) & 255);             \
}

#define LOAD32H(x, y) { \
    x = ((unsigned long) ((y)[0] & 255) << 24) |     \
      ((unsigned long) ((y)[1] & 255) << 16) |       \
      ((unsigned long) ((y)[2] & 255) << 8)  |       \
      ((unsigned long) ((y)[3] & 255));              \
  }

#  define ROL(x, y) \
    ( (((unsigned long) (x) << (unsigned long) ((y) & 31)) | \
       (((unsigned long) (x) & 0xFFFFFFFFUL) >> (unsigned long) (32 - ((y) & 31)))) & \
      0xFFFFFFFFUL)
#  define ROR(x, y) \
    ( ((((unsigned long) (x) & 0xFFFFFFFFUL) >> (unsigned long) ((y) & 31)) | \
       ((unsigned long) (x) << (unsigned long) (32 - ((y) & 31)))) & 0xFFFFFFFFUL)


int32_t psSha256Init(struct psSha256_t *sha256);
void psSha256Update(struct psSha256_t *sha256, const unsigned char *buf, uint32_t len);
void psSha256Final(struct psSha256_t *sha256, unsigned char hash[SHA256_HASHLEN]);

#endif // SHA256_H__
