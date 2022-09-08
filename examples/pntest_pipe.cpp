using namespace std;

#include "petrinet.h"
PETRINET_STATICS

int main()
{
    auto
        *mutex = new PNPlace("mutex"),
        *free = new PNPlace("free"),
        *filled = new PNPlace("filled"),
        *push1P = new PNPlace("push1P"),
        *push2P = new PNPlace("push2P"),
        *pop1P = new PNPlace("pop1P"),
        *pop2P = new PNPlace("pop2P");
    auto
        *push1 = new PNTransition("PushReq1"),
        *push2 = new PNTransition("PushReq2"),
        *pop1 = new PNTransition("PopReq1"),
        *pop2 = new PNTransition("PopReq2");
    auto
        *a1 = new PNPTArc(mutex,push1),
        *a2 = new PNPTArc(mutex,push2),
        *a3 = new PNPTArc(mutex,pop1),
        *a4 = new PNPTArc(mutex,pop2),
        *a5 = new PNPTArc(free,push1),
        *a6 = new PNPTArc(free,push2),
        *a7 = new PNPTArc(filled,pop1),
        *a8 = new PNPTArc(filled,pop2),
        *a9 = new PNPTArc(push1P,push1),
        *a10 = new PNPTArc(push2P,push2),
        *a11 = new PNPTArc(pop1P,pop1),
        *a12 = new PNPTArc(pop2P,pop2);
    auto
        *a13 = new PNTPArc(push1,mutex),
        *a14 = new PNTPArc(push2,mutex),
        *a15 = new PNTPArc(pop1,mutex),
        *a16 = new PNTPArc(pop2,mutex),
        *a17 = new PNTPArc(pop1,free),
        *a18 = new PNTPArc(pop2,free),
        *a19 = new PNTPArc(push1,filled),
        *a20 = new PNTPArc(push2,filled);

    PetriNet pn(
        {mutex,free,filled,push1P,push2P,pop1P,pop2P,
        push1,push2,pop1,pop2,
        a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16,a17,a18,a19,a20}
        );
    pn.printdot();
    free->addtokens(3);
    mutex->addtokens(1);
    push1P->addtokens(1);
    push2P->addtokens(1);
    push1P->addtokens(1);
    push2P->addtokens(1);
    pop1P->addtokens(1);
    pn.wait();
    pn.deleteElems();
}
