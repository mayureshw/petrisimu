#include "petrinet.h"

int main()
{
    auto
        *excl = new PNDbgPlace("Excl"),
        *depth = new PNDbgPlace("depth"),
        *push1P = new PNDbgPlace("push1P"),
        *push2P = new PNDbgPlace("push2P"),
        *pop1P = new PNDbgPlace("pop1P"),
        *pop2P = new PNDbgPlace("pop2P");
    auto
        *push1 = new PNDbgTransition("PushReq1"),
        *push2 = new PNDbgTransition("PushReq2"),
        *pop1 = new PNDbgTransition("PopReq1"),
        *pop2 = new PNDbgTransition("PopReq2");
    auto
        *a1 = new PNPTArc(excl,push1),
        *a2 = new PNPTArc(excl,push2),
        *a3 = new PNPTArc(excl,pop1),
        *a4 = new PNPTArc(excl,pop2),
        *a5 = new PNPTArc(depth,push1),
        *a6 = new PNPTArc(depth,push2);
    auto
        *a7 = new PNTPArc(push1,excl),
        *a8 = new PNTPArc(push2,excl),
        *a9 = new PNTPArc(pop1,excl),
        *a10 = new PNTPArc(pop2,excl),
        *a11 = new PNTPArc(pop1,depth),
        *a12 = new PNTPArc(pop2,depth);
    auto
        *a13 = new PNPTArc(push1P,push1),
        *a14 = new PNPTArc(push2P,push2),
        *a15 = new PNPTArc(pop1P,pop1),
        *a16 = new PNPTArc(pop2P,pop2);

    PetriNet pn({excl,depth,push1P,push2P,pop1P,pop2P},{push1,push2,pop1,pop2},{a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12});
    pn.printdot();
    depth->addtokens(3);
    excl->addtokens(1);
    push1P->addtokens(1);
    push2P->addtokens(1);
    push1P->addtokens(1);
    push2P->addtokens(1);
    pop1P->addtokens(1);
    pn.wait();
    pn.deleteElems();
}
