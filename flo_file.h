#ifndef FLO_FILE_H
#define FLO_FILE_H

#include <stdint.h>

size_t flo_filesize(FILE *fp);
const uint8_t *flo_readfile(FILE *fp, size_t *sz);
const uint8_t *flo_mapfile(FILE *fp, size_t *sz);
int flo_unmapfile(const uint8_t *buf, size_t sz);
const char *flo_readall(const char *fn);

#ifdef FLO_FILE_IMPLEMENTATION
#include <stdio.h>

const char *flo_readall(const char *fn)
{
  FILE *fp = fopen(fn, "r");
  if (!fp)
  {
    return NULL;
  }
  size_t sz = flo_filesize(fp);
  char *str = calloc(sz + 1, 1);
  if (!str)
  {
    return NULL;
  }
  if (sz != fread(str, 1, sz, fp))
  {
    free(str);
    return NULL;
  }
  return str;
}

size_t flo_filesize(FILE *fp)
{
  int p = fseek(fp, 0, SEEK_END);
  if (p < 0)
  {
    return 0;
  }
  long pos = ftell(fp);
  if (pos < 0) {
    abort();
  }
  fseek(fp, 0, SEEK_SET);
  return (size_t) pos;
}

// read whole file. returns buffer and size
const uint8_t *flo_readfile(FILE *fin, size_t *sz)
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
#ifdef _WIN32
#include <windows.h>
#include <io.h>

const uint8_t *flo_mapfile(FILE *fin, size_t *sz)
{

  if (sz == NULL)
  {
    return NULL;
  }
  if (!fin)
  {
    return NULL;
  }
  const size_t pos = *sz = flo_filesize(fin);
  const int fd = fileno(fin);
  const HANDLE file_handle = (HANDLE)_get_osfhandle(fd);
  const HANDLE mapping_handle = CreateFileMappingA(file_handle, NULL, PAGE_READONLY, 0, 0, "local_mmap");
  return (const uint8_t *)MapViewOfFile(mapping_handle, FILE_MAP_READ, 0, 0, 0);
}

int flo_unmapfile(const uint8_t *buf, size_t sz)
{
  return (int)UnmapViewOfFile(buf);
}
#else
  #include <sys/mman.h>
const uint8_t *flo_mapfile(FILE *fin, size_t *sz)
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

int flo_unmapfile(const uint8_t *buf, size_t sz)
{
  return munmap((void *)buf, sz);
}
#endif
#endif
#endif
