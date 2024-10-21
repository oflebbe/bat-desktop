#ifndef FLO_FILE_H
#define FLO_FILE_H

#include <stdint.h>
#ifdef __cplusplus
extern "C" {
  uint8_t *flo_readfile( const char filename[], long *sz);
}
#else
  uint8_t *flo_readfile( const char filename[static 1], long *sz);
#endif


#ifdef FLO_FILE_IMPLEMENTATION
// read whole file. returns buffer and size
uint8_t *flo_readfile( const char filename[static 1], long *sz)
{
  if (sz == NULL) {
    return NULL;
  }
  FILE *fin = fopen( filename, "rb");
  if (!fin) {
    return NULL;
  }
  int p = fseek( fin, 0, SEEK_END);
  if (p < 0) {
    fclose(fin);
    return NULL;
  }
  long pos = ftell(fin);
  *sz = pos;
  fseek(fin, 0, SEEK_SET);
  if (pos % 2 == 1 || pos == 0) {
    fclose(fin);
    return NULL;
  }
  uint8_t *filebuf = calloc( pos, 1);
  if (!filebuf) {
    fclose(fin);
    return NULL;
  }
  if (pos != fread( filebuf, 1, pos, fin)) {
    fclose(fin);
    free( filebuf);
    return NULL;
  }
  fclose( fin);
  return filebuf;
}

#endif
#endif