using namespace std;

#include "petrinet.h"
PETRINET_STATICS

int main()
{
    auto
        *p1 = new PNPlace("p1"),
        *p2 = new PNPlace("p2"),
        *p3 = new PNPlace("p3"),
        *p4 = new PNPlace("p4");
    auto
        *pq = new PNQuitPlace("pq");
    auto
        *t1 = new PNTransition("t1"),
        *t2 = new PNTransition("t2"),
        *t3 = new PNTransition("t3");
    auto
        *a1 = new PNPTArc(p1,t1),
        *a2 = new PNPTArc(p1,t2),
        *a3 = new PNPTArc(p2,t1),
        *a4 = new PNPTArc(p2,t2),
        *a5 = new PNPTArc(p3,t3),
        *a6 = new PNPTArc(p4,t3);
    auto
        *a7 = new PNTPArc(t1,p3),
        *a8 = new PNTPArc(t2,p4),
        *a9 = new PNTPArc(t3,pq);

    PetriNet pn({p1,p2,p3,p4,pq,t1,t2,t3,a1,a2,a3,a4,a5,a6,a7,a8,a9});
    pn.printdot();
    pn.printpnml();
    p1->addtokens(1);
    p2->addtokens(1);
    // The net may or may not deadlock in different runs as the behavior is non
    // deterministic. Putting a delay here makes it deadlock with more certainty.
    this_thread::sleep_for(chrono::milliseconds(200));
    p1->addtokens(1);
    p2->addtokens(1);
    pn.wait();
    pn.deleteElems();
}
