#include "mm.h"

/* Input: timeval, timeval (start and end time of execution)
 * Output: double (difference between start and end time)
 *
 * Calculates the difference between the start and the
 * end time, then returns the double of 1000000.0 time
 * the time elapsed */
double comp_time(struct timeval time_s, struct timeval time_e) {

  double elap = 0.0;

  if (time_e.tv_sec > time_s.tv_sec) {
    elap += (time_e.tv_sec - time_s.tv_sec - 1) * 1000000.0;
    elap += time_e.tv_usec + (1000000 - time_s.tv_usec);
  }
  else {
    elap = time_e.tv_usec - time_s.tv_usec;
  }
  return elap;
}

/* Input: mm_t, int (memory allocation object, and int to push)
 * Output: none
 * 
 * Pushes int d to the stack */
void push(mm_t *mm, int d){
  if(mm->stacksize < mm->numChunks)
    mm->stackOfUnused[mm->stacksize++] = d;
  else
    fprintf(stderr, "Error: Stack is full\n");
}

/* Input: mm_t (dynamic memory allocation object)
 * Output: int (index of unused chunk of memory)
 * 
 * Pops the last int d from the stack, d will be an index of an unused memory */
int pop(mm_t *mm){
  if(mm->stacksize == 0){
    fprintf(stderr, "Error: Stack is empty\n");
    return -1;
  }else{
    // return the index of unused chunk
    return mm->stackOfUnused[--mm->stacksize];
  }
}

/* Input: mm_t, int, int (dynamic mem alloc obj, numChunks, chunkSize)
 * Output: int (0 if successful, -1 if not successful)
 *
 * Initializes the dynamic memory allocation block of memory */
int mm_init(mm_t *mm, int hm, int sz) {
  int i;
  if( (mm->memory = calloc(hm,sz)) == NULL){
    perror("calloc: Cannot allocate memory for mm_init.");
    return -1;
  }
    
  if( (mm->chunkAvailable = (bool* )calloc(hm,sizeof(bool))) == NULL){
    free(mm->memory);
    perror("calloc: Cannot allocate memory for array.");
    return -1;
  }
  mm->chunkSize = sz;
  mm->numChunks = hm;
  mm->stackOfUnused = (int* )calloc(hm,sizeof(int));
  mm->stacksize = 0;
  for ( i = hm -1 ; i >= 0; i--){
    mm->chunkAvailable[i] = true;
    mm->stackOfUnused[i] = i;
    push(mm,i);
  }
  return 0;  
}

/* Input: mm_t (dynamic memory allocation object)
 * Output: void pointer to memory available address
 *
 * Gets the appropriate chunk of unused memory of size chunkSize 
 */
void *mm_get(mm_t *mm) {
  int index = pop(mm);
  if(mm->chunkAvailable[index]){
    mm->chunkAvailable[index] = false;
    return &mm->memory[index * (mm->chunkSize)];
  }else return NULL;
}

/* 
 * Puts the desired chunk back into the available memory */
void mm_put(mm_t *mm, void *chunk) {
  long tmp = chunk - &(*mm->memory);
  // checking if tmp's address belongs in the mm->memory
  if (!(tmp % mm->chunkSize)){ 
    tmp = tmp / mm->chunkSize;
    if (!((tmp < mm->numChunks) &&(tmp >= 0))){
      return;
    }else{
      if(mm->chunkAvailable[tmp]){
        fprintf(stderr, "Error:mm_put line 77 reported a free chunk to mm\n");
        return;
      }else{
  // return the chunk to mm
        push(mm,tmp);
        mm->chunkAvailable[tmp] = true;
        return;
      }
    }
  }else{
    fprintf(stderr, "Error:mm_put was given an invalid address.\n");
    return;
  }
}

/* 
 * Releases dynamic memory object at the very end of the process */
void mm_release(mm_t *mm) {
  free(mm->memory);
  free(mm->chunkAvailable);
  free(mm->stackOfUnused);
return;
}
