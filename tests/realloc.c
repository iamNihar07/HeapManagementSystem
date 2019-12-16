#include <stdlib.h>
#include <stdio.h>

int main()
{

  printf("Running test 2 to exercise malloc and free\n");
  char *ptr = (char*) malloc (1000);
  ptr = realloc(ptr,2000);
  free(ptr);
  char *ptr2 = (char*)malloc(1500);
  free(ptr2);
  return 0;
}
