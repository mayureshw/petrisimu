#ifndef _MTENGINE_H
#define _MTENGINE_H

#include <list>
#include <iostream>
#include <queue>
#include <mutex>
#include <thread>
#include <chrono>
#include <functional>
#include <condition_variable>
  
using namespace std;

typedef function<void()> Work;

// A queue based, multithreading engine
// TODO: Make use of priority queue
class MTEngine
{
    unsigned _lqthreshold; // Add work to _gq if _lq size exeeds this
    list<thread*> _threads;
    static thread_local queue<Work> _lq; // NOTE: Application must declare it in any one cpp file
    queue<Work> _gq;
    mutex _gq_mutex;
    condition_variable _gq_cvar;

    void dolocal()
    {
        while( not _lq.empty() )
        {
            auto work = _lq.front();
            _lq.pop();
            work();
        }
    }
    void dowork()
    {
        while(true)
        {
            dolocal();
            bool qempty;
            Work work;
            {
                const lock_guard<mutex> lockq(_gq_mutex);
                qempty = _gq.empty();
                if(!qempty)
                {
                    work = _gq.front();
                    _gq.pop();
                }
            }
            if(qempty)
            {
                if(_quit) break;
                else
                {
                    unique_lock<mutex> ulockq(_gq_mutex);
                    _gq_cvar.wait(ulockq, []{return true;} );
                }
            }
            else work();
        }
    }
protected:
    bool _quit = false;
public:
    void addwork(Work& work)
    {
        // TODO: lqthreshold heuristic is temporarily removed 1. If there are
        // threads that are not a part of the pool how will this heuristic
        // behave, needs to be analyzed.
        // 2. During wait 100% CPU usage is seen
        // 3. Find a way to notify when item is added to lq
        //if ( _lq.size() > _lqthreshold )
        if ( true )
        {
            {
                const lock_guard<mutex> lockq(_gq_mutex);
                _gq.push(work);
            }
            _gq_cvar.notify_one();
        }
        else _lq.push(work);
    }

    // Set to quit when the queue is found empty
    void quit() { _quit = true; }

    void wait()
    {
        dowork(); // Let main thread also work
        for(auto t:_threads)
        {
            t->join();
            delete t;
        }
    }

    MTEngine()
    {
        char *nthreadsvar = getenv("NTHREADS");
        unsigned nThreads = nthreadsvar ? stoi(nthreadsvar) : 1;
        cout << "MTEngine : nThreads set to " << nThreads << endl;

        char *lqthresholdvar = getenv("LQTHRESHOLD");
        _lqthreshold = lqthresholdvar ? stoi(lqthresholdvar) : 2;
        cout << "MTEngine : lqthreshold set to " << _lqthreshold << endl;

        // We rope in main thread once it invokes wait hence start 1 thread less
        for(int i=0; i<nThreads-1; i++) _threads.push_back(new thread(&MTEngine::dowork,this));
    }
};

#endif
