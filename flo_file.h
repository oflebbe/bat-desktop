#ifndef FLO_FILE_H
#define FLO_FILE_H

#include <stdint.h>

size_t flo_filesize(FILE fp[static 1]);
const uint8_t *flo_readfile(const char fn[static 1], size_t sz[static 1]);
const uint8_t *flo_mapfile(const char fn[static 1], size_t sz[static 1]);
int flo_unmapfile(const uint8_t buf[static 1], size_t sz);

#ifdef FLO_FILE_IMPLEMENTATION
#include <stdio.h>
#include <stdlib.h>

size_t flo_filesize(FILE fp[static 1])
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
const uint8_t *flo_readfile(const char fn[static 1], size_t sz[static 1])
{
  FILE *fp = fopen(fn, "r");
  if (!fp)
  {
    return nullptr;
  }
 
  size_t pos = *sz = flo_filesize(fp);
  if (*sz == 0)
  {
    return nullptr;
  }
  uint8_t *filebuf = calloc(pos, 1);
  if (!filebuf)
  {
    return nullptr;
  }

  if (pos != fread(filebuf, 1, pos, fp))
  {
    free(filebuf);
    fclose(fp);
    return nullptr;
  }
  fclose(fp);
  return filebuf;
}
#ifdef _WIN32
#include <windows.h>
#include <io.h>

const uint8_t *flo_mapfile(const char fn[static 1], size_t sz[static 1])
{
  if (!sz)
  {
    return nilptr;
  }
  FILE *fp = fopen(fin, "r");
  if (!fp)
  {
    return nullptr;
  }
 
  const size_t pos = *sz = flo_filesize(fp);
  if (pos == 0) {
    return nullptr;
  }
  const int fd = fileno(fp);
  fclose( fp);
  const HANDLE file_handle = (HANDLE)_get_osfhandle(fd);
  const HANDLE mapping_handle = CreateFileMappingA(file_handle, NULL, PAGE_READONLY, 0, 0, "local_mmap");
  return (const uint8_t *)MapViewOfFile(mapping_handle, FILE_MAP_READ, 0, 0, 0);
}

int flo_unmapfile(const uint8_t buf[static 1], size_t sz)
{
  return (int)UnmapViewOfFile(buf);
}
#else
  #include <sys/mman.h>
const uint8_t *flo_mapfile(const char fn[static 1], size_t sz[static 1])
{
  FILE *fp = fopen(fn, "r");
  if (!fp)
  {
    return nullptr;
  }
 
  size_t pos = *sz = flo_filesize(fp);

  int fd = fileno(fp);
  const uint8_t *filebuf = mmap(NULL, pos, PROT_READ, MAP_PRIVATE, fd, 0);
  fclose(fp);
  return filebuf;
}

int flo_unmapfile(const uint8_t buf[static 1], size_t sz)
{
  return munmap((void *)buf, sz);
}
#endif
#endif
#endif
