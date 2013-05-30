/* General I/O interface for reading big-endian binary data */

#include <assert.h>
#include "io.h"

// Read a 64-bit integer from a buffer
unsigned long long read64 (byte *b) {
  return (((unsigned long long)(b[0]))<<56) | (((unsigned long long)(b[1]))<<48) | (((unsigned long long)(b[2]))<<40) | (((unsigned long long)(b[3]))<<32) | (((unsigned long long)(b[4]))<<24) | (((unsigned long long)(b[5]))<<16) | (((unsigned long long)(b[6]))<<8) | (((unsigned long long)(b[7]))<<0);
}

// Read a 32-bit integer from a buffer
unsigned int read32 (byte *b) {
  return (((unsigned int)(b[0]))<<24) | (((int)(b[1]))<<16) | (((int)(b[2]))<<8) | (((int)(b[3]))<<0);
}

// Read a 24-bit integer from a buffer
int read24 (byte *b) {
  return (((int)(b[0]))<<16) | (((int)(b[1]))<<8) | (((int)(b[2]))<<0);
}

// Read a 16-bit integer from a buffer
int read16 (byte *b) {
  return (((int)(b[0]))<<8) | (((int)(b[1]))<<0);
}

// Read a 32-bit integer from a stream
unsigned int fread32 (FILE *f) {
  byte b[4];
  int n = fread (b, 1, 4, f);
  assert (n == 4);
  return read32(b);
}

// Read a 32-bit float from a stream
float freadfloat (FILE *f) {
  int x = fread32(f);
  return *((float*)(&x));
}

// Validate the byte count at the beginning/end of an unformatted Fortran stream
void ftn_section (FILE *f, int n) {
  int m = fread32(f);
  assert (m == n);
}

