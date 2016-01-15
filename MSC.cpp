#include "helper.h"                               
  template <class T>
  class ALIGNEDMA {
  public:
      void* operator new(size_t);        override new
      void operator delete(void*);       override delete
  };
  
  template <class T> 
  void * ALIGNEDMA<T>::operator new(size_t sz){
  	return _aligned_malloc(sz,lineSz);
  }
  
  template <class T> 
  void  ALIGNEDMA<T>::operator delete(void *p){
  	_aligned_free(p);
  }
  
  class QNode : public ALIGNEDMA<QNode> {
  	public:
  		volatile int waiting;
  		volatile QNode *next;
  };
  
  class MCS {
  public:
  	DWORD myTLSIndex;
  	inline void acquire(QNode ** lock, DWORD myTLSIndex){
  		volatile QNode * i = (QNode*) TlsGetValue(myTLSIndex);
  		i->next = NULL;
  		volatile QNode * pred = (QNode*) InterlockedExchangePointer((PVOID*) lock, (PVOID) i);
  		if (pred == NULL) return;
  		i->waiting = 1;
  		pred->next = i;
  		   the program needs to sleep when there are more threads than CPU's
  		while (i->waiting) Sleep(0); 
  	}
  
  	inline void release(QNode ** lock, DWORD myTLSIndex){
  		volatile QNode *i = (QNode*) TlsGetValue(myTLSIndex);
  		volatile QNode *succ;
  		if (!(succ = i->next)) {
  			if (InterlockedCompareExchangePointer((PVOID*)lock, NULL, (PVOID) i) == i)
  				return;
  			do {
  				succ = i->next;
  			} while(!succ);
  		}
  		succ->waiting = 0;
  		   the program needs to sleep when there are more threads than CPU's
  		Sleep(0);
  	}
  
  };