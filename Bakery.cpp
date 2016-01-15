#include <intrin.h>
class Bakery {

public:
  volatile long * choosing;
  volatile long * ticket;
  int size;

  Bakery(int numOfThreads) :
    choosing(new volatile long[numOfThreads]),
    ticket(new volatile long[numOfThreads]){
    size = numOfThreads;
  };


  inline void release(int id) {
    ticket[id] = 0;
    _mm_mfence();
  }

  inline void acquire(int id) {
    choosing[id] = 1;
    int max = 0;
    for (int i = 0; i < size; i++) 
      if (ticket[i] > max) max = ticket[i];
    ticket[id] = max + 1;
    choosing[id] = 0;
    _mm_mfence();
    for (int j = 0; j < size; j++) {
      while (choosing[j]);
      while ((ticket[j] != 0) && ((ticket[j] < ticket[id]) || ((ticket[j] == ticket[id]) && (j < id))));
    }
  }
};