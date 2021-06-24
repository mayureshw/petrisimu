#include "petrinet.h"

// https://people.cs.vt.edu/kafura/ComputationalThinking/Class-Notes/Petri-Net-Notes-Expanded.pdf
// Fig 7

int main()
{
    auto
        *at1 = new PNDbgPlace("at1"),
        *use1 = new PNDbgPlace("use1"),
        *done1 = new PNDbgPlace("done1"),
        *drive1 = new PNDbgPlace("drive1"),
    
        *at2 = new PNDbgPlace("at2"),
        *use2 = new PNDbgPlace("use2"),
        *done2 = new PNDbgPlace("done2"),
        *drive2 = new PNDbgPlace("drive2"),

        *mutex = new PNDbgPlace("mutex");

    auto
        *toat1 = new PNDbgTransition("toat1"),
        *touse1 = new PNDbgTransition("touse1"),
        *todone1 = new PNDbgTransition("todone1"),
        *todrive1 = new PNDbgTransition("todrive1"),

        *toat2 = new PNDbgTransition("toat2"),
        *touse2 = new PNDbgTransition("touse2"),
        *todone2 = new PNDbgTransition("todone2"),
        *todrive2 = new PNDbgTransition("todrive2");

    list<PNArc*> arcs = {
        new PNPTArc(at1,touse1),
        new PNPTArc(use1,todone1),
        new PNPTArc(done1,todrive1),
        new PNPTArc(drive1,toat1),
        new PNTPArc(toat1,at1),
        new PNTPArc(touse1,use1),
        new PNTPArc(todone1,done1),
        new PNTPArc(todrive1,drive1),

        new PNPTArc(at2,touse2),
        new PNPTArc(use2,todone2),
        new PNPTArc(done2,todrive2),
        new PNPTArc(drive2,toat2),
        new PNTPArc(toat2,at2),
        new PNTPArc(touse2,use2),
        new PNTPArc(todone2,done2),
        new PNTPArc(todrive2,drive2),

        new PNPTArc(mutex,touse1),
        new PNTPArc(todone1,mutex),
        new PNPTArc(mutex,touse2),
        new PNTPArc(todone2,mutex),
        };

    PetriNet pn({at1,use1,done1,drive1,mutex,at2,use2,done2,drive2},{toat1,touse1,todone1,todrive1,toat2,touse2,todone2,todrive2},arcs);
    pn.printdot();
    mutex->addtokens(1);
    // Deliberately add 2 tokens, to test that transition doesn't deadlock if
    // token supply remains high after transition (without dipping to low in
    // between)
    drive1->addtokens(2);
    drive2->addtokens(2);
    pn.wait();
    pn.deleteElems();
}
