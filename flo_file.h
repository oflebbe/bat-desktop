#ifndef FLO_FILE_H
#define FLO_FILE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


size_t flo_filesize(FILE *fp);
uint8_t *flo_readfile(FILE *fp, size_t *sz);
uint8_t *flo_mapfile(FILE *fp, size_t *sz);
int flo_unmapfile(uint8_t *buf, size_t sz);

#ifdef __cplusplus
}
#endif


#ifdef FLO_FILE_IMPLEMENTATION

size_t flo_filesize(FILE *fp)
{
  int p = fseek(fp, 0, SEEK_END);
  if (p < 0)
  {
    return 0;
  }
  long pos = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  return pos;
}

// read whole file. returns buffer and size
uint8_t *flo_readfile(FILE *fin, size_t *sz)
{
  if (sz == NULL)
  {
    return NULL;
  }
  if (!fin)
  {
    return NULL;
  }

  size_t pos = *sz = flo_filesize(fin);
  if (*sz == 0)
  {
    return NULL;
  }
  uint8_t *filebuf = calloc(pos, 1);
  if (!filebuf)
  {
    return NULL;
  }

  if (pos != fread(filebuf, 1, pos, fin))
  {
    free(filebuf);
    return NULL;
  }
  return filebuf;
}

#include <sys/mman.h>
#include <stdio.h>
uint8_t *flo_mapfile(FILE *fin, size_t *sz)
{

  if (sz == NULL)
  {
    return NULL;
  }
  if (!fin)
  {
    return NULL;
  }
  size_t pos = *sz = flo_filesize(fin);

  int fd = fileno(fin);
  uint8_t *filebuf = mmap(NULL, pos, PROT_READ, MAP_PRIVATE, fd, 0);
  return filebuf;
}

int flo_unmapfile(uint8_t *buf, size_t sz)
{
  return munmap(buf, sz);
}

#endif
#endif