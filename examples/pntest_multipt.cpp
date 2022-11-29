using namespace std;

#include "petrinet.h"

int main()
{
    IPetriNet *pn = new MTPetriNet();
    auto
        *p1 = pn->createPlace("p1"),
        *p2 = pn->createPlace("p2"),
        *p3 = pn->createPlace("p3"),
        *p4 = pn->createPlace("p4");
    auto
        *pq = pn->createQuitPlace("pq");
    auto
        *t1 = pn->createTransition("t1"),
        *t2 = pn->createTransition("t2"),
        *t3 = pn->createTransition("t3");

    pn->createArc(p1,t1);
    pn->createArc(p1,t2);
    pn->createArc(p2,t1);
    pn->createArc(p2,t2);
    pn->createArc(p3,t3);
    pn->createArc(p4,t3);
    pn->createArc(t1,p3);
    pn->createArc(t2,p4);
    pn->createArc(t3,pq);

    pn->printdot();
    pn->printpnml();
    pn->addtokens(p1, 1);
    pn->addtokens(p2, 1);
    // The net may or may not deadlock in different runs as the behavior is non
    // deterministic. Putting a delay here makes it deadlock with more certainty.
    this_thread::sleep_for(chrono::milliseconds(200));
    pn->addtokens(p1, 1);
    pn->addtokens(p2, 1);
    pn->wait();
    pn->deleteElems();
}
