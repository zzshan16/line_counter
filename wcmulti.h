#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <sys/mman.h>
typedef struct Task {
  int length;
  int size;
  int i;
  char* buf;
  struct Task* next;
  struct Task* tail;
} Task;
void* count_line_multi(void* arg);
int first_attempt(char* filename);
void count_thread_multi_2(struct Task* task_head, int* soln_arr);
void *getTask(void* arg);
int second_attempt(char* filename);
int summation_function(int* soln_arr,int nums);
