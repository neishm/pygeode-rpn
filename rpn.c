#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include "io.h"
#include "rpn.h"

// pseudo-RPN interface

// Read packed characters
void readchar (char *dest, byte *src, int n) {
  for (int i = 0; i < n; i++) {
    dest[i] = 0;
    // first byte needed
    byte j = (i*6)/8;
    // shift from the beginning
    byte sh = (i*6)%8;
    dest[i] |= ((byte)(src[j] << sh) >> 2);
    // do we need a second byte?
    if (sh > 2) dest[i] |= (src[j+1]>>(10-sh));
    dest[i] += 32;
  }
}

void read_file_header (FILE *f, FileHeader *h) {
  int nbytes;
  byte buf[208];
  nbytes = fread (buf, 1, 208, f);
  assert (nbytes == 208);
  assert (buf[0] == 0); //idtyp
  assert (read24(buf+1) == 26);  // header length (words)
  assert (read32(buf+4) == 0); // address of file header
  assert (strncmp(buf+8,"XDF0",4) == 0); // XDF version
  assert (strncmp(buf+12,"STDR",4) == 0); // application signature
  h->file_size = read32(buf+16) * 8L;
  h->num_overwrites = read32(buf+20);
  h->num_extensions = read32(buf+24);
  h->nchunks = read32(buf+28);
  h->last_chunk = read32(buf+32) * 8L;
  h->max_data_length = read32(buf+36) * 8;
  assert (read16(buf+40) == 16); // # of primary keys
  assert (read16(buf+42) == 9);  // length of primary keys (words)
  assert (read16(buf+44) == 2);  // # of auxiliary keys
  assert (read16(buf+46) == 1);  // length of auxiliary keys (words)
  h->num_erasures = read32(buf+48);
  h->nrecs = read32(buf+52);
  assert (read32(buf+56) == 0);  // read/write flag
  assert (read32(buf+60) == 0);  // reserved area
  char ncle[5];
  // Validate primary keys
  for (int i = 0; i < 16; i++) {
    sprintf (ncle, "SF%02d", i+1);
    assert (strncmp(buf+64+i*8, ncle, 4) == 0);  // validate key names
    assert ((read16(buf+64+i*8+4)>>3) == 31+i*32);  // validate bit1
    assert ((read24(buf+64+i*8+5)&0x7FFFF) == 0x7C000); // validate lcls/tcle
  }
  // Validate auxiliary keys
  for (int i = 0; i < 2; i++) {
    sprintf (ncle, "AXI%01d", i+1);
    assert (strncmp(buf+192+i*8, ncle, 4) == 0);  // validate key names
    assert ((read16(buf+192+i*8+4)>>3) == 31+i*32);  // validate bit1
    assert ((read24(buf+192+i*8+5)&0x7FFFF) == 0x7C000); // validate lcls/tcle
  }
}

void print_file_header (FileHeader *h) {
  printf ("file_size: %lld, num_overwrites: %d, num_extensions: %d, nchunks: %d, last_chunk: %llx, max_data_length: %d, num_erasures: %d, nrecs: %d\n", h->file_size, h->num_overwrites, h->num_extensions, h->nchunks, h->last_chunk, h->max_data_length, h->num_erasures, h->nrecs);
}

void read_chunk_header (FILE *f, ChunkHeader *h) {
  int nbytes;
  byte buf[32];
  nbytes = fread (buf, 1, 32, f);
  assert (nbytes == 32);
  assert (buf[0] == 0); // idtyp
  assert (read24(buf+1) == 2308);  // Header length (words)
  h->this_chunk_words = read32(buf+4);
  h->this_chunk = h->this_chunk_words * 8L - 8;
  assert (read32(buf+8) == 0); // reserved1
  assert (read32(buf+12) == 0); // reserved2
  h->next_chunk_words = read32(buf+16);
  h->next_chunk = h->next_chunk_words * 8L;
  if (h->next_chunk > 0) h->next_chunk -= 8; // Rewind a bit
  h->nrecs = read32(buf+20);
  h->checksum = read32(buf+24);
  assert (read32(buf+28) == 0);  // reserved3
}

void print_chunk_header (ChunkHeader *h) {
  printf ("this_chunk: %llx, next_chunk: %llx, nrecs: %d, checksum: %x\n", h->this_chunk, h->next_chunk, h->nrecs, h->checksum);
}


void read_record_header (FILE *f, RecordHeader *h) {
  int nbytes;
  byte buf[72];
  nbytes = fread (buf, 1, 72, f);
  assert (nbytes == 72);
  h->status = buf[0];
  h->size = read24(buf+1);
  h->data = read32(buf+4) * 8L;
  assert (h->data > 8);
  h->data -= 8;  // rewind a bit to get the proper start of the data
  h->deet = read24(buf+8);
  h->npak = buf[11];
  h->ni = read24(buf+12);
  h->grtyp = buf[15];
  h->nj = read24(buf+16);
  h->datyp = buf[19];
  h->nk = read24(buf+20)>>4;
  //TODO: ubc
  h->npas = (read32(buf+24)>>6);
  h->ig4 = read24(buf+28);
  h->ig2 = buf[31]*65536 + buf[35]*256 + buf[39];
  h->ig1 = read24(buf+32);
  h->ig3 = read24(buf+36);
  readchar (h->etiket, buf+40, 5);
  readchar (h->etiket+5, buf+44, 5);
  readchar (h->etiket+10, buf+48, 2);
  h->etiket[12] = 0;
  h->typvar[0] = ((buf[49]&0x0F)<<2) + (buf[50]>>6) + 32;
  h->typvar[1] = (buf[50]&0x3F) + 32;
  h->typvar[2] = 0;
  readchar (h->nomvar, buf+52, 4);
  h->nomvar[4] = 0;
  h->ip1 = read32(buf+56) >> 4;
  h->ip2 = read32(buf+60) >> 4;
  h->ip3 = read32(buf+64) >> 4;
  h->dateo = read32(buf+68);

  // Checksum
  h->checksum = 0;
  for (int i = 0; i < 72; i+= 4) {
    h->checksum ^= ((buf[i]<<24) | (buf[i+1]<<16) | (buf[i+2]<<8) | buf[i+3]);
  }
}

void print_record_header (RecordHeader *h) {
  printf ("status: %d, size: %d, data pointer: %llx, deet: %d, npak: %d, ni: %d, grtyp: '%c', nj: %d, datyp: %d nk: %d, npas: %d, ig4: %d, ig2: %d, ig1: %d, ig3: %d, etiket: '%s', typvar: '%s', nomvar: '%s', ip1: %d, ip2: %d, ip3: %d, \ndateo: %lld\n", h->status, h->size, h->data, h->deet, h->npak, h->ni, h->grtyp, h->nj, h->datyp, h->nk, h->npas, h->ig4, h->ig2, h->ig1, h->ig3, h->etiket, h->typvar, h->nomvar, h->ip1, h->ip2, h->ip2, h->dateo);
}


// Get the number of records in a file
int get_num_records (char *filename) {
  FileHeader h;
  FILE *file = fopen (filename, "rb");
  read_file_header (file, &h);
  fclose (file);
  return h.nrecs;
}

// Read the records into a pre-allocated array
int get_record_headers (char *filename, RecordHeader *h) {
  FILE *f = fopen(filename, "rb");
  FileHeader fileheader;
  fseek (f, 0, SEEK_SET);
  read_file_header (f, &fileheader);
  int rec = 0;
  for (int c = 0; c < fileheader.nchunks; c++) {
    ChunkHeader chunkheader;
    read_chunk_header (f, &chunkheader);
    unsigned int checksum = 0;
    for (int r = 0; r < chunkheader.nrecs; r++) {
      read_record_header (f, h+rec);
      // Skip erased records
      if (h[rec].status == 255) continue;
      checksum ^= h[rec].checksum;
      rec++;
    }
    checksum ^= chunkheader.nrecs;
    checksum ^= (chunkheader.next_chunk_words);
//    printf ("chunk %d checksum: 0x%08X   computed checksum: 0x%08X, xor: %08X\n", c, chunkheader.checksum, checksum, chunkheader.checksum ^ checksum);
    assert (checksum == chunkheader.checksum);
    // Go to next chunk
    fseek (f, chunkheader.next_chunk, SEEK_SET);
  }
  fclose (f);
  return 0;
}


// Compare RecordHeaders for equality
// NOTE: this used to compare a RecordHeader with a Varinfo_entry,
// hence the 'h' and 'v' names.
int receq (RecordHeader *h, RecordHeader *v) {
  if (strncmp(h->nomvar, v->nomvar, sizeof(Nomvar)) != 0) {
    printf ("'%s' != '%s'\n", h->nomvar, v->nomvar);
    return 0;
  }
  if (strncmp(h->etiket, v->etiket, sizeof(Etiket)) != 0) return 0;
  if (strncmp(h->typvar, v->typvar, sizeof(Typvar)) != 0) return 0;
  if (h->grtyp != v->grtyp) return 0;
  if (h->ip1 != v->ip1) return 0;
  if (h->ip2 != v->ip2) return 0;
  if (h->ip3 != v->ip3) return 0;
  if (h->ig1 != v->ig1) return 0;
  if (h->ig2 != v->ig2) return 0;
  if (h->ig3 != v->ig3) return 0;
  if (h->ig4 != v->ig4) return 0;
  if (h->ni != v->ni) return 0;
  if (h->nj != v->nj) return 0;
  if (h->nk != v->nk) return 0;
  return 1;

}


// Read a chunk of data
int read_data (char *filename, int nrecs, RecordHeader *headers, int recsize, float *out) {

  FILE *file = fopen (filename, "rb");

  // Loop over all records
  for (int rec = 0; rec < nrecs; rec++) {

      unsigned long long offset = headers[rec].data;
//      printf ("offset: %llx\n", offset);
      RecordHeader h;
      fseek (file, offset, SEEK_SET);
      read_record_header (file, &h);
//      print_record_header (&h);
      // Make sure the header matches
      assert (receq(headers+rec, &h) == 1);
      assert (h.datyp == 1 || h.datyp == 5 || h.datyp == 133); //currently only support packed floating-point/IEEE floating point/compressed IEEE floating point

      byte b[4];
      fread (b, 1, 4, file);
      assert (read32(b) == 0);
      fread (b, 1, 4, file);
      assert (read32(b) == 0);

      // Easiest case: 32-bit IEEE float
      if (h.datyp == 5) assert (h.npak == 32 || h.npak == 64);
      if (h.datyp == 5 && h.npak == 32) {
        assert (sizeof(float) == sizeof(uint32_t));
        byte *raw = malloc(4*recsize);
        fread (raw, 4, recsize, file);
        for (int i = 0; i < recsize; i++) {
          ((uint32_t*)(out))[i] = read32(raw+4*i);
        }
        free(raw);
      }
      // Another easy case: 64-bit IEEE float
      else if (h.datyp == 5 && h.npak == 64) {
        assert (sizeof(double) == sizeof(uint64_t));
        byte *raw = malloc(8*recsize);
        fread (raw, 8, recsize, file);
        for (int i = 0; i < recsize; i++) {
          ((uint64_t*)(out))[i] = read64(raw+8*i);
        }
        free(raw);
      }

      // Compressed?
      else if (h.datyp == 133) {
        unsigned char buf[h.size*8];
        fread (buf, 1, h.size*8, file);
        read_compress32 (buf, &h, recsize, out);
      }

      // Other supported case: packed float
      else if (h.datyp == 1) {

        // Get and validate record size
        fread (b, 1, 4, file);
        unsigned int marker_and_recsize = read32(b);
        int marker = (marker_and_recsize >> 20);
        int recsize_ = marker_and_recsize & 0xFFFFF;
        assert (marker == 0x7ff);  // Check if supported header type
        assert (recsize_ == recsize);

        // Get the exponent used in the encoding
        fread (b, 1, 2, file);
        int range_exp = read16(b) - 4096;

        // Get the min value exponent and sign
        fread (b, 1, 2, file);
        int min_expo_sign = read16(b);
        int min_expo = (min_expo_sign >> 4) + 127 - 1024 + 48;  // based on compact_h.c
        int min_sign = min_expo_sign & 0x000F;

        // Get the min value mantissa
        fread (b, 1, 4, file);
        int min_mantissa = read24(b);  // skipping last byte (not used in 32-bit float encoding)
        assert (b[3] == 0);  // the last byte in the encoded mantissa should be zero?

        // Construct the min value
        // note: already includes the '1' in '1.xxxxx' (24th bit)
        float min = (min_mantissa / 8388608.) * pow(2,min_expo-127);
        if (min_sign == 1) min *= -1;
        // Special case - min is 0
        //if (min_mantissa == 0 || min_expo < 849) min = 0;

        // Skip over the 16-bit mantissa argument (not used?)
        fread (b, 1, 2, file);
        assert (b[0] == 0 && b[1] == 0);  // don't know what to do with a 16-bit mantissa

        // Get and verify npak
        fread (b, 1, 1, file);
        int npak = b[0];
        assert (npak == h.npak);
//        printf ("npak: %d\n", npak);

        // Fast case: pack=16
        if (npak == 16) {
          byte *raw = malloc(2*recsize);
          fread (raw, 2, recsize, file);
          for (int i = 0; i < recsize; i++) {
            out[i] = read16(raw+2*i);
          }
          free (raw);
        }

        // Fast case: pack=24
        else if (npak == 24) {
          byte *raw = malloc(3*recsize);
          fread (raw, 3, recsize, file);
          for (int i = 0; i < recsize; i++) {
            out[i] = read24(raw+3*i);
          }
          free (raw);
        }

        // Fast case: pack=32
        else if (npak == 32) {
          byte *raw = malloc(4*recsize);
          fread (raw, 4, recsize, file);
          for (int i = 0; i < recsize; i++) {
            out[i] = read32(raw+4*i);
          }
          free (raw);
        }

        // Slow case: other packing density
        else {
          printf ("warning: slow unpacking!\n");
          // Read the data into a buffer
          byte *raw = malloc(recsize * npak / 8);
          byte *bits = malloc(recsize * npak);
          unsigned int *codes = malloc(sizeof(unsigned int) * recsize);
          // First, read in bytes
          fread (raw, 1, recsize*npak/8, file);
          // Next, expand bytes into bit array
          for (int i = 0; i < recsize * npak / 8; i++) {
            byte x = raw[i];
            for (int j = 7; j >= 0; j--) {
              bits[i*8+j] = x & 0x01;
              x >>= 1;
            }
          }
          // Now, collapse this back into an integer code of size <npak>
          for (int i = 0; i < recsize; i++) {
            unsigned int x = 0;
            for (int j = 0; j < npak; j++) x = (x<<1) + bits[i*npak + j];
            codes[i] = x;
//            printf ("%d ", x);
          }
//          printf ("\n");
          // Decode this into a float
          for (int i = 0; i < recsize; i++) {
            out[i] = codes[i];
          }
//          printf ("\n");
          free (codes);
          free (bits);
          free (raw);
        }

        // Finish decoding the values
        float mulfactor = pow(2,range_exp);
        for (int i = 0; i < recsize; i++) {

          out[i] = out[i] * mulfactor + min;
          // note: original code (compact_h.c) multiplies mulfactor by 1.0000000000001.  why???

        }

      }


      out += recsize;
  }

  fclose (file);

  return 0;
}

