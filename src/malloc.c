




#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define ALIGN4(s) (((((s)-1) >> 2) << 2) + 4)
#define BLOCK_DATA(b) ((b) + 1)
#define BLOCK_HEADER(ptr) ((struct _block *)(ptr)-1)

static int atexit_registered = 0;
static int num_mallocs = 0;
static int num_frees = 0;
static int num_reuses = 0;
static int num_grows = 0;
static int num_splits = 0;
static int num_coalesces = 0;
static int num_blocks = 0;
static int num_requested = 0;
static int max_heap = 0;

/*
 *  \brief printStatistics
 *
 *  \param none
 *
 *  Prints the heap statistics upon process exit.  Registered
 *  via atexit()
 *
 *  \return none
 */
void printStatistics(void)
{
   printf("\nheap management statistics\n");
   printf("mallocs:\t%d\n", num_mallocs);
   printf("frees:\t\t%d\n", num_frees);
   printf("reuses:\t\t%d\n", num_reuses);
   printf("grows:\t\t%d\n", num_grows);
   printf("splits:\t\t%d\n", num_splits);
   printf("coalesces:\t%d\n", num_coalesces);
   printf("blocks:\t\t%d\n", num_blocks);
   printf("requested:\t%d\n", num_requested);
   printf("max heap:\t%d\n", max_heap);
}

struct _block
{
   size_t size;         /* Size of the allocated _block of memory in bytes */
   struct _block *prev; /* Pointer to the previous _block of allcated memory   */
   struct _block *next; /* Pointer to the next _block of allcated memory   */
   bool free;           /* Is this _block free?                     */
   char padding[3];
};

struct _block *freeList = NULL;        /* Free list to track the _blocks available */
static struct _block *lastUsed = NULL; /* Pointer to store address of most recently used block for next fit algorithm */

/*
 * \brief findFreeBlock
 *
 * \param last pointer to the linked list of free _blocks
 * \param size size of the _block needed in bytes 
 *
 * \return a _block that fits the request or NULL if no free _block matches
 *
 * \TODO Implement Next Fit
 * \TODO Implement Best Fit
 * \TODO Implement Worst Fit
 */
struct _block *findFreeBlock(struct _block **last, size_t size)
{
   struct _block *curr = freeList;
   if (lastUsed == NULL)
   {
      lastUsed = freeList; //initializing lastUsed for first use in program
   }

#if defined FIT && FIT == 0
   /* First fit */
   while (curr && !(curr->free && curr->size >= size))
   {
      *last = curr;
      curr = curr->next;
   }
#endif

#if defined BEST && BEST == 0
   //printf("TODO: Implement best fit here\n");
   struct _block *temp = NULL;
   size_t current_optimum_size = 0; //setting size to 0 which will be taken care in first iteration

   while (curr != NULL)
   {
      if (curr->free && curr->size >= size)
      {
         if (current_optimum_size == 0) //takes care for size for first iteration.
         {
            temp = curr;
            current_optimum_size = curr->size;
         }
         if (curr->size < current_optimum_size) //if optimum size, consider it (best fit)
         {
            temp = curr;
            current_optimum_size = temp->size;
         }
      }
      *last = curr; //updating last value, otherwise causes error in allocating blocks
      curr = curr->next;
   }

   curr = temp;
#endif

#if defined WORST && WORST == 0
   //printf("TODO: Implement worst fit here\n");
   struct _block *temp = NULL;
   size_t current_worst_size = 0; //setting worst size to 0 for first use in iteration

   while (curr != NULL)
   {
      if (curr->free && curr->size >= size)
      {
         if (curr->size > current_worst_size) //if greater than worst size, set as worst size (worst fit)
         {
            temp = curr;
            current_worst_size = temp->size;
         }
      }

      curr = curr->next;
   }

   curr = temp;
#endif

#if defined NEXT && NEXT == 0
   //printf("TODO: Implement next fit here\n");
   /*curr = lastUsed;
   while(curr!=NULL && (curr->free == false || curr->size<size))
   {
      lastUsed = curr;
      curr= curr->next;
   }*/

   if (curr != NULL) //if curr is null, then last used will also be null for that case
   {
      curr = lastUsed->next; //but if curr is not null, then set our new curr as the next block of most recently used block
   }

   //borrowed from first fit algorithm with few changes
   while (curr && !(curr->free && curr->size >= size)) //curr has a new starting point if curr is not null
   {
      *last = curr;
      curr = curr->next;
      lastUsed = curr; //storing the location of most recently used block
   }

   /*struct _block *temp = NULL;
   curr=lastUsed; 
   while (curr != NULL)
   {
      if (curr->free && curr->size >= size)
      {
         temp = curr;
      }

      curr = curr->next;
   }

   curr = temp;
   lastUsed=curr;*/

#endif

   return curr;
}

/*
 * \brief growheap
 *
 * Given a requested size of memory, use sbrk() to dynamically 
 * increase the data segment of the calling process.  Updates
 * the free list with the newly allocated memory.
 *
 * \param last tail of the free _block list
 * \param size size in bytes to request from the OS
 *
 * \return returns the newly allocated _block of NULL if failed
 */
struct _block *growHeap(struct _block *last, size_t size)
{
   /* Request more space from OS */
   struct _block *curr = (struct _block *)sbrk(0);
   struct _block *prev = (struct _block *)sbrk(sizeof(struct _block) + size);

   assert(curr == prev);

   /* OS allocation failed */
   if (curr == (struct _block *)-1)
   {
      return NULL;
   }

   /* Update freeList if not set */
   if (freeList == NULL)
   {
      freeList = curr;
   }

   /* Attach new _block to prev _block */
   if (last)
   {
      last->next = curr;
   }

   /* Update _block metadata */
   curr->size = size;
   curr->next = NULL;
   curr->free = false;

   num_blocks++;
   num_grows++;
   max_heap = max_heap + size;

   return curr;
}

/*
 * \brief malloc
 *
 * finds a free _block of heap memory for the calling process.
 * if there is no free _block that satisfies the request then grows the 
 * heap and returns a new _block
 *
 * \param size size of the requested memory in bytes
 *
 * \return returns the requested memory allocation to the calling process 
 * or NULL if failed
 */
void *malloc(size_t size)
{

   if (atexit_registered == 0)
   {
      atexit_registered = 1;
      atexit(printStatistics);
   }

   /* Align to multiple of 4 */
   size = ALIGN4(size);

   /* Handle 0 size */
   if (size == 0)
   {
      return NULL;
   }

   /* Look for free _block */
   struct _block *last = freeList;
   struct _block *next = findFreeBlock(&last, size);

   /* TODO: Split free _block if possible */
   if (next != NULL) //found a free block, therefore reusing.
   {
      num_reuses++;
   }

   if (next != NULL && next->size - size > sizeof(struct _block))
   {
      //updating values
      num_splits++;
      num_blocks++;
      //storing required information before changing
      int actual_size = next->size;
      next->size = size;
      struct _block *actual_next = next->next;
      struct _block *new_next = (struct _block *)(((char *)BLOCK_DATA(next)) + size);
      
      next->next = new_next;
      next->next->free = true;
      next->next->size = actual_size - sizeof(struct _block) - size ;
      next->next->next = actual_next;
   }

   /* Could not find free _block, so grow heap */
   if (next == NULL)
   {
      next = growHeap(last, size);
   }

   /* Could not find free _block or grow heap, so just return NULL */
   if (next == NULL)
   {
      return NULL;
   }

   /* Mark _block as in use */
   next->free = false;

   num_mallocs++;
   num_requested = num_requested + size;

   /* Return data address associated with _block */
   return BLOCK_DATA(next);
}

void *calloc(size_t nmemb, size_t size)
{
   if (nmemb == 0)
   {
      return NULL;
   }
   if (size == 0)
   {
      return NULL;
   }

   void *ptr = malloc(nmemb * size);
   if (ptr == NULL)
   {
      return NULL;
   }

   struct _block *temp = BLOCK_HEADER(ptr);
   memset(BLOCK_DATA(temp), 0, temp->size);

   return BLOCK_DATA(temp);
}

void *realloc(void *ptr, size_t size) //not completed as of now
{
   //printf("%p \n",ptr);
   if (size == 0) //realloc can be used to free if size is 0
   {
      free(ptr);
      return NULL;
   }
   if (ptr == NULL) //realloc can also be used to malloc if ptr is NULL
   {
      return malloc(size);
   }

   /*
      If the above 2 cases are false then the command falls under the following 2 cases
         1. The realloc size is more than current size
         2. The realloc size is less than current size
   */
   struct _block *data = BLOCK_HEADER(ptr);

   //case 1
   if(size > data->size)
   {
      return malloc(size); //temp for now to avoid compilation warnings
   }
   //case 2
   else //since this is less size, we can just split the block into 2
   {
      //since splitting takes place here, I think we should increment splits. (NOT SURE)
      num_blocks++;
      //storing required information before changing
      int actual_size = data->size;
      data->size = size;
      struct _block *actual = data->next;
      struct _block *new = (struct _block *)(((char *)BLOCK_DATA(data)) + size);
      
      data->next = new;
      data->next->free = true;
      data->next->size = actual_size - sizeof(struct _block) - size ;
      data->next->next = actual;
      return BLOCK_DATA(data);
   }
   //since there is a return statement in all branches there is no error.
}

/*
 * \brief free
 *
 * frees the memory _block pointed to by pointer. if the _block is adjacent
 * to another _block then coalesces (combines) them
 *
 * \param ptr the heap memory to free
 *
 * \return none
 */
void free(void *ptr)
{
   if (ptr == NULL)
   {
      return;
   }

   /* Make _block as free */
   struct _block *curr = BLOCK_HEADER(ptr);
   assert(curr->free == 0);
   curr->free = true;


   /* Coalesce free _blocks if needed */
   struct _block *check = freeList;
   while (check != NULL)
   {
      if (check->next != NULL)
      {
         if ((check->free == true) && (check->next->free == true)) //check if the current block and next block is free.
         {
            //printf("Came to coalese.\n");
            //printf("size = %d, adding size = %d\n",check->size, check->next->size);
            check->free = true;
            check->size = check->size + sizeof(struct _block) + check->next->size ;
            struct _block *tempptr = check->next->next;

            //struct _block *curr = check->next; //freeing the memory of check->next as it is no longer needed as an individual block
            //assert(curr->free == 0);

            check->next = tempptr;

            num_coalesces++;
            num_blocks--;
         }
      }
      check = check->next;
   } 

   //Incrementing frees value
   num_frees++;
}

/* vim: set expandtab sts=3 sw=3 ts=6 ft=cpp: --------------------------------*/
