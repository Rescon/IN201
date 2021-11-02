#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

struct fs_header {
  // TO BE WRITTEN
};

void decode(struct fs_header *fs, size_t size) {
  (void) size; // to avoid a "unused paramater" warning from the compiler
  // it's better to have warnings enabled with -Wall -Werror and
  // silence them this way on a case by case basis.
  (void) fs;
}

int main(void){

  int fd = open("fs.romfs",O_RDONLY);
  assert(fd != -1);
  off_t fsize;
  fsize = lseek(fd,0,SEEK_END);

  //  printf("size is %d", fsize);
  void *addr = mmap(NULL, fsize, PROT_READ, MAP_SHARED, fd, 0);
  assert(addr != MAP_FAILED);
  decode(addr, fsize);
  return 0;
}
