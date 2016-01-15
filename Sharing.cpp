#include "stdafx.h"                             // pre-compiled headers
#include <iostream>                             // cout
#include <iomanip>                              // setprecision
#include "helper.h"                             //
#include "MSC.cpp"                              //
#include "RandomNumberGenerator.hpp"

using namespace std;                            // cout

#define RTM                     3
#define BST_LOCK                9
#define BST_NO_LOCK             12
#define BST_HLE			  13

#define START                   0
#define MAX                     5
#define NUM_OF_NODES            10

#define HIGH_OPERATION          1

volatile long lock = 0;
int id = 0;
int maxThread;                                      // max # of threads
DWORD tlsIndex;
RandomNumberGenerator aOrRDist = RandomNumberGenerator(HIGH_OPERATION,START);

class Node {
    public:
        UINT64 volatile key;
        Node * volatile left;
        Node * volatile right;
        Node(UINT64 value) {key = value; right = left = NULL;} // default constructor
};

class BST {
public:
    Node * volatile root; // root of BST, initially NULL
    int volatile numberOfNodes;

    BST(){
        root = NULL;
        numberOfNodes = 1;
    }

    // checks to see if the tree is empty
    bool empty(){ return numberOfNodes == 0 && root == NULL; }

    // Returns the number of nodes in the tree
    int size(){ return numberOfNodes; }

    // Adds a node from the BST
    int BST::add (Node * n){
        if(root == NULL) root = n;
        else{
            Node * volatile * pp = &root;
            Node * volatile p = root;
            while (p) {
                if(p == NULL) break;
                if (n -> key < p -> key) pp = &p -> left;
                else if (n -> key > p -> key) pp = &p -> right;
                else return 0;
                p = *pp;
            }
            *pp = n;
        }
        numberOfNodes++;
        return 1;
    }

   // Removes a node from the BST
   Node volatile * BST::remove(UINT64 key){
       if(root == NULL) return NULL;
       else{
          Node  * volatile * pp = &root;
          Node  * volatile p = root;
          while (p) {
              if (key < p -> key) pp = &p -> left;
              else if (key > p -> key) pp = &p -> right;
              else break;
              p = *pp;
          }
          if(p == NULL) return NULL;
          if (p -> left == NULL && p -> right == NULL) *pp = NULL; // NO children
          else if (p -> left == NULL) *pp = p -> right; // ONE child
          else if (p -> right == NULL) *pp = p -> left; // ONE child
          else {
              Node * volatile r = p->right; // TWO children
              Node * volatile * ppr = &p->right; // find min key in right sub tree
              while (r != NULL && r -> left != NULL) {
                  ppr = &r -> left;
                  r = r -> left;
              }
		    if (r != NULL) {
			    p->key = r->key; // could move...
			    p = r; // node instead
			    *ppr = r->right;
		    }
              
          }
          numberOfNodes--;
          return p; // return removed node
       }
      
    }

    
};

UINT64 addOrRemove(){ return aOrRDist.createRandomNumber(); }

void prefillTree(BST theTree, int high){
    RandomNumberGenerator myGen = RandomNumberGenerator(high,START);
    vector<UINT64> random;
    int i;
    for(i = myGen.low ; i < high; i++) random.push_back(myGen.createRandomNumber());

    for(i = 0; i < random.size() ; i++) {
        Node * volatile node = new Node(random[i]);
        theTree.add(node);
    }
}

BST bstTree = BST();


int ranges[] = {16, 256, 4096, 65536, 1048576};

#define K                    1024                        //
#define GB                   (K * K * K)                 //
#define NOPS                 10000                       //
#define NSECONDS             2                           // run each test for NSECONDS

#define COUNTER64            1					  // comment for 32 bit counter

#ifdef COUNTER64
#define VINT                 UINT64                          //  64 bit counter
#else
#define VINT                 UINT                            //  32 bit counter
#endif

#define ALIGNED_MALLOC(sz, align) _aligned_malloc(sz, align)

#ifdef FALSESHARING
#define GINDX(n)            (g+n)                       //
#else
#define GINDX(n)            (g+n*lineSz/sizeof(VINT))   //
#endif

#define OPTYP      BST_HLE                      // set op type
										  // 10 : BST lockless
										  // 11 : BST RTM 
										  // 12 : BST no Lock


UINT64 tstart;                                  // start of test in ms
int range;                                      // % range
int lineSz;                                     // cache line size

THREADH *threadH;                               // thread handles
UINT64 *ops;                                    // for ops per thread

#if OPTYP == 3
UINT64 *aborts;                                 // for counting aborts
#endif

typedef struct {
    int range;                                  // range
    int nt;                                     // # threads
    UINT64 rt;                                  // run time (ms)
    UINT64 ops;                                 // ops
    UINT64 incs;                                // should be equal ops
    UINT64 aborts;                              //
} Result;

Result *r;                                      // results
UINT indx;                                      // results index

volatile VINT *g;                               // NB: position of volatile

                                                // test memory allocation [see lecture notes]
ALIGN(64) UINT64 cnt0;
ALIGN(64) UINT64 cnt1;
ALIGN(64) UINT64 cnt2;
UINT64 cnt3;                                    // NB: in Debug mode allocated in cache line occupied by cnt0


// worker
WORKER worker(void *vthread) {
	int thread = (int)((size_t)vthread);
	int id = thread;
	UINT64 n = 0;

	RandomNumberGenerator myGenerator = RandomNumberGenerator(ranges[range], 0);

#if OPTYP == 2
	VINT x;
#elif OPTYP == 3
	UINT64 nabort = 0;
#endif

	while (1) {
		switch (OPTYP) {
			   case BST_NO_LOCK:
			       // create the numbers
			       //for(int i = 0 ; i < ranges[range]; i++){
					  UINT64 val = myGenerator.createRandomNumber();
			            if( addOrRemove() == 0){
			               Node  * volatile node = new Node(val);
			               bstTree.add(node);
			            }
			            else bstTree.remove(val);
			       //}
			       break;
		   //     case BST_LOCK:
				 // 
				 // //for (int i = 0; i < ranges[range]; i++) {
					//UINT64 val = myGenerator.createRandomNumber();
					//while (InterlockedExchange(&lock, 1))								
 	   // 				while (lock == 1) _mm_pause();
					//if (addOrRemove() == 0) {
					//	Node  * volatile node = new Node(val);
					//	bstTree.add(node);
					//}
					//else bstTree.remove(val);
					//lock = 0;
				 // //}
				 // break;
	   //  case RTM:
		  //   UINT64 val = myGenerator.createRandomNumber();
			 //int status = _xbegin();
			 //if (status == _XBEGIN_STARTED) { // our transaction was started succesfully
			 //	if (addOrRemove() == 0) {
			 //		Node  * volatile node = new Node(val);
			 //		bstTree.add(node);
			 //	}
			 //	else bstTree.remove(val);
			 //	_xend();
			 //}
			 //else { // this is the abort transaction code
			 /*	while (InterlockedExchange(&lock, 1))
					 while (lock == 1) _mm_pause();
				 if (addOrRemove() == 0) {
					 Node  * volatile node = new Node(val);
					 bstTree.add(node);
				 }
				 else bstTree.remove(val);
				 lock = 0;
			 }*/
			 //break;
		//case BST_HLE:
		//	UINT64 val = myGenerator.createRandomNumber();

		//	while (_InterlockedExchange_HLEAcquire(&lock, 1) == 1) { // wait for transaction to begin.
		//		do { _mm_pause(); }
		//		while (lock == 1); // abort the transaction
		//		if (addOrRemove() == 0) {
		//			Node  * volatile node = new Node(val);
		//			bstTree.add(node);
		//		}
		//		else bstTree.remove(val);
		//	}
		//	_Store_HLERelease(&lock, 0); // stop the transaction
		//	break;
		}
		// check if runtime exceeded
		if ((getWallClockMS() - tstart) > NSECONDS * 1000) break;
		n += NOPS;
	}
    ops[thread] = n;
#if OPTYP == 3
    aborts[thread] = nabort;
#endif
    return 0;

}


// main
int main(){
    ncpu = getNumberOfCPUs();   // number of logical CPUs
    maxThread = 2 * ncpu;       // max number of threads
    
    // get date
    char dateAndTime[256];
    getDateAndTime(dateAndTime, sizeof(dateAndTime));

    // console output
    cout << getHostName() << " " << getOSName() << " range " << (is64bitExe() ? "(64" : "(32") << "bit EXE)";
#ifdef _DEBUG
    cout << " DEBUG";
#else
    cout << " RELEASE";
#endif
   // cout << " [" << OPSTR << "]" << " NCPUS=" << ncpu << " RAM=" << (getPhysicalMemSz() + GB - 1) / GB << "GB " << dateAndTime << endl;
#ifdef COUNTER64
    cout << "COUNTER64";
#else
    cout << "COUNTER32";
#endif
#ifdef FALSESHARING
    cout << " FALSESHARING";
#endif
    cout << " NOPS=" << NOPS << " NSECONDS=" << NSECONDS << " OPTYP=" << OPTYP;
#ifdef USEPMS
    cout << " USEPMS";
#endif
    cout << endl;
    cout << "Intel" << (cpu64bit() ? "64" : "32") << " family " << cpuFamily() << " model " << cpuModel() << " stepping " << cpuStepping() << " " << cpuBrandString() << endl;
#ifdef USEPMS
    cout << "performance monitoring version " << pmversion() << ", " << nfixedCtr() << " x " << fixedCtrW() << "bit fixed counters, " << npmc() << " x " << pmcW() << "bit performance counters" << endl;
#endif

    // get cache info
    lineSz = getCacheLineSz();
    //lineSz *= 2;

    if ((&cnt3 >= &cnt0) && (&cnt3 < (&cnt0 + lineSz / sizeof(UINT64)))) cout << "Warning: cnt3 shares cache line used by cnt0" << endl;
    if ((&cnt3 >= &cnt1) && (&cnt3 < (&cnt1 + lineSz / sizeof(UINT64)))) cout << "Warning: cnt3 shares cache line used by cnt1" << endl;
    if ((&cnt3 >= &cnt2) && (&cnt3 < (&cnt2 + lineSz / sizeof(UINT64)))) cout << "Warning: cnt2 shares cache line used by cnt1" << endl;

#if OPTYP == RTM

    // check if RTM supported
    if (!rtmSupported()) {
        cout << "RTM (restricted transactional memory) NOT supported by this CPU" << endl;
        quit();
        return 1;
    }

#endif

    cout << endl;

    // allocate global variable
    // NB: each element in g is stored in a different cache line to stop false range
    threadH = (THREADH*)ALIGNED_MALLOC(maxThread*sizeof(THREADH), lineSz);             // thread handles
    ops = (UINT64*)ALIGNED_MALLOC(maxThread*sizeof(UINT64), lineSz);                   // for ops per thread

#if OPTYP == 3
    aborts = (UINT64*)ALIGNED_MALLOC(maxThread*sizeof(UINT64), lineSz);                // for counting aborts
#endif

#ifdef FALSESHARING
    g = (VINT*)ALIGNED_MALLOC((maxThread + 1)*sizeof(VINT), lineSz);                     // local and shared global variables
#else
    g = (VINT*)ALIGNED_MALLOC((maxThread + 1)*lineSz, lineSz);                         // local and shared global variables
#endif

#ifdef USEPMS

    fixedCtr0 = (UINT64*)ALIGNED_MALLOC(5 * maxThread*ncpu*sizeof(UINT64), lineSz);      // for fixed counter 0 results
    fixedCtr1 = (UINT64*)ALIGNED_MALLOC(5 * maxThread*ncpu*sizeof(UINT64), lineSz);      // for fixed counter 1 results
    fixedCtr2 = (UINT64*)ALIGNED_MALLOC(5 * maxThread*ncpu*sizeof(UINT64), lineSz);      // for fixed counter 2 results
    pmc0 = (UINT64*)ALIGNED_MALLOC(5 * maxThread*ncpu*sizeof(UINT64), lineSz);           // for performance counter 0 results
    pmc1 = (UINT64*)ALIGNED_MALLOC(5 * maxThread*ncpu*sizeof(UINT64), lineSz);           // for performance counter 1 results
    pmc2 = (UINT64*)ALIGNED_MALLOC(5 * maxThread*ncpu*sizeof(UINT64), lineSz);           // for performance counter 2 results
    pmc3 = (UINT64*)ALIGNED_MALLOC(5 * maxThread*ncpu*sizeof(UINT64), lineSz);           // for performance counter 3 results

#endif

    r = (Result*)ALIGNED_MALLOC(5 * maxThread*sizeof(Result), lineSz);                   // for results
    memset(r, 0, 5 * maxThread*sizeof(Result));                                           // zero

    indx = 0;

#ifdef USEPMS
    // set up performance monitor counters
    setupCounters();
#endif

    // use thousands comma separator
    setCommaLocale();

    // header
    cout << "range";
    cout << setw(4) << "nt";
    cout << setw(6) << "rt";
    cout << setw(16) << "ops";
    cout << setw(6) << "rel";
    cout << setw(16) << "SizeOfTree";
#if OPTYP == 3
    cout << setw(8) << "commit";
#endif
    cout << endl;

    cout << "-------";              // range
    cout << setw(4) << "--";        // nt
    cout << setw(6) << "--";        // rt
    cout << setw(16) << "---";      // ops
    cout << setw(6) << "---";       // rel
    cout << setw(16) << "-----------";
#if OPTYP == 3
    cout << setw(8) << "------";
#endif
    cout << endl;

    // boost process priority
    // boost current thread priority to make sure all threads created before they start to run
#ifdef WIN32
    //  SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS);
    //  SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
#endif

    //
    // run tests
    
    UINT64 ops1 = 1;
    int treeSize[16];
    for (range = START; range < MAX ; range++) {
	    prefillTree(bstTree, ranges[range]);
        for (int nt = 1; nt <= maxThread; nt *= 2, indx++) {

            //  zero shared memory
            for (int thread = 0; thread < nt; thread++) *(GINDX(thread)) = 0;   // thread local
            *(GINDX(maxThread)) = 0;    // shared

#ifdef USEPMS
            zeroCounters();             // zero PMS counters
#endif

            // get start time
            tstart = getWallClockMS();

            // create worker threads
            for (int thread = 0; thread < nt; thread++)
                createThread(&threadH[thread], worker, (void*)(size_t)thread);

            // wait for ALL worker threads to finish
            waitForThreadsToFinish(nt, threadH);
            UINT64 rt = getWallClockMS() - tstart;

#ifdef USEPMS
            saveCounters();             // save PMS counters
#endif

            // save results and output summary to console
            for (int thread = 0; thread < nt; thread++) {
                r[indx].ops += ops[thread];
                r[indx].incs += *(GINDX(thread));
#if OPTYP == 3
                r[indx].aborts += aborts[thread];
#endif
            }
            r[indx].incs += *(GINDX(maxThread));
            if ((range == 0) && (nt == 1))
                ops1 = r[indx].ops;
            r[indx].range = range;
            r[indx].nt = nt;
            r[indx].rt = rt;

            cout << setw(6) << ranges[range];
            cout << setw(4) << nt;
            cout << setw(6) << fixed << setprecision(2) << (double)rt / 1000;
            cout << setw(16) << r[indx].ops;
            cout << setw(6) << fixed << setprecision(2) << (double)r[indx].ops / ops1;
		  treeSize[indx] = bstTree.size();
            if(bstTree.size() == NUM_OF_NODES) cout << setw(16) << "SAME";
		  else cout << setw(16) << treeSize[indx];
            bstTree = BST();

#if OPTYP == 3

            cout << setw(7) << fixed << setprecision(0) << 100.0 * (r[indx].ops - r[indx].aborts) / r[indx].ops << "%";

#endif
            cout << endl;
            // delete thread handles
            for (int thread = 0; thread < nt; thread++)	closeThread(threadH[thread]);
        }
    }

    cout << endl;

    // output results so they can easily be pasted into a spread sheet from console window
    
    setLocale();
    cout << "range/nt/rt/ops/incs/treeSize";
#if OPTYP == 3
    cout << "/aborts";
#endif
    cout << endl;
    for (UINT i = 0; i < indx; i++) {
        cout << r[i].range << "/" << r[i].nt << "/" << r[i].rt << "/" << r[i].ops << "/" << r[i].incs << "/" << treeSize[indx];
#if OPTYP == 3
        cout << "/" << r[i].aborts;
#endif      
        cout << endl;
    }
    cout << endl;

    quit();

    return 0;

}

// eof