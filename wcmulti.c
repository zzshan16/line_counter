#include "wcmulti.h"
/*
 * Initial attempts to find a quick multi-threaded line counting algorithm
 * The efficiency improved in each attempt
 * The first attempt requires that the entire file be loaded into memory.
 * The subsequent attempts remove this requirement.
 */
Task* task_head;//global variable for the head of the linked list
const int SIZE_FACTOR = 8;
void* count_line_multi(void* arg){
  int remaining = *(int*)*((void**)arg+1);
  if (remaining <= 0){
    **(long int**)((void**)arg+2) = 0;//sum
    *(*(long int**)((void**)arg+2)+1) = 0;//carry
    *((long int*)(*((void**)arg+2)) +2) = 0;//head
    return arg;
  }
  *(long int*)(*((void**)arg+2)) = 0;
  *((long int*)(*((void**)arg+2)) +2) = 0;
  int last = -1;
  int count = 0;
  char* current_ptr = *(char**)(arg);
  while(last == -1){
    if (remaining--){
      switch(*current_ptr++){
      case '\n':
	*((long int*)*((void**)arg+2)+2) = 1;
	last = 0;
	break;
      case ' ':
	break;
      default:
	last = 1;
      }
    }
    else{
      **(long int**)((void**)arg+2) = 0;
      *(*(long int**)((void**)arg+2)+1) = 2;//special meaning assigned to value of 2
      return arg;
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
  **(long int**)((void**)arg+2) = count;
  *(*(long int**)((void**)arg+2)+1) = last;
  return arg;
}

int first_attempt(char* filename){
  struct stat statbuf;
  int fd = open(filename, O_RDONLY);
  fstat(fd, &statbuf);
  size_t filesize = statbuf.st_size;
  size_t size_part = (filesize + 7) / 8;
  struct iovec* char_buffers = malloc(8*sizeof(struct iovec));
  for (int i = 0; i < 8; ++i){
    (char_buffers+i)->iov_base = malloc(size_part*sizeof(char));//may allocate a very large amount of memory
    (char_buffers+i)->iov_len = size_part;
  }
  readv(fd, char_buffers, 8);
  close(fd);
  
  pthread_t *threads = malloc(8*sizeof(pthread_t));
  void ***arr = malloc(8*sizeof(void**));
  for (int i = 0; i < 8; ++i){
    *(arr+i) = malloc(3*sizeof(void*));
    *(*(arr+i)+1) = malloc(sizeof(int));
    *(*(arr+i)+2) = malloc(3*sizeof(long int));
    if (filesize >= size_part){
      *(int*)*(*(arr+i)+1) = size_part;
      filesize = filesize - size_part;
    }
    else {
      *(int*)*(*(arr+i)+1) = filesize;
      filesize = 0u;
    }
    *(char**)*(arr+i) = (char_buffers+i)->iov_base;
    if (pthread_create(threads+i, NULL, count_line_multi, *(arr+i)) != 0){
      puts("failed to create thread");
      exit(2);
    }
  }
  int sum = 0;
  int carry = 0;
  for (int i = 0; i < 8; ++i){
    pthread_join(*(threads+i), NULL);
    sum += **(long int**)(*(arr+i)+2);
    if (i == 0){
      carry = *(*(long int**)(*(arr+i)+2)+1);
    }
    else{
      if (*(*(long int**)(*(arr+i)+2)+2) == 2){
	continue;
      }
      sum += *(*(long int**)(*(arr+i)+2)+2) & carry;
      carry = *(*(long int**)(*(arr+i)+2)+1);
    }
  }
  for (int i = 0; i < 8; ++i){
    free(*(*(arr+i)+2));
    free(*(*(arr+i)+1));
    free(*(arr+i));
    free((char_buffers+i)->iov_base);
  }
  free(threads);
  free(arr);
  free(char_buffers);
  return sum;
}
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
  //printf("count and carry: %d %d\n", count, last);
  return;
}
void *get_task(void* arg){
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
    if(munmap(task_p->buf,task_p->length)) puts("munmap error");
    free(task_p);
  }
  return NULL;
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
int second_attempt(char* filename){
  int sum;
  const int factor = 4;
  struct stat statbuf;
  int fd = open(filename, O_RDONLY);
  fstat(fd, &statbuf);
  size_t filesize = statbuf.st_size;
  size_t size = sysconf(_SC_PAGESIZE);// == page size
  size = size << factor;
  char** addr_arr = malloc(filesize *sizeof(char*) / size + 1);
  int nums = (filesize+size-1)/size;// == total number of segments
  //printf("length of soln_arr = %d\n", 4*nums);
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
    pthread_create(threads+i, NULL, get_task, arr);
  }
  while(filesize > size){
    *(addr_arr+i) = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, i * size);
    pthread_mutex_lock(&mutex_task_q);
    while (task_head->size > 15){
      //puts("too many tasks; waiting...");
      pthread_cond_wait(&task_q_full, &mutex_task_q);
    }
    if (task_head->size == 0){
      task_head->size = 1;
      task_head->buf = *(addr_arr+i);
      task_head->length = size;
      task_head->i = i;
      //printf("created task %d\n", i);
    }
    else{
      (task_head->size)++;
      //printf("incremented size to %d\n", task_head->size);
      Task *new_task = malloc(sizeof(Task));
      task_head->tail->next = new_task;
      task_head->tail = new_task;
      new_task->buf = *(addr_arr+i);
      new_task->length = size;
      new_task->i = i;
      new_task->size = 0;
      //printf("created task %d\n", i);
      task_head->tail = new_task;
    }
    pthread_mutex_unlock(&mutex_task_q);
    pthread_cond_signal(&task_q_empty);
    filesize -= size;
    i++;
  }
  if(filesize){
    *(addr_arr+i) = mmap(NULL, filesize, PROT_READ, MAP_SHARED, fd, i * size);
    pthread_mutex_lock(&mutex_task_q);
    while (task_head->size > 15){
      pthread_cond_wait(&task_q_full, &mutex_task_q);
    }
    if (task_head-> size == 0){
      task_head->size = 1;
      task_head->buf = *(addr_arr+i);
      task_head->length = filesize;
      task_head->i = i;
    }
    else{
      (task_head->size)++;
      Task *new_task = malloc(sizeof(Task));
      task_head->tail->next = new_task;
      task_head->tail = new_task;
      new_task->buf = *(addr_arr+i);
      new_task->length = filesize;
      new_task->i = i;
      new_task->size = 0;
      task_head->tail = new_task;
    }
    pthread_mutex_unlock(&mutex_task_q);
    pthread_cond_broadcast(&task_q_empty);//signal?
    filesize =0;
  }
  close(fd);
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
  free(threads);
  free(addr_arr);
  free(soln_arr);
  free(arr);
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
    while (task_head->size > 15){
      //puts("too many tasks; waiting...");
      pthread_cond_wait(&task_q_full, &mutex_task_q);
    }
    if (task_head->size == 0){
      task_head->size = 1;
      task_head->buf = map_p+i*size;
      task_head->length = size;
      task_head->i = i;
      //printf("created task %d\n", i);
    }
    else{
      (task_head->size)++;
      //printf("incremented size to %d\n", task_head->size);
      Task *new_task = malloc(sizeof(Task));
      task_head->tail->next = new_task;
      task_head->tail = new_task;
      new_task->buf = map_p+i*size;
      new_task->length = size;
      new_task->i = i;
      new_task->size = 0;
      //printf("created task %d\n", i);
      task_head->tail = new_task;
    }
    pthread_mutex_unlock(&mutex_task_q);
    pthread_cond_signal(&task_q_empty);
    filesize -= size;
    i++;
  }
  if(filesize){
    pthread_mutex_lock(&mutex_task_q);
    while (task_head->size > 15){
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
  close(fd);
}
int third_attempt(char* filename){
  struct stat statbuf;
  int fd = open(filename, O_RDONLY);
  fstat(fd, &statbuf);
  size_t filesize = statbuf.st_size;
  return multi_m(fd, filesize);
}
int switching_function(char* filename){
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
  struct timespec start_time, end_time;
  long signed int nsec;
  long unsigned int sec;
  clock_gettime(CLOCK_MONOTONIC, &start_time);
  for(int i = 0; i < 4; ++i){
    count += first_attempt(*(argv+1));
  }
  clock_gettime(CLOCK_MONOTONIC, &end_time);
  count = count >> 2;
  nsec = end_time.tv_nsec - start_time.tv_nsec;
  sec = end_time.tv_sec - start_time.tv_sec;
  if (nsec < 0){
    --sec;
    nsec += 1000000000;
  }
  printf("answer by first multithread method %llu\ntime elapsed: %lu seconds and %ld nanoseconds\n", count, sec, nsec);
  count = 0;
  clock_gettime(CLOCK_MONOTONIC, &start_time);
  for(int i = 0; i < 4; ++i){
    count += second_attempt(*(argv+1));
  }
  clock_gettime(CLOCK_MONOTONIC, &end_time);
  count = count >> 2;
  nsec = end_time.tv_nsec - start_time.tv_nsec;
  sec = end_time.tv_sec - start_time.tv_sec;
  if (nsec < 0){
    --sec;
    nsec += 1000000000;
  }
  printf("answer by second multithread method %llu\ntime elapsed: %lu seconds and %ld nanoseconds\n", count, sec, nsec);

  count = 0;
  clock_gettime(CLOCK_MONOTONIC, &start_time);
  for(int i = 0; i < 4; ++i){
    count += third_attempt(*(argv+1));
  }
  clock_gettime(CLOCK_MONOTONIC, &end_time);
  count = count >> 2;
  nsec = end_time.tv_nsec - start_time.tv_nsec;
  sec = end_time.tv_sec - start_time.tv_sec;
  if (nsec < 0){
    --sec;
    nsec += 1000000000;
  }
  printf("answer by third multithread method %llu\ntime elapsed: %lu seconds and %ld nanoseconds\n", count, sec, nsec);
  
  return 0;
}
