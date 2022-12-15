#ifndef _PETRINET_H
#define _PETRINET_H

#include <iostream>
#include <string>
#include <list>
#include <queue>
#include <set>
#ifdef USESEQNO
#   include <atomic>
#endif
#include "dot.h"
#include "mtengine.h"

#ifdef PNDBG
#   define PNLOG(ARGS) \
    _pn->pnlogmutex.lock(); \
    _pn->pnlog << ARGS << endl; \
    _pn->pnlogmutex.unlock();
#else
#   define PNLOG(ARGS)
#endif

// Design Note: Some methods that should ideally belong to PNPlace /
// PNTransition are hosted in respective PetriNet impelementation, because
// they are specific to respective Petri net algorithm. So the behavioral
// aspects, no matter of Place / Transition are implemented in PN variants and
// rest of the classes have only structural and aspects that are agnostic to
// Petri net approach. This allows us to keep multiple Petri net
// implementation options available.

class PNArc;
class PNPlace;
class PNQuitPlace;
class PNTransition;
class PNNode;
typedef vector<PNArc*> Arcs; // a vector to aid filtering by indices
typedef set<PNPlace*> Places;
typedef set<PNTransition*> Transitions;

// Interfaces to resolve inter-dependencies
class IPetriNet : public MTEngine
{
    function<void(unsigned, unsigned long)> _eventListener;
    void setLogfile(string logfile)
    {
#ifdef PNDBG
    pnlog.open(logfile);
#endif
    }
public:
    string _netname;
#ifdef PNDBG
    ofstream pnlog;
    mutex pnlogmutex;
#endif
    unsigned _idcntr = 0;
#   ifdef USESEQNO
    atomic<unsigned long> _eseqno = 0;
#   endif
    virtual PNTransition* createTransition(string name)=0;
    virtual PNPlace* createPlace(string name, unsigned marking=0, unsigned capacity=0)=0;
    virtual PNQuitPlace* createQuitPlace(string name, unsigned marking=0, unsigned capacity=0)=0;
    virtual PNNode* createArc(PNNode *n1, PNNode *n2, string name = "", unsigned wt = 1)=0;
    virtual void printdot(string filename="petri.dot")=0;
    virtual void printpnml(string filename="petri.pnml")=0;
    virtual void deleteElems()=0;
    virtual void addtokens(PNPlace* place, unsigned newtokens)=0;
    void tellListener(unsigned e, unsigned long eseqno) { _eventListener(e, eseqno); }
    IPetriNet(string netname, function<void(unsigned,unsigned long)> eventListener) : _netname(netname), _eventListener(eventListener)
    {
        setLogfile(netname+".petri.log");
    }
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
    virtual PNNode* source()=0;
    virtual PNNode* target()=0;
    virtual DEdge dedge() = 0;
    Etyp typ() { return ARC; }
    PNArc(PNPlace* p, PNTransition* t, unsigned wt) : _place(p), _transition(t), _wt(wt) {}
    virtual ~PNArc() {}
};

class PNNode : public PNElement
{
protected:
    IPetriNet* _pn;
public:
    const unsigned _nodeid;
    Arcs _iarcs;
    Arcs _oarcs;
    const string _name;
    void addiarc(PNArc* a) { _iarcs.push_back(a); }
    void addoarc(PNArc* a) { _oarcs.push_back(a); }
    string idlabel() { return idstr() + ":" + _name; }
    string idstr() { return to_string(_nodeid); }
    PNNode(string name, IPetriNet* pn) : _name(name), _nodeid(pn->_idcntr++), _pn(pn) {}
};

class PNPlace : public PNNode
{
    function<list<int>()> _arcchooser = NULL;
    unsigned _marking;
    unsigned _capacity;
    mutex _tokenmutex;
protected:
    function<void()> _addactions = [](){};
public:
    virtual void addactions(unsigned newtokens)
    {
        PNLOG("p:" << idstr() << ":+" << newtokens << ":" << _tokens << ":" << _name)
        _addactions();
    }
    Arcs eligibleArcs()
    {
        if ( _arcchooser != NULL )
        {
            Arcs retarcs;
            list<int> arcindices = _arcchooser();
            for(auto i:arcindices)
                retarcs.push_back(_oarcs[i]);
            return retarcs;
        }
        else return _oarcs;
    }
    void setMarking(unsigned marking) { _marking = marking; }
    // This can be put on queue by adding a wrapper that does addwork, for granularity reason it wasn't
    unsigned marking() { return _marking; }
    unsigned _tokens = 0;
    Etyp typ() { return PLACE; }
    void setArcChooser(function<list<int>()> f) { _arcchooser = f; }
    void setAddActions(function<void()> af) { _addactions = af; }
    virtual void deductactions(unsigned dedtokens)
    {
        PNLOG("p:" << idstr() << ":-" << dedtokens << ":" << _tokens << ":" << _name)
    }
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
    DNode dnode() { return DNode(idstr(),(Proplist){{"label","p:"+idlabel()}}); }
    // capacity 0 means place can hold unlimited tokens
    PNPlace(string name, IPetriNet* pn, unsigned marking=0,unsigned capacity=0) : PNNode(name, pn), _capacity(capacity), _marking(marking) {}
    virtual ~PNPlace() {}
};

class PNTransition : public PNNode
{
    function<void(unsigned long)> _enabledactions = [](unsigned long){};
    function<unsigned long()> _delayfn = [](){ return 0; };
public:
    virtual void notEnoughTokensActions()
    {
        PNLOG("wait:" << idlabel())
    }
    void enabledactions(unsigned long eseqno)
    {
// the event is delivered to a listener first and then the actions attached, if any, are carried out
// Usually it is expected that the listener is some event sequence analyzer (such as a CEP tool)
// while actions may trigger state changes under the main system under simulation. This sequence ensures
// that the state changes caused by this event happen only after the listener sees the event.
#       ifdef PN_USE_EVENT_LISTENER
        _pn->tellListener(_nodeid, eseqno);
#       endif
        PNLOG("t:" << idlabel() << ":" << eseqno)
        _enabledactions(eseqno);
    }
    // Although tryTrigger is called only when preceding places have enough
    // tokens, there can be other contenders for those tokens who may consume
    // them, hence we need to check again whether this transition can fire by
    // holding all predecessor places' counts under a lock
    unsigned _enabledPlaceCnt = 0;
    mutex _enabledPlaceCntMutex;
    bool hasEnabledPlaces() { return _enabledPlaceCnt == _iarcs.size(); }
    bool mayFire()
    {
        for(auto ia:_iarcs)
            if ( ia->_place->_tokens < ia->_wt ) return false;
        return true;
    }
    void setEnabledActions(function<void(unsigned long)> af) { _enabledactions = af; }
    void setDelayFn( function<unsigned long()> df ) { _delayfn = df; }
    unsigned long delay() { return _delayfn(); }
    Etyp typ() { return TRANSITION; }
    DNode dnode() { return DNode(idstr(),(Proplist){{"shape","rectangle"},{"label","t:"+idlabel()}}); }
    PNTransition(string name, IPetriNet *pn): PNNode(name, pn) {}
    virtual ~PNTransition() {}
};

class PNPTArc : public PNArc
{
public:
    PNNode* source() { return _place; }
    PNNode* target() { return _transition; }
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
    PNNode* source() { return _transition; }
    PNNode* target() { return _place; }
    DEdge dedge() { return DEdge(_transition->idstr(),_place->idstr()); }
    PNTPArc(PNTransition* t, PNPlace* p, unsigned wt=1) : PNArc(p,t,wt)
    {
        _transition->addoarc(this);
        _place->addiarc(this);
    }
};

// When QuitPlace gets a token the simulation ends
class PNQuitPlace : public PNPlace
{
using PNPlace::PNPlace;
public:
    void addactions(unsigned) { _pn->quit(); }
};

class PetriNetBase : public IPetriNet
{
    void assertPlacePresent(PNPlace* n)
    {
        if ( _places.find(n) == _places.end() )
        {
            cout << "Place not found: " << n->_name << endl;
            exit(1);
        }
    }
    void assertTransitionPresent(PNTransition* n)
    {
        if ( _transitions.find(n) == _transitions.end() )
        {
            cout << "Transition not found: " << n->_name << endl;
            exit(1);
        }
    }
protected:
    Places _places;
    Transitions _transitions;
    Arcs _arcs;
    virtual void _postinit() {}
    // Note: Fire is to be called after deducting tokens from sources
    // it will add tokens to destinations
    void fire(PNTransition* t)
    {
        for(auto iarc:t->_iarcs) iarc->_place->deductactions(iarc->_wt);
#       ifdef USESEQNO
        t->enabledactions ( _eseqno++ );
#       else
        t->enabledactions ( 0 );
#       endif
        for(auto oarc:t->_oarcs) addtokens(oarc->_place, oarc->_wt);
    }
public:
    PNTransition* createTransition(string name)
    {
        auto t = new PNTransition(name, this);
        _transitions.insert(t);
        return t;
    }
    PNPlace* createPlace(string name, unsigned marking=0, unsigned capacity=0)
    {
        auto p = new PNPlace(name, this, marking, capacity);
        _places.insert(p);
        return p;
    }
    PNQuitPlace* createQuitPlace(string name, unsigned marking=0, unsigned capacity=0)
    {
        auto p = new PNQuitPlace(name, this, marking, capacity);
        _places.insert(p);
        return p;
    }
    // returns intermediate node if it was inserted between PP/TT else NULL
    // adds the created arc(s) and intermediate node (if any) to Elements pnes
    // For intermediate node (if any i.e. for PP/TT arguments) optional
    // argument 'name' can be passed
    // Optional argument 'wt' is used to assign weight to arc only for PT/TP
    // arguments i.e. only when a single arc is created
    PNNode* createArc(PNNode *n1, PNNode *n2, string name = "", unsigned wt = 1)
    {
        if ( n1->typ() == PNElement::TRANSITION )
        {
            assertTransitionPresent((PNTransition*)n1);
            if ( n2->typ() == PNElement::TRANSITION )
            {
                assertTransitionPresent((PNTransition*)n2);
                auto *dummy = createPlace(name);
                _arcs.push_back(new PNTPArc((PNTransition*)n1,dummy));
                _arcs.push_back(new PNPTArc(dummy,(PNTransition*)n2));
                return dummy;
            }
            else
            {
                assertPlacePresent((PNPlace*)n2);
                _arcs.push_back(new PNTPArc((PNTransition*)n1,(PNPlace*)n2,wt));
                return NULL;
            }
        }
        else
        {
            assertPlacePresent((PNPlace*)n1);
            if ( n2->typ() == PNElement::TRANSITION )
            {
                assertTransitionPresent((PNTransition*)n2);
                _arcs.push_back(new PNPTArc((PNPlace*)n1,(PNTransition*)n2,wt));
                return NULL;
            }
            else
            {
                assertPlacePresent((PNPlace*)n2);
                auto *dummy = createTransition(name);
                _arcs.push_back(new PNPTArc((PNPlace*)n1,dummy));
                _arcs.push_back(new PNTPArc(dummy,(PNPlace*)n2));
                return dummy;
            }
        }
    }
    void printpnml(string filename="petri.pnml")
    {
        ofstream ofs;
        ofs.open(filename);
        ofs << "<?xml version=\"1.0\"?>" << endl;
        ofs << "<pnml xmlns=\"http://www.pnml.org/version-2009/grammar/pnml\">" << endl;
        for(auto p:_places)
        {
            ofs << "<place id=\"" << p->idstr() << "\">";
            ofs << "<name><text>" << p->_name << "</text></name>";
            auto marking = p->marking();
            if( marking )
                ofs << "<initialMarking><text>" << marking << "</text></initialMarking>";
            ofs << "</place>" << endl;
        }
        for(auto t:_transitions)
        {
            ofs << "<transition id=\"" << t->idstr() << "\">";
            ofs << "<name><text>" << t->_name << "</text></name>";
            ofs << "</transition>" << endl;
        }
        unsigned tmparcid=0;
        for(auto e:_arcs)
            ofs << "<arc id=\"" << tmparcid++ << "\" source=\"" << e->source()->idstr() << "\" target=\"" << e->target()->idstr() << "\"/>" << endl;
        ofs << "</pnml>" << endl;
        ofs.close();
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
            ofs << "(" << p->idstr() << ",'" << p->_name << "'," << p->marking() << ",[";
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
    void printMarkings()
    {
        for(auto p:_places) if ( p->_tokens )
            cout << "MARKING:" << p->idlabel() << ":" << p->_tokens << endl;
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
    void init()
    {
        for(auto p:_places)
            if ( p->marking() )
                addtokens(p, p->marking());
        _postinit();
    }
    PetriNetBase(string netname = "system", function<void(unsigned, unsigned long)> eventListener = [](unsigned, unsigned long){})
        : IPetriNet(netname, eventListener)
    {}
};

class MTPetriNet : public PetriNetBase
{
using PetriNetBase::PetriNetBase;

// Transition methods (See Design note)
    void tryTrigger(PNTransition *transition)
    {
        Arcs::iterator it = transition->_iarcs.begin();
        while(transition->hasEnabledPlaces())
            if(tryTransferTokens(transition,it)) fire(transition);
    }
    // Recursive walk helps keep it simple to avoid locking input places in
    // case previous ones do not meet the criteria
    bool tryTransferTokens(PNTransition* transition, Arcs::iterator it)
    {
        if(it==transition->_iarcs.end()) return true;
        auto ptarc = *it;
        if(ptarc->_place->lockIfEnough(ptarc->_wt))
        {
            if(tryTransferTokens(transition,++it))
            {
                deducttokens(ptarc->_place, ptarc->_wt);
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
    void gotEnoughTokens(PNTransition* transition)
    {
        transition->_enabledPlaceCntMutex.lock();
        transition->_enabledPlaceCnt++;
        Work tryTriggerWrok = bind(&MTPetriNet::tryTrigger,this,transition);
        if(transition->hasEnabledPlaces()) addwork(tryTriggerWrok);
        else transition->notEnoughTokensActions();
        transition->_enabledPlaceCntMutex.unlock();
    }
    void notEnoughTokens(PNTransition* transition)
    {
        transition->_enabledPlaceCntMutex.lock();
        if( transition->_enabledPlaceCnt > 0 ) transition->_enabledPlaceCnt--;
        transition->_enabledPlaceCntMutex.unlock();
    }
// Place methods (See Design note)
    // deducttokens Expects caller to have taken care of locking
    void deducttokens(PNPlace* place, unsigned tokens)
    {
        unsigned oldcnt = place->_tokens;
        place->_tokens -= tokens;
        for(auto oarc:place->_oarcs)
            // inform the transition only if we went below the threshold now
            if( place->_tokens < oarc->_wt && oldcnt >= oarc->_wt )
                notEnoughTokens((PNTransition*)oarc->_transition);
    }
public:
    void addtokens(PNPlace* place, unsigned newtokens)
    {
        Arcs eligibleArcs = place->eligibleArcs();
        place->lock();
        unsigned oldcnt = place->_tokens;
        place->_tokens += newtokens;
        for(auto oarc:eligibleArcs)
            // inform the transition only if we crossed the threshold now
            // Think, whether we want to randomize the sequence of oarc for non
            // determinism Of course, without it also the behavior is correct,
            // since a deterministic sequence is a subset of possible non
            // deterministic behaviors anyway.
            if( place->_tokens >= oarc->_wt && oldcnt < oarc->_wt )
                gotEnoughTokens((PNTransition*)oarc->_transition);
        place->unlock();
        place->addactions(newtokens);
    }
};

// TODO: Decide how to use multiple cores for STPN. Either start multiple
// simulations over threads or let the user start multiple processes. Log
// clashes should be avoided when doing so.
class STPetriNet : public PetriNetBase
{
using PetriNetBase::PetriNetBase;
using t_pair  = pair<double, PNTransition*>;
// Saves the overhead of comparing 2nd member of the pair
class PriorityLT
{
public:
    // Since delay is opposite of priority, we use >
    bool operator() (t_pair& l, t_pair& r) { return l.first > r.first; }
};
using t_queue = priority_queue<t_pair, vector<t_pair>, PriorityLT>;

    t_queue _tq;
// An experimental (undocumented) option to adjust the delay with the running time
// But this has undesirable side effecto of reducing the defect detection probabilities
#ifdef USE_DELAY_OFFSET
    double _offset = 0;
#endif
    mutex _tqmutex;
    condition_variable _tq_cvar;

    void _simuloop()
    {
        while ( true )
        {
            _tqmutex.lock();
            if ( _tq.empty() )
            {
                _tqmutex.unlock();
                break;
            }
#ifdef USE_DELAY_OFFSET
            _offset += _tq.top().first;
#endif
            auto t = _tq.top().second;
            _tq.pop();
            _tqmutex.unlock();
            // this was checked when adding to _tq, but marking may change
            // till its turn comes, so check again
            if ( t->mayFire() )
            {
                for(auto ia:t->_iarcs)
                {
                    auto p = ia->_place;
                    p->lock();
                    p->_tokens -= ia->_wt;
                    p->unlock();
                }
                fire(t);
            }
        }
    }
    void simuloop()
    {
        while ( not _quit )
        {
            _simuloop();
            unique_lock<mutex> ulockq(_tqmutex);
            _tq_cvar.wait(ulockq, [](){return true;});
        }
    }
    void _postinit()
    {
        Work w = bind(&STPetriNet::simuloop,this);
        addwork(w);
    }
public:
    void addtokens(PNPlace* place, unsigned newtokens)
    {
        place->lock();
        place->_tokens += newtokens;
        place->unlock();
        place->addactions(newtokens);
        Arcs eligibleArcs = place->eligibleArcs();
        for(auto oarc:eligibleArcs)
        {
            auto t = oarc->_transition;
            if ( t->mayFire() )
            {
                _tqmutex.lock();
#ifdef USE_DELAY_OFFSET
                _tq.push( { t->delay() + _offset, t } );
#else
                _tq.push( { t->delay(), t } );
#endif
                _tqmutex.unlock();
            }
        }
        unique_lock<mutex> ulockq(_tqmutex);
        _tq_cvar.notify_one();
    }
};

#endif
