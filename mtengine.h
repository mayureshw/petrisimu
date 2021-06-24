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
    list<thread*> _threads;
    queue<Work> _q;
    mutex _q_mutex;
    bool _quit = false;
    int _sleepms = 100; // if queue is empty sleep for these many ms

    void dowork()
    {
        while(true)
        {
            bool qempty;
            Work work;
            {
                const lock_guard<mutex> lockq(_q_mutex);
                qempty = _q.empty();
                if(!qempty)
                {
                    work = _q.front();
                    _q.pop();
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
    void addwork(Work work)
    {
        const lock_guard<mutex> lockq(_q_mutex);
        _q.push(work);
    }

    // Set to quit when the queue is found empty
    void quit() { _quit = true; }

    void wait()
    {
        for(auto t:_threads)
        {
            t->join();
            delete t;
        }
    }

    MTEngine(int nThreads)
    {
        for(int i=0; i<nThreads; i++) _threads.push_back(new thread(&MTEngine::dowork,this));
    }
};

#endif
