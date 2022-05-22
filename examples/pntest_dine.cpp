#include <string>
#include <vector>
#include "petrinet.h"
thread_local queue<Work> MTEngine::_lq;
// https://www.labri.fr/perso/anca/FDS/Pn-ESTII.pdf

// Factory to construct petrinet with configurable no of diners
// As a convention: Philisopher n's left hand fork is numbered n
class Diner
{
public:
    string _id;
    PNDbgPlace
        *have_lfork = new PNDbgPlace("have_lfork"+_id),
        *eating = new PNDbgPlace("eating"+_id),
        *thinking = new PNDbgPlace("thinking"+_id),
        *free_fork = new PNDbgPlace("free_fork"+_id);
    PNDbgTransition
        *take_lfork = new PNDbgTransition("take_lfork"+_id),
        *strt_eating = new PNDbgTransition("strt_eating"+_id),
        *strt_thinking = new PNDbgTransition("strt_thinking"+_id);
    list<PNDbgPlace*> places = {
        have_lfork, eating, thinking, free_fork
        };
    list<PNDbgTransition*> transitions = {
        take_lfork, strt_eating, strt_thinking
        };
    Arcs arcs = {
        new PNPTArc(thinking,take_lfork),
        new PNPTArc(have_lfork,strt_eating),
        new PNPTArc(eating,strt_thinking),
        new PNPTArc(free_fork,take_lfork),

        new PNTPArc(take_lfork,have_lfork),
        new PNTPArc(strt_eating,eating),
        new PNTPArc(strt_thinking,thinking),
        new PNTPArc(strt_thinking,free_fork),
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
        Arcs arcs;
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
