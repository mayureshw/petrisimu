using namespace std;

#include "petrinet.h"
PETRINET_STATICS

// https://people.cs.vt.edu/kafura/ComputationalThinking/Class-Notes/Petri-Net-Notes-Expanded.pdf
// Fig 7

int main()
{
    auto
        *at1 = new PNPlace("at1"),
        *use1 = new PNPlace("use1"),
        *done1 = new PNPlace("done1"),
        *drive1 = new PNPlace("drive1"),
    
        *at2 = new PNPlace("at2"),
        *use2 = new PNPlace("use2"),
        *done2 = new PNPlace("done2"),
        *drive2 = new PNPlace("drive2"),

        *mutex = new PNPlace("mutex");

    auto
        *toat1 = new PNTransition("toat1"),
        *touse1 = new PNTransition("touse1"),
        *todone1 = new PNTransition("todone1"),
        *todrive1 = new PNTransition("todrive1"),

        *toat2 = new PNTransition("toat2"),
        *touse2 = new PNTransition("touse2"),
        *todone2 = new PNTransition("todone2"),
        *todrive2 = new PNTransition("todrive2");

    Elements pnes = {
        at1,use1,done1,drive1,mutex,at2,use2,done2,drive2,toat1,touse1,todone1,todrive1,toat2,touse2,todone2,todrive2,
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

    PetriNet pn(pnes);
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
