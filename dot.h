#ifndef _DOT_H
#define _DOT_H

#include<list>
#include<fstream>
#include<tuple>
#include<string>

using namespace std;

typedef list<tuple<string,string>> Proplist;

class Props
{
    Proplist _props;
public:
    string str()
    {
        string label = "[";
        for(auto const& pv:_props) label += get<0>(pv) + "=" + get<1>(pv) + ",";
        return label + "]";
    }
    Props(Proplist pl={}) : _props(pl) {}
};

class DNode
{
    string _id;
    Props _props;
public:
    string str() { return _id + " " + _props.str(); }
    DNode(string id, Props props=Props()) : _id(id), _props(props) {}
};

class DEdge
{
    string _id1, _id2;
    Props _props;
public:
    string str() { return _id1 + " -> " + _id2 + " " + _props.str(); }
    DEdge(string id1, string id2, Props props=Props()) : _id1(id1), _id2(id2), _props(props) {}
};

typedef list<DNode> DNodeList;
typedef list<DEdge> DEdgeList;

class DGraph
{
    Props _gprops;
    DNodeList _nodes;
    DEdgeList _edges;
public:
    void printdot(string flnm = "graph.dot")
    {
        ofstream ofs;
        ofs.open(flnm);
        ofs << "digraph {" << endl;
        ofs << "graph " << _gprops.str() << endl;
        for(auto n:_nodes) ofs << n.str() << endl;
        for(auto e:_edges) ofs << e.str() << endl;
        ofs << "}" << endl;
        ofs.close();
    }
    DGraph(DNodeList& nodes, DEdgeList& edges, Props props=Props()) :
        _nodes(nodes), _edges(edges), _gprops(props) {}
};

#endif
