#include "mtengine.h"

// A dummy workload that sleeps for given number of seconds
class DummyWork
{
    int _n;
public:
    void run() { this_thread::sleep_for(chrono::seconds(_n)); }
    DummyWork(int n):_n(n){}
};

int main()
{
    // Run the executable with time command. Expected elapsed time is around ((nitems//nthreads)+1)*sleepsec

    int nthreads = 4;
    int sleepsec = 2;
    int nitems = 4;

    MTEngine mte(nthreads);
    DummyWork work(sleepsec);
    for(int i=0;i<nitems;i++) mte.addwork(bind(&DummyWork::run,&work));
    mte.quit();
    mte.wait();
    return 0;
}
