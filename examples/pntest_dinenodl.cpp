#include <string>
#include <vector>
#include "petrinet.h"

// Non deadlocking

// Factory to construct petrinet with configurable no of diners
// As a convention: Philisopher n's left hand fork is numbered n
class Diner
{
public:
    string _id;
    PNDbgPlace
        *eating = new PNDbgPlace("eating"+_id),
        *thinking = new PNDbgPlace("thinking"+_id),
        *free_fork = new PNDbgPlace("free_fork"+_id);
    PNDbgTransition
        *strt_eating = new PNDbgTransition("strt_eating"+_id),
        *strt_thinking = new PNDbgTransition("strt_thinking"+_id);
    list<PNDbgPlace*> places = {
        eating, thinking, free_fork
        };
    list<PNDbgTransition*> transitions = {
        strt_eating, strt_thinking
        };
    list<PNArc*> arcs = {
        new PNPTArc(free_fork,strt_eating),
        new PNPTArc(thinking,strt_eating),
        new PNPTArc(eating,strt_thinking),

        new PNTPArc(strt_eating,eating),
        new PNTPArc(strt_thinking,free_fork),
        new PNTPArc(strt_thinking,thinking),
        };
    Diner(int id) : _id(to_string(id)) {}
};

class DinerFactory
{
public:
    PetriNet *pn;
    DinerFactory(int nDiners)
    {
        vector<Diner> diners;
        list<PNPlace*> places;
        list<PNTransition*> transitions;
        list<PNArc*> arcs;
        for(int i;i<nDiners;i++) diners.push_back(Diner(i));
        for(int i;i<nDiners;i++)
        {
            Diner& prev = i ? diners[i-1] : diners[nDiners-1], cur = diners[i];
            prev.arcs.push_back(new PNPTArc(cur.free_fork,prev.strt_eating));
            prev.arcs.push_back(new PNTPArc(prev.strt_thinking,cur.free_fork));
            for(auto p:prev.places) places.push_back(p);
            for(auto t:prev.transitions) transitions.push_back(t);
            for(auto a:prev.arcs) arcs.push_back(a);
        }
        pn = new PetriNet(places,transitions,arcs);
        // Place tokens only after constituting full net
        for(auto d:diners)
        {
            d.free_fork->addtokens(1);
            d.thinking->addtokens(1);
        }
    }
    ~DinerFactory()
    {
        pn->deleteElems();
        delete pn;
    }
};

int main()
{
    // No of philosophers that end up dining varies before it deadlocks
    DinerFactory df(2);
    df.pn->printdot();
    df.pn->wait();
}
