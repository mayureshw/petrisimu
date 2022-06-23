class PetriNet:
    # seeds : set of seeds for transitive closure
    # stopset : the walk would not traverse BEYOND these
    # excludeset : the walk would not traverse TO these
    # WARNING: This is just a graph reduction technique for ease of
    # comprehension. This is not guaranteed to be semantics preserving (e.g.
    # there may be multiple predecessor to a transition, some of which may be
    # hidden due to excludeset etc.). Always correlate your understanding with
    # the whole graph.
    def slice(self,seeds,rel,stopset,excludeset):
        closure = seeds
        newnodes = seeds
        while True:
            newnodes = { newn for n in newnodes if n not in stopset for newn in rel.get(n,[]) if newn not in closure and newn not in excludeset }
            if len(newnodes) == 0 : break
            closure = closure.union(newnodes)
        return closure

    def fwdslice(self,seeds,stopset=set(),excludeset=set()): return self.slice(seeds,self.succ,stopset,excludeset)

    def bwdslice(self,seeds,stopset=set(),excludeset=set()): return self.slice(seeds,self.pred,stopset,excludeset)

    def chop(self,fwdseeds,bwdseeds,fwdstopset=set(),bwdstopset=set(),excludeset=set()):
        return self.bwdslice(bwdseeds,bwdstopset.union(fwdseeds),excludeset).intersection(
            self.fwdslice(fwdseeds,fwdstopset.union(bwdseeds),excludeset))

    def logtrace(self,quant,ns,indent,nlabel): print('\t'*indent + quant, ' '.join(self.tracelabel(n) for n in ns),nlabel)

    def trace(self,n,relf,stopset,excludeset,visited=set(),indent=0):
        newvisited = visited.union({n})
        if n in stopset: return newvisited
        nlabel = self.labels[n]
        if n in visited:
            self.logtrace(str(n) + ': VISITED', [] , indent,nlabel)
            return newvisited
        preds = relf(n)
        quant = str(n) + ': ' + ( 'ANYOF' if self.isplace(n) else 'ALLOF' ) + '(' + str(len(preds)) + ')'
        self.logtrace(quant,preds,indent,nlabel)
        for p in preds:
            newvisited = newvisited.union (
                self.trace(p,relf,stopset,excludeset,newvisited,indent+1) )
        return newvisited

    def tracebwd(self,seed,stopset=set(),excludeset=set(),skiptrivial=True,retainset=set()):
        self.trace(seed,lambda n:self.predecessors(n,skiptrivial,retainset),stopset,excludeset)

    def tracefwd(self,seed,stopset=set(),excludeset=set(),skiptrivial=True,retainset=set()):
        self.trace(seed,lambda n:self.successors(n,skiptrivial,retainset),stopset,excludeset)

    def isplace(self,p): return p in self.places

    def istransition(self,t): return t in self.transitions

    # TODO: Currently no way to identify marked places, need it in petri.json dumped
    def trivialplace(self,p): return self.isplace(p) and len(self.succ.get(p,[])) == 1 and len(self.pred.get(p,[])) == 1
    def trivialnode(self,n): return len(self.succ.get(n,[])) == 1 and len(self.pred.get(n,[])) == 1

    def ntneighbor(self,n,rel,retainset): return n if not self.trivialnode(n) or n in retainset else self.ntneighbor(rel[n][0],rel,retainset)

    def neighbors(self,n,rel,skiptrivial,retainset):
        succs = rel.get(n,[])
        return succs if not skiptrivial else [
            self.ntneighbor(s,rel,retainset) for s in succs ]

    def successors(self,n,skiptrivial=True,retainset=set()): return self.neighbors(n,self.succ,skiptrivial,retainset)
    def predecessors(self,n,skiptrivial=True,retainset=set()): return self.neighbors(n,self.pred,skiptrivial,retainset)

    def idlabel(self,n): return str(n) + ':' + self.labels[n]
    def tracelabel(self,n): return ( 'p' if self.isplace(n) else 't' ) + ':' + str(n)
    def printdot(self,nodes=None,flnm='slice.dot',highlight=set(),skiptrivial=True):
        slicenodes = nodes if nodes != None else self.nodes
        skippednodes = { n for n in slicenodes if not self.trivialnode(n) or n in highlight } if skiptrivial else slicenodes
        fp = open(flnm,'w')
        print('digraph {',file=fp)
        for n in skippednodes:
            label='label="'+self.idlabel(n)+'"'
            shape = 'shape=rectangle' if n in self.transitions else ''
            style = 'style=filled' if n in highlight else ''
            props = [shape, style, label]
            propstr = '[' + ','.join(p for p in props if p != '') + ']'
            print(n, propstr, file=fp)
            for s in self.successors(n,skiptrivial,highlight):
                if s in skippednodes: print(n,'->',s,file=fp)
        print('}',file=fp)

    def __init__(self,flnm):
        self.pn = eval( open(flnm).read() )
        self.places = { pid for (pid,_,_) in self.pn['places'] }
        self.transitions = { tid for (tid,_,_) in self.pn['transitions'] }
        self.nodes = self.places.union(self.transitions)
        self.labels = { id:lbl for (id,lbl,_) in self.pn['places'] + self.pn['transitions'] }
        self.succ = { pid:succ for (pid,_,succ) in self.pn['places'] + self.pn['transitions'] }
        self.pred = {}
        for (pred,succs) in self.succ.items():
            for succ in succs:
                self.pred[succ] = self.pred.get(succ,[]) + [pred]
