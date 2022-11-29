using namespace std;

#include "petrinet.h"

int main()
{
    IPetriNet *pn = new MTPetriNet();

    auto
        *mutex  = pn->createPlace("mutex"),
        *free   = pn->createPlace("free"),
        *filled = pn->createPlace("filled"),
        *push1P = pn->createPlace("push1P"),
        *push2P = pn->createPlace("push2P"),
        *pop1P  = pn->createPlace("pop1P"),
        *pop2P  = pn->createPlace("pop2P");
    auto
        *push1 = pn->createTransition("PushReq1"),
        *push2 = pn->createTransition("PushReq2"),
        *pop1  = pn->createTransition("PopReq1"),
        *pop2  = pn->createTransition("PopReq2");

        pn->createArc(mutex,push1);
        pn->createArc(mutex,push2);
        pn->createArc(mutex,pop1);
        pn->createArc(mutex,pop2);
        pn->createArc(free,push1);
        pn->createArc(free,push2);
        pn->createArc(filled,pop1);
        pn->createArc(filled,pop2);
        pn->createArc(push1P,push1);
        pn->createArc(push2P,push2);
        pn->createArc(pop1P,pop1);
        pn->createArc(pop2P,pop2);

        pn->createArc(push1,mutex);
        pn->createArc(push2,mutex);
        pn->createArc(pop1,mutex);
        pn->createArc(pop2,mutex);
        pn->createArc(pop1,free);
        pn->createArc(pop2,free);
        pn->createArc(push1,filled);
        pn->createArc(push2,filled);

    pn->printdot();
    pn->addtokens(free,3);
    pn->addtokens(mutex,1);
    pn->addtokens(push1P,1);
    pn->addtokens(push2P,1);
    pn->addtokens(push1P,1);
    pn->addtokens(push2P,1);
    pn->addtokens(pop1P,1);
    pn->wait();
    pn->deleteElems();
}
