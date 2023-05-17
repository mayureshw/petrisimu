// Utility classes to make it easy to dump a C++ data structure in json format

#ifndef _JSONPRINTER_H
#define _JSONPRINTER_H

using namespace std;

#include <iostream>
#include <string>
#include <list>
#include <map>

// Base class for Json objects
class JsonObj
{
public:
    virtual void print(ostream& ostr = cout) = 0;
    virtual ~JsonObj() {}
};

// Class to represent atomic objects of type T, e.g. T = int, float, string etc.
template<typename T> class JsonAtom : public JsonObj
{
    T _val;
public:
    void print(ostream& ostr = cout)
    {
        if constexpr ( is_same<T,string>::value )
            ostr << "\"" << _val << "\"";
        else ostr << _val;
    }
    JsonAtom(T val) : _val(val) {}
};


// Class to represent json lists
class JsonList :  public JsonObj, public list<JsonObj*>
{
using list<JsonObj*>::list;
public:
    void print(ostream& ostr = cout)
    {
        ostr << "[ ";
        string delim = "";
        for(auto e:*this)
        {
            ostr << delim;
            e->print(ostr);
            delim = ", ";
        }
        ostr << " ]";
    }
};

// Class to represent json maps (aka dictionaries
class JsonMap : public JsonObj, public list<pair<JsonObj*,JsonObj*>>
{
using list<pair<JsonObj*,JsonObj*>>::list;
public:
    void print(ostream& ostr = cout)
    {
        ostr << "{ ";
        string delim = "";
        for(auto p:*this)
        {
            ostr << delim;
            p.first->print(ostr);
            ostr << " : ";
            p.second->print(ostr);
            delim = ", ";
        }
        ostr << " }";
    }
};

// JsonFactory provides convenience methods to create lists and maps of JsonObj
// of atomic types. Their memory management is handled by JsonFactory
// Applications can instantiate JsonObj subclasses directly also, in which case
// they have to do the memory management.
class JsonFactory
{
    list<JsonObj*> _objs;
public:
    template<typename T> JsonAtom<T>* createJsonAtom(T val)
    {
        auto retatom = new JsonAtom<T>(val);
        _objs.push_back(retatom);
        return retatom;
    }
    JsonList* createJsonList()
    {
        auto retlist = new JsonList();
        _objs.push_back(retlist);
        return retlist;
    }
    JsonMap* createJsonMap()
    {
        auto retmap = new JsonMap();
        _objs.push_back(retmap);
        return retmap;
    }
    template<typename T1, typename T2> JsonMap* createAtomPairMap(list<pair<T1,T2>> pairs)
    {
        auto retmap = createJsonMap();
        for(auto p:pairs)
        {
            auto k = new JsonAtom<T1>(p.first);
            auto v = new JsonAtom<T2>(p.second);
            _objs.push_back(k);
            _objs.push_back(v);
            retmap->push_back({k,v});
        }
        return retmap;
    }
    template<typename T> JsonList* createAtomList(list<T> vals)
    {
        auto retlist = createJsonList();
        for(auto v: vals)
        {
            auto o = new JsonAtom<T>(v);
            _objs.push_back(o);
            retlist->push_back(o);
        }
        return retlist;
    }
    ~JsonFactory() { for(auto o:_objs) delete o; }
};

#endif
