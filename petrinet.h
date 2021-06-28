#ifndef _PETRINET_H
#define _PETRINET_H

#include <iostream>
#include <string>
#include <list>
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
typedef list<PNArc*> Arcs;
typedef list<PNPlace*> Places;
typedef list<PNTransition*> Transitions;

// Interfaces to resolve inter-dependencies
class IPetriNet : public MTEngine
{
public:
    IPetriNet(unsigned nThreads) : MTEngine(nThreads) {}
};

class IPNTransition
{
public:
    virtual void gotEnoughTokens()=0;
    virtual void notEnoughTokens()=0;
};


class PNArc
{
public:
    PNPlace* _place;
    PNTransition* _transition;
    unsigned _wt;
    virtual DEdge dedge() = 0;
    PNArc(PNPlace* p, PNTransition* t, unsigned wt) : _place(p), _transition(t), _wt(wt) {}
    virtual ~PNArc() {}
};

class PNNode
{
    queue<Work> _q;
    mutex _q_mutex;
    bool soughtslot = false;
    void dowork()
    {
        while(true)
        {
            Work work;
            {
                const lock_guard<mutex> lockq(_q_mutex);
                bool qempty = _q.empty();
                if(qempty)
                {
                    soughtslot = false;
                    break;
                }
                else
                {
                    work = _q.front();
                    _q.pop();
                }
            }
            work();
        }
    }
public:
    IPetriNet* _pn;
    Arcs _iarcs;
    Arcs _oarcs;
    void addwork(Work work)
    {
        const lock_guard<mutex> lockq(_q_mutex);
        _q.push(work);
        if (!soughtslot)
        {
            _pn->addwork(bind(&PNNode::dowork,this));
            soughtslot = true;
        }
    }
    const string _name;
    void setpn(IPetriNet* pn) { _pn = pn; }
    void addiarc(PNArc* a) { _iarcs.push_back(a); }
    void addoarc(PNArc* a) { _oarcs.push_back(a); }
    PNNode(string name) : _name(name) {}
};

class PNPlace : public PNNode
{
    unsigned _capacity;
    mutex _tokenmutex;
    void _addtokens(unsigned newtokens)
    {
        lock();
        unsigned oldcnt = _tokens;
        _tokens += newtokens;
        for(auto oarc:_oarcs)
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
public:
    unsigned _tokens = 0;
    virtual void addactions() {}
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
    void addtokens(unsigned newtokens) { addwork(bind(&PNPlace::_addtokens,this,newtokens)); }
    DNode dnode() { return DNode(_name); }
    // capacity 0 means place can hold unlimited tokens
    PNPlace(string name,unsigned capacity=0) : PNNode(name), _capacity(capacity) {}
    virtual ~PNPlace() {}
};

class PNTransition : public PNNode, IPNTransition
{
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
public:
    virtual void enabledactions() {}
    void gotEnoughTokens()
    {
        _enabledPlaceCntMutex.lock();
        _enabledPlaceCnt++;
        if(haveEnoughTokens()) addwork(bind(&PNTransition::tryTrigger,this));
        _enabledPlaceCntMutex.unlock();
    }
    void notEnoughTokens()
    {
        _enabledPlaceCntMutex.lock();
        _enabledPlaceCnt--;
        _enabledPlaceCntMutex.unlock();
    }
    DNode dnode() { return DNode(_name,(Proplist){{"shape","rectangle"}}); }
    // Although tryTrigger is called only when preceding places have enough
    // takens, there can be other contenders for those tokens who may consume
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
    DEdge dedge() { return DEdge(_place->_name,_transition->_name); }
    PNPTArc(PNPlace* p, PNTransition* t, unsigned wt=1) : PNArc(p,t,wt)
    {
        _place->addoarc(this);
        _transition->addiarc(this);
    }
};

class PNTPArc : public PNArc
{
public:
    DEdge dedge() { return DEdge(_transition->_name,_place->_name); }
    PNTPArc(PNTransition* t, PNPlace* p, unsigned wt=1) : PNArc(p,t,wt)
    {
        _transition->addoarc(this);
        _place->addiarc(this);
    }
};


class PetriNet : public IPetriNet
{
    Places _places;
    Transitions _transitions;
    Arcs _arcs;

public:

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

    PetriNet(Places places, Transitions transitions, Arcs arcs, unsigned nThreads=4) :
        _places(places), _transitions(transitions), _arcs(arcs), IPetriNet(nThreads)
    {
        for(auto n:_places) n->setpn(this);
        for(auto n:_transitions) n->setpn(this);
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
        cout << "Place " << _name << " got tokens, current tokens: " << _tokens << endl;
    }
    void deductactions()
    {
        cout << "Place " << _name << " consumed tokens, current tokens: " << _tokens << endl;
    }
    template <typename... Arg> PNDbgPlace(Arg... args) : PNPlace(args...) {}
};

class PNDbgTransition : public PNTransition
{
public:
    void enabledactions()
    {
        cout << "Transition " << _name << " enabled" << endl;
    }
    template <typename... Arg> PNDbgTransition(Arg... args) : PNTransition(args...) {}
};

#endif