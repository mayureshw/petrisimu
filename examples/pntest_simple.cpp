#include "petrinet.h"

int main()
{
    PNPlace
        *p1 = new PNPlace("p1"),
        *p2 = new PNPlace("p2");
    PNTransition
        *t1 = new PNTransition("t1"),
        *t2 = new PNTransition("t2");
    PNArc
        *a1 = new PNPTArc(p1,t1),
        *a2 = new PNTPArc(t1,p2),
        *a3 = new PNPTArc(p2,t2);
    PetriNet pn({p1,p2},{t1,t2},{a1,a2,a3});
    pn.printdot();
    pn.deleteElems();
}
