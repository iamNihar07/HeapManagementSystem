# HeapManagementSystem
A customized implementation of C libraries of malloc, calloc and realloc interacting with the Operating System performing heap management using the strategies of First Fit, Next Fit, Best Fit and Worst Fit with splitting and coalescing of blocks

## Building and Runnning the code
The code compiles into four shared libraries and four test programs. To build the code, change to
your top level assignment directory and type: 

      make
Once you have the library, you can use it to override the existing malloc by using
LD_PRELOAD: 

      env LD_PRELOAD=lib/libmalloc-ff.so tests/test1

To run the other heap management schemes replace libmalloc-ff.so with the appropriate
library:


      Best-Fit: lib/libmalloc-bf.so
      First-Fit: lib/libmalloc-ff.so
      Next-Fit: lib/libmalloc-nf.so
      Worst-Fit: liblibmalloc-wf.so 
      
To use other tests replace test1 with the name of other test files, which are present in the tests folder.

