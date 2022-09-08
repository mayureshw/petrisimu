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
public:
    string _id;
    PNPlace
        *eating = new PNPlace("eating"+_id),
        *thinking = new PNPlace("thinking"+_id),
        *free_fork = new PNPlace("free_fork"+_id);
    PNTransition
        *strt_eating = new PNTransition("strt_eating"+_id),
        *strt_thinking = new PNTransition("strt_thinking"+_id);
    list<PNPlace*> places = {
        eating, thinking, free_fork
        };
    list<PNTransition*> transitions = {
        strt_eating, strt_thinking
        };
    Arcs arcs = {
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
        Elements pnes;
        for(int i;i<nDiners;i++) diners.push_back(Diner(i));
        for(int i;i<nDiners;i++)
        {
            Diner& prev = i ? diners[i-1] : diners[nDiners-1], cur = diners[i];
            prev.arcs.push_back(new PNPTArc(cur.free_fork,prev.strt_eating));
            prev.arcs.push_back(new PNTPArc(prev.strt_thinking,cur.free_fork));
            for(auto p:prev.places) pnes.insert(p);
            for(auto t:prev.transitions) pnes.insert(t);
            for(auto a:prev.arcs) pnes.insert(a);
        }
        pn = new PetriNet(pnes);
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
