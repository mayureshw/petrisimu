using namespace std;

#include "petrinet.h"

int main()
{
    IPetriNet *pn = new MTPetriNet();
    PNPlace
        *p1 = pn->createPlace("p1"),
        *p2 = pn->createPlace("p2");
    PNTransition
        *t1 = pn->createTransition("t1"),
        *t2 = pn->createTransition("t2");
    pn->createArc(p1,t1),
    pn->createArc(t1,p2),
    pn->createArc(p2,t2);
    pn->printdot();
    pn->deleteElems();
}
