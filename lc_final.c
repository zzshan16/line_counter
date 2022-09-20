#include "lc_final.h"
/*
 * This is the result of this experiment.
 * This program uses a switching function to determine how to approach the question of counting non-empty lines.
 * This program seems to be very fast but it only accepts filenames as an input.
 * If I come back to this project, I would change it to permit input from stdin so that a user can pipe into this program
 * (e.g. "cat filename filename2 | ./lc_f")
 */
int SIZE_FACTOR=8;//page file size is multiplied by this number to determine size of task performed by each thread
Task* task_head;//global variable for the head of the linked list
void count_thread_multi_2(struct Task* task_in, int* soln_arr){
  int remaining = task_in->length;
  int position = task_in->i << 2;
  *(soln_arr+position) = 0;//count
  *(soln_arr+position+2) = 0;//carry
  *(soln_arr+position+1) = 0;//head
  if (remaining <= 0){
    return;
  }
  int last = -1;
  int count = 0;
  char* current_ptr = task_in->buf;
  while(last == -1){
    if (remaining--){
      switch(*current_ptr++){
      case '\n':
	*(soln_arr+position+1) = 1;//head
	last = 0;
	break;
      case ' ':
	break;
      default:
	last = 1;
      }
    }
    else{
      *(soln_arr+position+2) = 2;//special meaning --> carry from previous
      return;
    }
  }
  while(remaining--){
    switch(*current_ptr++){
    case '\n':
      count += last;
      last = 0;
      break;
    case ' ':
      break;
    default:
      last = 1;
    }
  }
  *(soln_arr+position) = count;
  *(soln_arr+position+2) = last;
  return;
}
void *get_task2(void* arg){
  pthread_mutex_t *mutex_p = *(void**)arg;
  int* soln_arr = *((void**)arg + 1);
  while(1){
    pthread_mutex_lock(mutex_p);
    while (task_head->size == 0){
      pthread_cond_wait(*((void**)arg+3), mutex_p);
    }
    Task* task_p = task_head;
    if (task_p->length < 0){
      pthread_mutex_unlock(mutex_p);
      pthread_cond_signal(*((void**)arg+2));
      pthread_cond_signal(*((void**)arg+3));
      return NULL;
    }
    if (!task_p->next || task_p->size == 1){
      Task *task_new = malloc(sizeof(Task));
      if(!task_new){
	puts("malloc failed");
      }
      task_new->size = 0;
      task_new->tail = task_new;
      task_head = task_new;
    }
    else{
      task_p->next->size = task_p->size - 1;
      task_p->next->tail = task_p->tail;
      task_head = task_p->next;
    }
    pthread_mutex_unlock(mutex_p);
    pthread_cond_signal(*((void**)arg+2));
    count_thread_multi_2(task_p,soln_arr);
    //if(munmap(task_p->buf,task_p->length)) puts("munmap error");/* slows down execution */
    free(task_p);
  }
  return NULL;
}

int summation_function(int* soln_arr,int nums){
  /*
   *Perhaps locality can be better exploited to speed up this function
   *Maybe summing up the counts, then computing the carries later would be quicker?
   */
  int sum = *(soln_arr);
  int carry = *(soln_arr+2);
  int i = 3;
  while(++i < (nums<<2ul)){
    switch(i & 3){
    case 0:
      sum += *(soln_arr+i);
      break;
    case 1:
      sum += *(soln_arr+i) & carry;
      break;
    case 2:
      if (*(soln_arr+i) & 2){
	break;
      }
      carry = *(soln_arr+i);
      break;
    case 3:;
    }
  }
  return sum;
}
int multi_m(int fd, size_t filesize){
  int sum;
  size_t size = sysconf(_SC_PAGESIZE);// == page size
  size = size * SIZE_FACTOR;
  int nums = (filesize+size-1)/size;// == total number of segments
  int* soln_arr = malloc(4*nums*sizeof(int));
  int i = 0;
  pthread_t *threads = malloc(8*sizeof(pthread_t));
  pthread_mutex_t mutex_task_q;
  pthread_cond_t task_q_full, task_q_empty;
  pthread_cond_init(&task_q_full,NULL);
  pthread_cond_init(&task_q_empty,NULL);
  pthread_mutex_init(&mutex_task_q,NULL);
  task_head = malloc(sizeof(Task));
  task_head->size = 0;
  task_head->tail = task_head;
  void** arr = malloc(4*sizeof(void*));
  *(arr) = &mutex_task_q;
  *(arr+1) = soln_arr;
  *(arr+2) = &task_q_full;
  *(arr+3) = &task_q_empty;
  for(int i = 0; i < 8; ++i){
    pthread_create(threads+i, NULL, get_task2, arr);
  }
  char* map_p = mmap(NULL, filesize, PROT_READ, MAP_SHARED, fd, 0);
  while(filesize > size){
    pthread_mutex_lock(&mutex_task_q);
    while (task_head->size > 15){//limit tasks in queue
      pthread_cond_wait(&task_q_full, &mutex_task_q);
    }
    if (task_head->size == 0){
      task_head->size = 1;
      task_head->buf = map_p+i*size;
      task_head->length = size;
      task_head->i = i;
    }
    else{
      (task_head->size)++;
      Task *new_task = malloc(sizeof(Task));
      task_head->tail->next = new_task;
      task_head->tail = new_task;
      new_task->buf = map_p+i*size;
      new_task->length = size;
      new_task->i = i;
      new_task->size = 0;
      task_head->tail = new_task;
    }
    pthread_mutex_unlock(&mutex_task_q);
    pthread_cond_signal(&task_q_empty);
    filesize -= size;
    i++;
  }
  if(filesize){
    pthread_mutex_lock(&mutex_task_q);
    while (task_head->size > 15){//limit tasks in queue
      pthread_cond_wait(&task_q_full, &mutex_task_q);
    }
    if (task_head-> size == 0){
      task_head->size = 1;
      task_head->buf = map_p + i*size;
      task_head->length = filesize;
      task_head->i = i;
    }
    else{
      (task_head->size)++;
      Task *new_task = malloc(sizeof(Task));
      task_head->tail->next = new_task;
      task_head->tail = new_task;
      new_task->buf = map_p+i*size;
      new_task->length = filesize;
      new_task->i = i;
      new_task->size = 0;
      task_head->tail = new_task;
    }
    pthread_mutex_unlock(&mutex_task_q);
    pthread_cond_broadcast(&task_q_empty);//signal?
    filesize =0;
  }
  pthread_mutex_lock(&mutex_task_q);
  if (task_head-> size == 0){
    task_head->size = 1;
    task_head->length = -1;
  }
  else{
    Task *new_task = malloc(sizeof(Task));
    (task_head->size)++;
    task_head->tail->next = new_task;
    task_head->tail = new_task;
    new_task->length = -1;
    new_task->size = 0;
  }
  pthread_mutex_unlock(&mutex_task_q);
  pthread_cond_signal(&task_q_empty);
  for(int i = 0; i < 8; ++i){
    pthread_join(*(threads+i), NULL);
  }
  free(task_head);
  pthread_mutex_destroy(&mutex_task_q);
  pthread_cond_destroy(&task_q_empty);
  pthread_cond_destroy(&task_q_full);
  sum = summation_function(soln_arr,nums);
  munmap(map_p,filesize);
  free(threads);
  free(soln_arr);
  free(arr);
  return sum;  
}

int count_by_mmap(int fd, size_t filesize){
  char* map_p = mmap(NULL, filesize, PROT_READ, MAP_SHARED, fd, 0);
  size_t offset = 0;
  int count = 0;
  int blank = 0;
  while(offset < filesize){
    switch(*(map_p+offset++)){
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
  munmap(map_p,filesize);
  return count;
}
int count_by_read(int fd, size_t filesize){
  char* buf = malloc(filesize);
  read(fd, buf, filesize);
  size_t offset = 0;
  int count = 0;
  int blank = 0;
  while(offset < filesize){
    switch(*(buf+offset++)){
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
  free(buf);
  return count;  
}
int switching_function(char* filename){
  struct stat statbuf;
  int fd = open(filename, O_RDONLY);
  fstat(fd, &statbuf);
  size_t filesize = statbuf.st_size;
  int count;
  if (filesize < 8192){
    count = count_by_read(fd, filesize);
  }
  else if (filesize < 300000){
    count = count_by_mmap(fd, filesize);
  }
  else{
    count = multi_m(fd,filesize);
  }
  
  close(fd);
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
  printf("%d\n", switching_function(*(argv+1)));
  
  return 0;
}
