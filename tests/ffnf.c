#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main()
{
  char * ptr1 = ( char * ) malloc ( 1000 );
  char * buffer1 = (char*)malloc(24);
  char * ptr2 = ( char * ) malloc ( 6000 );
  char * buffer2 = (char*)malloc(24);
  char * ptr3 = ( char * ) malloc ( 1000 );

  printf("First fit should pick this one: %p\n", ptr1 );
  printf("Next fit should pick this one: %p\n", ptr3 );

  free( ptr1 ); 
  free( ptr2 ); 
  free( ptr3 ); 

  char * ptr5 = ( char * ) malloc ( 6000 );

  ptr5 = ptr5;

  char * ptr4 = ( char * ) malloc ( 1000 );
  printf("Chosen address: %p\n", ptr4 );
  free(buffer1);
  free(buffer2);
  free(ptr4);
  free(ptr2);
  return 0;
}

