#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define FIT 0
#define ALIGN4(s)         (((((s) - 1) >> 2) << 2) + 4)
#define BLOCK_DATA(b)      ((b) + 1)
#define BLOCK_HEADER(ptr)   ((struct _block *)(ptr) - 1)


static int atexit_registered = 0;
static int num_mallocs       = 0;
static int num_frees         = 0;
static int num_reuses        = 0;
static int num_grows         = 0;
static int num_splits        = 0;
static int num_coalesces     = 0;
static int num_blocks        = 0;
static int num_requested     = 0;
static int max_heap          = 0;

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
void printStatistics( void )
{
  printf("\nheap management statistics\n");
  printf("mallocs:\t%d\n", num_mallocs );
  printf("frees:\t\t%d\n", num_frees );
  printf("reuses:\t\t%d\n", num_reuses );
  printf("grows:\t\t%d\n", num_grows );
  printf("splits:\t\t%d\n", num_splits );
  printf("coalesces:\t%d\n", num_coalesces );
  printf("blocks:\t\t%d\n", num_blocks );
  printf("requested:\t%d\n", num_requested );
  printf("max heap:\t%d\n", max_heap );
}

struct _block 
{
   size_t  size;         /* Size of the allocated _block of memory in bytes */
   struct _block *next;  /* Pointer to the next _block of allcated memory   */
   bool   free;          /* Is this _block free?                     */
   char   padding[3];
};


struct _block *freeList = NULL; /* Free list to track the _blocks available */
struct _block *l_used = NULL;
/*
 * \brief findFreeBlock
 *
 * \param last pointer to the linked list of free _blocks
 * \param size size of the _block needed in bytes 
 *
 * \return a _block that fits the request or NULL if no free _block matches
 *
 */
struct _block *findFreeBlock(struct _block **last, size_t size) 
{
   struct _block *curr = freeList;
   
#if defined FIT && FIT == 0
   /* First fit */
   while (curr && !(curr->free && curr->size >= size)) 
   {
      *last = curr;
      curr  = curr->next;
   }
#endif

#if defined BEST && BEST == 0
   //Implementation of best fit
   struct _block *result = NULL;
   size_t m = 0;
   while(curr)
   {
      if(curr->free && (curr->size >=size))
      {
         if(m == 0 || curr->size < m)
         {
            result = curr;
            m = curr->size;
         }
      }
      curr = curr->next;
   }
   curr = result;
#endif

#if defined WORST && WORST == 0
   //Implementation of worst fit
   struct _block *result = NULL;
   size_t m = 0;
   while(curr)
   {
      if(curr->free && (curr->size >=size))
      {
         if(curr->size > m)
         {
            result = curr;
            m = curr->size;
         }
      }
      curr = curr->next;
   }
   curr = result;
#endif

#if defined NEXT && NEXT == 0
   //Implementation of next fit
   curr = l_used;
   while (curr && !(curr->free && curr->size >= size)) 
   {
      l_used = curr;
      curr  = curr->next;
   }
   if(curr == NULL)
   {
      curr = freeList;
      while (curr && !(curr->free && curr->size >= size)) 
      {
         l_used = curr;
         curr  = curr->next;
      }
   }
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
      l_used = freeList;
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
   num_grows++;
   num_blocks++;
   max_heap+=size;
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

void *realloc(void *ptr, size_t size)
{
   if(ptr == NULL)
      return malloc(size);
   else if(size == 0)
   {
      free(ptr);
      return NULL;
   }
   struct _block *temp = BLOCK_HEADER(ptr);
   if(size > temp->size )
   {
      if(temp->next == NULL)
      {
         growHeap(temp,size- temp->size);
         temp->size = temp->size + temp->next->size;
         temp->next = NULL;
         temp->free = false;
         num_blocks--;
         num_requested+=(size - temp->size);
         return BLOCK_DATA(temp);
      }
      else
      {
         void *from_address = BLOCK_DATA(temp);
         struct _block* last = freeList;
         while(last->next != NULL)
            last = last->next;
         struct _block* new = growHeap(last,size);
         void *to_address = BLOCK_DATA(new);
         memcpy(to_address,from_address,temp->size);
         temp->free = true;
         return BLOCK_DATA(new);
      }
   }
   else
   {
      int original_size = temp->size;
      struct _block *org_next = temp->next;
      temp->size = size;
      temp->next = (struct _block*)(((char*)BLOCK_DATA(temp)) + size);
      temp->next->free = 1;
      temp->next->next = org_next;
      temp->next->size = original_size - size - sizeof(struct _block);
      num_blocks++;
      return BLOCK_DATA(temp);
   }
   
}

void *calloc(size_t nmemb,size_t size)
{
   void *ptr = malloc(nmemb*size);
   struct _block *new = BLOCK_HEADER(ptr);
   memset(BLOCK_DATA(new),0,new->size);
   return BLOCK_DATA(new);
}

void *malloc(size_t size) 
{
   num_requested+=size;
   if( atexit_registered == 0 )
   {
      atexit_registered = 1;
      atexit( printStatistics );
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
   if(next != NULL)
   {
      num_reuses++;
      int diff = next->size - size;
      if(diff> sizeof(struct _block))
      {
         int original_size = next->size;
         struct _block *org_next = next->next;
         next->size = size;
         next->next = (struct _block*)(((char*)BLOCK_DATA(next)) + size);
         next->next->free = 1;
         next->next->next = org_next;
         next->next->size = original_size - size - sizeof(struct _block);
         num_splits++;
         num_blocks++;
      }
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
   /* Return data address associated with _block */
   return BLOCK_DATA(next);
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
    num_frees++;


   //Coalesing blocks if needed.
   struct _block *temp = freeList;
   while(temp)
   {
      if(temp->free ==1 && temp->next && temp->next->free ==1)
      {
         temp->size = temp->size + temp->next->size + sizeof(struct _block);
         temp->next = temp->next->next;
         temp->free = 1;
         num_coalesces++;
         num_blocks--;
      }
      temp= temp->next;
   }
   
}

/* vim: set expandtab sts=3 sw=3 ts=6 ft=cpp: --------------------------------*/
