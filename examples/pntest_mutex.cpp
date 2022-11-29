using namespace std;

#include "petrinet.h"

// https://people.cs.vt.edu/kafura/ComputationalThinking/Class-Notes/Petri-Net-Notes-Expanded.pdf
// Fig 7

int main()
{
    IPetriNet *pn = new MTPetriNet();
    auto
        at1    = pn->createPlace("at1"),
        use1   = pn->createPlace("use1"),
        done1  = pn->createPlace("done1"),
        drive1 = pn->createPlace("drive1"),
    
        at2    = pn->createPlace("at2"),
        use2   = pn->createPlace("use2"),
        done2  = pn->createPlace("done2"),
        drive2 = pn->createPlace("drive2"),

        mutex = pn->createPlace("mutex");

    auto
        toat1    = pn->createTransition("toat1"),
        touse1   = pn->createTransition("touse1"),
        todone1  = pn->createTransition("todone1"),
        todrive1 = pn->createTransition("todrive1"),

        toat2    = pn->createTransition("toat2"),
        touse2   = pn->createTransition("touse2"),
        todone2  = pn->createTransition("todone2"),
        todrive2 = pn->createTransition("todrive2");

        pn->createArc(at1,touse1);
        pn->createArc(use1,todone1);
        pn->createArc(done1,todrive1);
        pn->createArc(drive1,toat1);
        pn->createArc(toat1,at1);
        pn->createArc(touse1,use1);
        pn->createArc(todone1,done1);
        pn->createArc(todrive1,drive1);

        pn->createArc(at2,touse2);
        pn->createArc(use2,todone2);
        pn->createArc(done2,todrive2);
        pn->createArc(drive2,toat2);
        pn->createArc(toat2,at2);
        pn->createArc(touse2,use2);
        pn->createArc(todone2,done2);
        pn->createArc(todrive2,drive2);

        pn->createArc(mutex,touse1);
        pn->createArc(todone1,mutex);
        pn->createArc(mutex,touse2);
        pn->createArc(todone2,mutex);

    pn->printdot();
    pn->addtokens(mutex,1);
    // Deliberately add 2 tokens, to test that transition doesn't deadlock if
    // token supply remains high after transition (without dipping to low in
    // between)
    pn->addtokens(drive1,2);
    pn->addtokens(drive2,2);
    pn->wait();
    pn->deleteElems();
    delete pn;
}
