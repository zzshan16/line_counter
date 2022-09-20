#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int random_int(int x){
  unsigned int i;
  unsigned char buffer[4];
  int fd = open("/dev/urandom", O_RDONLY);
  read(fd, buffer, 4);
  close(fd);
  i = buffer[0];
  i += buffer[1]<<8;
  i += buffer[2]<<16;
  i += buffer[3]<<24;
  return i % x;
}
int main(int argc, char** argv){
  if (argc != 3){
    printf("%s FILESIZE(in KiB) FILENAME\n", *argv);
    return -1;
  }
  int size = atoi(*(argv+1));
  if (size < 0){
    puts("size must be positive");
    return -2;
  }
  if (size > 10000000){
    puts("size must be less than 10000000");
    return -2;
  }
  if (strlen(*(argv+2)) > 10){
    puts("filename must be fewer than 11 characters");
    return -3;
  }
  FILE* fp = fopen(*(argv+2), "w");
  int j;
  while (size--){
    for(int i = 0; i < 1024; ++i){
      j = random_int(95);
      if (j == 0)
	putc('\n', fp);
      else{
	putc(j+31, fp);
      }
    }
  }
  fclose(fp);
  return 0;
}
