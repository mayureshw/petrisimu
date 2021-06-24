#include <string>
#include <vector>
#include "petrinet.h"

// https://workcraft.org/tutorial/model/dining_philosophers/start

// Factory to construct petrinet with configurable no of diners
// As a convention: Philisopher n's left hand fork is numbered n
class Diner
{
public:
    string _id;
    PNDbgPlace
        *have_lfork = new PNDbgPlace("have_lfork"+_id),
        *have_rfork = new PNDbgPlace("have_rfork"+_id),
        *lhand_empty = new PNDbgPlace("lhand_empty"+_id),
        *rhand_empty = new PNDbgPlace("rhand_empty"+_id),
        *eating = new PNDbgPlace("eating"+_id),
        *thinking = new PNDbgPlace("thinking"+_id),
        *thinkingl = new PNDbgPlace("thinkingl"+_id),
        *thinkingr = new PNDbgPlace("thinkingr"+_id),
        *free_fork = new PNDbgPlace("free_fork"+_id);
    PNDbgTransition
        *take_lfork = new PNDbgTransition("take_lfork"+_id),
        *take_rfork = new PNDbgTransition("take_rfork"+_id),
        *strt_eating = new PNDbgTransition("strt_eating"+_id),
        *strt_thinking = new PNDbgTransition("strt_thinking"+_id),
        *put_lfork = new PNDbgTransition("put_lfork"+_id),
        *put_rfork = new PNDbgTransition("put_rfork"+_id);
    list<PNDbgPlace*> places = {
        have_lfork, have_rfork, lhand_empty, rhand_empty, eating, thinking, thinkingl, thinkingr, free_fork
        };
    list<PNDbgTransition*> transitions = {
        take_lfork, take_rfork, strt_eating, strt_thinking, put_lfork, put_rfork
        };
    list<PNArc*> arcs = {
        new PNPTArc(have_lfork,strt_eating),
        new PNPTArc(have_rfork,strt_eating),
        new PNPTArc(lhand_empty,take_lfork),
        new PNPTArc(rhand_empty,take_rfork),
        new PNPTArc(thinking,strt_eating),
        new PNPTArc(thinkingl,put_lfork),
        new PNPTArc(thinkingr,put_rfork),
        new PNPTArc(eating,strt_thinking),
        new PNPTArc(free_fork,take_lfork),

        new PNTPArc(take_lfork,have_lfork),
        new PNTPArc(take_rfork,have_rfork),
        new PNTPArc(strt_eating,eating),
        new PNTPArc(put_lfork,lhand_empty),
        new PNTPArc(put_rfork,rhand_empty),
        new PNTPArc(strt_thinking,thinking),
        new PNTPArc(strt_thinking,thinkingl),
        new PNTPArc(strt_thinking,thinkingr),
        new PNTPArc(put_lfork,free_fork),
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
            prev.arcs.push_back(new PNPTArc(cur.free_fork,prev.take_rfork));
            prev.arcs.push_back(new PNTPArc(prev.put_rfork,cur.free_fork));
            for(auto p:prev.places) places.push_back(p);
            for(auto t:prev.transitions) transitions.push_back(t);
            for(auto a:prev.arcs) arcs.push_back(a);
        }
        pn = new PetriNet(places,transitions,arcs);
        // Place tokens only after constituting full net
        for(auto d:diners)
        {
            d.free_fork->addtokens(1);
            d.lhand_empty->addtokens(1);
            d.rhand_empty->addtokens(1);
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
    DinerFactory df(4);
    df.pn->printdot();
    df.pn->wait();
}
