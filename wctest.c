#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
unsigned int count_by_system_wc(char* filename){
  char command_string[100];
  sprintf(command_string, "sed '/^ *$/d' \"%s\" | wc -l", filename);
  FILE* fp = popen(command_string, "r");
  fgets(command_string, 100, fp);
  fclose(fp);
  return atoi(command_string);
}
unsigned int count_by_getc_single_thread(char* filename){
  FILE* fp = fopen(filename, "r");
  unsigned int count = 0;
  int current;
  unsigned int blank = 0;
  while((current = getc(fp)) > 0){
    switch(current){
    case '\n':
      count += blank;
      blank = 0;
      break;
    case ' ':
      break;
    default:
      blank = 1;
    }
  }
  fclose(fp);
  return count;
}

int main(int argc, char** argv){
  if (argc != 2){
    printf("%s FILENAME\n", *argv);
    return -1;
  }
  if (access(*(argv+1), F_OK) != 0) {
    puts("file not found");
    printf("%s FILENAME\n", *argv);
    return -2;
  }
  long long unsigned int count = 0;
  struct timespec start_time,end_time;
  long signed int nsec;
  long unsigned int sec;
  clock_gettime(CLOCK_REALTIME, &start_time);
  for(int i = 0; i < 4; ++i){
    count += count_by_system_wc(*(argv+1));
  }
  clock_gettime(CLOCK_REALTIME, &end_time);
  count = count >> 2;
  nsec = end_time.tv_nsec - start_time.tv_nsec;
  sec = end_time.tv_sec - start_time.tv_sec;
  if (nsec < 0){
    --sec;
    nsec += 1000000000;
  }
  printf("answer by system() %llu\ntime elapsed: %lu seconds and %lu nanoseconds\n", count, sec, nsec);
  
  count = 0;
  clock_gettime(CLOCK_REALTIME, &start_time);
  for(int i = 0; i < 4; ++i){
    count += count_by_getc_single_thread(*(argv+1));
  }//this method appears to be quicker for files under 60k. perhaps an optimized version would take this even further.
  clock_gettime(CLOCK_REALTIME, &end_time);//thus a switching algorithm is likely to be optimal.
  count = count >> 2;
  nsec = end_time.tv_nsec - start_time.tv_nsec;
  sec = end_time.tv_sec - start_time.tv_sec;
  if (nsec < 0){
    --sec;
    nsec += 1000000000;
  }
  printf("answer by getc_single_thread() %llu\ntime elapsed: %lu seconds and %lu nanoseconds\n", count, sec, nsec);


  return 0;
}
