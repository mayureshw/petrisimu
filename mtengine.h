#ifndef _MTENGINE_H
#define _MTENGINE_H

#include <list>
#include <queue>
#include <mutex>
#include <thread>
#include <chrono>
#include <functional>
  
using namespace std;

typedef function<void()> Work;

// A queue based, multithreading engine
// TODO: Make use of priority queue
class MTEngine
{
    const unsigned _lqthreshold = 2; // Add to _gq if lq size exeeds this (TODO: parameterize this?)
    list<thread*> _threads;
    static thread_local queue<Work> _lq; // NOTE: Application must declare it in any one cpp file
    queue<Work> _gq;
    mutex _gq_mutex;
    bool _quit = false;
    int _sleepms = 100; // if queue is empty sleep for these many ms

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
                else this_thread::sleep_for(chrono::milliseconds(_sleepms));
            }
            else work();
        }
    }
public:
    void addworkg(Work work)
    {
        const lock_guard<mutex> lockq(_gq_mutex);
        _gq.push(work);
    }
    void addwork(Work work)
    {
        if ( _lq.size() > _lqthreshold ) addworkg(work);
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

    MTEngine(int nThreads)
    {
        // We rope in main thread once it invokes wait hence start 1 thread less
        for(int i=0; i<nThreads-1; i++) _threads.push_back(new thread(&MTEngine::dowork,this));
    }
};

#endif
