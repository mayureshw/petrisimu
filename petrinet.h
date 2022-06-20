#ifndef _PETRINET_H
#define _PETRINET_H

#include <iostream>
#include <string>
#include <list>
#include <set>
#include "dot.h"
#include "mtengine.h"

using namespace std;

// NOTES (in E-R model terms):
// Basic entities (class names have prefix PN):
//  - Place and Transition - basic petri net concepts
//  - PTArc and TPArc are arcs from Place to Transition and reverse resp.
// Inheritance:
//  - Place and Transition are Node
//  - PTArc and TPArc are Arc
// Properties:
//  - All Nodes have a name
//  - Arcs have weight (default 1)
//  - Places have capacity (default 0 i.e. unlimited)
// Relations between objects:
//  - There is a 2-way relation between PetriNet and each of Place, Transition, Arc
//  - There is a 2-way relation between Node and Arc
//    (input and output relation identified separately)

class PNArc;
class PNPlace;
class PNTransition;
class PNNode;
class PNElement;
typedef vector<PNArc*> Arcs; // a vector to aid filtering by indices
typedef list<PNPlace*> Places;
typedef list<PNTransition*> Transitions;
typedef list<PNNode*> Nodes;
typedef set<PNElement*> Elements;

// Interfaces to resolve inter-dependencies
class IPetriNet : public MTEngine
{
    int _idcntr = 0;
public:
    int getIncId() { return _idcntr++; }
};

class IPNTransition
{
public:
    virtual void gotEnoughTokens()=0;
    virtual void notEnoughTokens()=0;
};


class PNElement
{
public:
    typedef enum {PLACE,TRANSITION,ARC} Etyp;
    virtual Etyp typ()=0;
};

class PNArc : public PNElement
{
public:
    PNPlace* _place;
    PNTransition* _transition;
    unsigned _wt;
    virtual DEdge dedge() = 0;
    Etyp typ() { return ARC; }
    PNArc(PNPlace* p, PNTransition* t, unsigned wt) : _place(p), _transition(t), _wt(wt) {}
    virtual ~PNArc() {}
};

class PNNode : public PNElement
{
    int _nodeid;
public:
    IPetriNet* _pn;
    Arcs _iarcs;
    Arcs _oarcs;
    const string _name;
    void setpn(IPetriNet* pn)
    {
        _pn = pn;
        _nodeid = _pn->getIncId();
    }
    void addiarc(PNArc* a) { _iarcs.push_back(a); }
    void addoarc(PNArc* a) { _oarcs.push_back(a); }
    string idlabel() { return idstr() + ":" + _name; }
    string idstr() { return to_string(_nodeid); }
    PNNode(string name) : _name(name) {}
};

class PNPlace : public PNNode
{
    function<list<int>()> _arcchooser = NULL;
    unsigned _capacity;
    mutex _tokenmutex;
protected:
    function<void()> _addactions = [](){};
public:
    // This can be put on queue by adding a wrapper that does addwork, for granularity reason it wasn't
    void addtokens(unsigned newtokens)
    {
        Arcs eligibleArcs;
        if ( _arcchooser != NULL )
        {
            list<int> arcindices = _arcchooser();
            for(auto i:arcindices)
                eligibleArcs.push_back(_oarcs[i]);
        }
        else
            eligibleArcs = _oarcs;

        lock();
        unsigned oldcnt = _tokens;
        _tokens += newtokens;
        for(auto oarc:eligibleArcs)
            // inform the transition only if we crossed the threshold now
            // Think, whether we want to randomize the sequence of oarc for non
            // determinism Of course, without it also the behavior is correct,
            // since a deterministic sequence is a subset of possible non
            // deterministic behaviors anyway.
            if( _tokens >= oarc->_wt && oldcnt < oarc->_wt )
                ((IPNTransition*)oarc->_transition)->gotEnoughTokens();
        unlock();
        addactions();
    }
    unsigned _tokens = 0;
    Etyp typ() { return PLACE; }
    void setArcChooser(function<list<int>()> f) { _arcchooser = f; }
    void setAddActions(function<void()> af) { _addactions = af; }
    virtual void addactions() { _addactions(); }
    virtual void deductactions() {}
    void lock() { _tokenmutex.lock(); }
    bool lockIfEnough(unsigned mintokens)
    {
        lock();
        if(_tokens >= mintokens) return true;
        else
        {
            unlock();
            return false;
        }
    }
    void unlock() { _tokenmutex.unlock(); }
    // deducttokens Expects caller to have taken care of locking
    void deducttokens(unsigned deducttokens)
    {
        unsigned oldcnt = _tokens;
        _tokens -= deducttokens;
        for(auto oarc:_oarcs)
            // inform the transition only if we went below the threshold now
            if( _tokens < oarc->_wt && oldcnt >= oarc->_wt )
                ((IPNTransition*)oarc->_transition)->notEnoughTokens();
    }
    DNode dnode() { return DNode(idstr(),(Proplist){{"label",_name}}); }
    // capacity 0 means place can hold unlimited tokens
    PNPlace(string name,unsigned capacity=0) : PNNode(name), _capacity(capacity) {}
    virtual ~PNPlace() {}
};

class PNTransition : public IPNTransition, public PNNode
{
    Work _tryTriggerWork = bind(&PNTransition::tryTrigger,this);
    unsigned _enabledPlaceCnt = 0;
    mutex _enabledPlaceCntMutex;
    bool haveEnoughTokens() { return _enabledPlaceCnt == _iarcs.size(); }
    // Recursive walk helps keep it simple to avoid locking input places in
    // case previous ones do not meet the criteria
    bool tryTransferTokens(Arcs::iterator it)
    {
        if(it==_iarcs.end()) return true;
        auto ptarc = *it;
        if(ptarc->_place->lockIfEnough(ptarc->_wt))
        {
            if(tryTransferTokens(++it))
            {
                ptarc->_place->deducttokens(ptarc->_wt);
                ptarc->_place->unlock();
                return true;
            }
            else
            {
                ptarc->_place->unlock();
                return false;
            }
        }
        else return false;
    }
protected:
    function<void()> _enabledactions = [](){};
    virtual void notEnoughTokensActions() {}
public:
    void setEnabledActions(function<void()> af) { _enabledactions = af; }
    virtual void enabledactions() { _enabledactions(); }
    Etyp typ() { return TRANSITION; }
    void gotEnoughTokens()
    {
        _enabledPlaceCntMutex.lock();
        _enabledPlaceCnt++;
        if(haveEnoughTokens()) _pn->addwork(_tryTriggerWork);
        else notEnoughTokensActions();
        _enabledPlaceCntMutex.unlock();
    }
    void notEnoughTokens()
    {
        _enabledPlaceCntMutex.lock();
        if( _enabledPlaceCnt > 0 ) _enabledPlaceCnt--;
        _enabledPlaceCntMutex.unlock();
    }
    DNode dnode() { return DNode(idstr(),(Proplist){{"shape","rectangle"},{"label",_name}}); }
    // Although tryTrigger is called only when preceding places have enough
    // tokens, there can be other contenders for those tokens who may consume
    // them, hence we need to check again whether this transition can fire by
    // holding all predecessor places' counts under a lock
    void tryTrigger()
    {
        Arcs::iterator it = _iarcs.begin();
        while(haveEnoughTokens())
            if(tryTransferTokens(it))
            {
                for(auto iarc:_iarcs) iarc->_place->deductactions();
                enabledactions();
                for(auto oarc:_oarcs) oarc->_place->addtokens(oarc->_wt);
            }
    }
    PNTransition(string name): PNNode(name) {}
    virtual ~PNTransition() {}
};

class PNPTArc : public PNArc
{
public:
    DEdge dedge() { return DEdge(_place->idstr(),_transition->idstr()); }
    PNPTArc(PNPlace* p, PNTransition* t, unsigned wt=1) : PNArc(p,t,wt)
    {
        _place->addoarc(this);
        _transition->addiarc(this);
    }
};

class PNTPArc : public PNArc
{
public:
    DEdge dedge() { return DEdge(_transition->idstr(),_place->idstr()); }
    PNTPArc(PNTransition* t, PNPlace* p, unsigned wt=1) : PNArc(p,t,wt)
    {
        _transition->addoarc(this);
        _place->addiarc(this);
    }
};


class PetriNet : public IPetriNet
{
    static const unsigned _defaultThreads = 4;
    Places _places;
    Transitions _transitions;
    Arcs _arcs;

    void setpn()
    {
        for(auto n:_places) n->setpn(this);
        for(auto n:_transitions) n->setpn(this);
    }
public:

    // returns intermediate node if it was inserted between PP/TT else NULL
    // adds the created arc(s) and intermediate node (if any) to Elements pnes
    // For intermediate node (if any) optional argument 'name' can be passed
    static PNNode* createArc(PNNode *n1, PNNode *n2, Elements& pnes, string name = "")
    {
        pnes.insert(n1);
        pnes.insert(n2);
        if ( n1->typ() == PNElement::TRANSITION )
        {
            if ( n2->typ() == PNElement::TRANSITION )
            {
                auto *dummy = new PNPlace(name);
                pnes.insert(dummy);
                pnes.insert(new PNTPArc((PNTransition*)n1,dummy));
                pnes.insert(new PNPTArc(dummy,(PNTransition*)n2));
                return dummy;
            }
            else
            {
                pnes.insert(new PNTPArc((PNTransition*)n1,(PNPlace*)n2));
                return NULL;
            }
        }
        else
        {
            if ( n2->typ() == PNElement::TRANSITION )
            {
                pnes.insert(new PNPTArc((PNPlace*)n1,(PNTransition*)n2));
                return NULL;
            }
            else
            {
                auto *dummy = new PNTransition(name);
                pnes.insert(dummy);
                pnes.insert(new PNPTArc((PNPlace*)n1,dummy));
                pnes.insert(new PNTPArc(dummy,(PNPlace*)n2));
                return dummy;
            }
        }
    }
    // Not a json, but json like data structure readable in python (we use
    // tuples, are not bothered about the trailing comma, can use single quoted
    // strings etc.
    void printjson(string filename="petri.json")
    {
        ofstream ofs;
        ofs.open(filename);
        ofs << "{" << endl;

        ofs << "'places' : [" << endl;
        for(auto p:_places)
        {
            ofs << "(" << p->idstr() << ",'" << p->_name << "',[";
            for(auto a:p->_oarcs) ofs << a->_transition->idstr() << ",";
            ofs << "])," << endl;
        }
        ofs << "]," << endl;

        ofs << "'transitions' : [" << endl;
        for(auto t:_transitions)
        {
            ofs << "(" << t->idstr() << ",'" << t->_name << "',[";
            for(auto a:t->_oarcs) ofs << a->_place->idstr() << ",";
            ofs << "])," << endl;
        }
        ofs << "]," << endl;

        ofs << "}" << endl;
        ofs.close();
    }
    void printdot(string filename="petri.dot")
    {
        DNodeList nl;
        for(auto n:_places) nl.push_back(n->dnode());
        for(auto n:_transitions) nl.push_back(n->dnode());
        DEdgeList el;
        for(auto e:_arcs) el.push_back(e->dedge());
        DGraph g(nl,el);
        g.printdot(filename);
    }

    // Convenience API to find and delete all elements (places, transitions,
    // arcs). Responsibility to delete is with one who does new (all
    // destructors are virtual)
    void deleteElems()
    {
        for(auto n:_places) delete n;
        for(auto n:_transitions) delete n;
        for(auto e:_arcs) delete e;
    }

    PetriNet(Places places, Transitions transitions, Arcs arcs) :
        _places(places), _transitions(transitions), _arcs(arcs)
    {
        setpn();
    }
    PetriNet(Elements elements)
    {
        for(auto e:elements)
            switch(e->typ())
            {
                case PNElement::PLACE: _places.push_back((PNPlace*)e); break;
                case PNElement::TRANSITION: _transitions.push_back((PNTransition*)e); break;
                case PNElement::ARC: _arcs.push_back((PNArc*)e); break;
            }
        setpn();
    }
};

// When QuitPlace gets a token the simulation ends
class PNQuitPlace : public PNPlace
{
public:
    void addactions() { _pn->quit(); }
    template <typename... Arg> PNQuitPlace(Arg... args) : PNPlace(args...) {}
};

// Dbg* classes for testing purposes with logging
class PNDbgPlace : public PNPlace
{
public:
    void addactions()
    {
        cout << "Place:" << idlabel() << ":added:remaining:" << _tokens << endl;
        _addactions();
    }
    void deductactions()
    {
        cout << "Place:" << idlabel() << ":deducted:remaining:" << _tokens << endl;
    }
    template <typename... Arg> PNDbgPlace(Arg... args) : PNPlace(args...) {}
};

class PNDbgTransition : public PNTransition
{
protected:
    void notEnoughTokensActions()
    {
        cout << "Not enough tokens to trigger " << idlabel() << endl;
    }
public:
    void enabledactions()
    {
        cout << "Transition:" << idlabel() << " enabled" << endl;
        _enabledactions();
    }
    template <typename... Arg> PNDbgTransition(Arg... args) : PNTransition(args...) {}
};

#endif
