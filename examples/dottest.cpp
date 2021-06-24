#include <iostream>
#include <list>
#include "dot.h"

int main()
{
    DNodeList nl = {DNode("1",(Proplist){{"shape","circle"}}), DNode("2")};
    DEdgeList el = {DEdge("1","2")};
    DGraph g(nl,el,(Proplist){{"rankdir","LR"}});
    g.printdot("see.dot");
    return 0;
}
