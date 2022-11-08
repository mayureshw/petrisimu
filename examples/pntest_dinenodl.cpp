using namespace std;

#include <string>
#include <vector>
#include "petrinet.h"
PETRINET_STATICS

// Non deadlocking

// Factory to construct petrinet with configurable no of diners
// As a convention: Philisopher n's left hand fork is numbered n
class Diner
{
    IPetriNet *_pn;
public:
    string _id;
    PNPlace *eating, *thinking, *free_fork;
    PNTransition *strt_eating, *strt_thinking;
    Diner(IPetriNet *pn, int id) : _id(to_string(id)), _pn(pn)
    {
        eating    = _pn->createPlace("eating"+_id),
        thinking  = _pn->createPlace("thinking"+_id),
        free_fork = _pn->createPlace("free_fork"+_id);

        strt_eating   = _pn->createTransition("strt_eating"+_id),
        strt_thinking = _pn->createTransition("strt_thinking"+_id);

        _pn->createArc(free_fork,strt_eating);
        _pn->createArc(thinking,strt_eating);
        _pn->createArc(eating,strt_thinking);

        _pn->createArc(strt_eating,eating);
        _pn->createArc(strt_thinking,free_fork);
        _pn->createArc(strt_thinking,thinking);
    }
};

class DinerFactory
{
public:
    IPetriNet *_pn = new MTPetriNet();
    DinerFactory(int nDiners)
    {
        vector<Diner> diners;
        for(int i;i<nDiners;i++) diners.push_back(Diner(_pn,i));
        for(int i;i<nDiners;i++)
        {
            Diner& prev = i ? diners[i-1] : diners[nDiners-1], cur = diners[i];
            _pn->createArc(cur.free_fork,prev.strt_eating);
            _pn->createArc(prev.strt_thinking,cur.free_fork);
        }
        // Place tokens only after constituting full net
        for(auto d:diners)
        {
            _pn->addtokens(d.free_fork,1);
            _pn->addtokens(d.thinking,1);
        }
    }
    ~DinerFactory()
    {
        _pn->deleteElems();
        delete _pn;
    }
};

int main()
{
    // No of philosophers that end up dining varies before it deadlocks
    DinerFactory df(2);
    df._pn->printdot();
    df._pn->wait();
}
