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

    # TODO: Currently no way to identify marked places, need it in petri.json dumped
    def trivialplace(self,p): return p in self.places and len(self.succ.get(p,[])) == 1 and len(self.pred.get(p,[])) == 1

    def successors(self,n,skiptrivial=True,retainset=set()):
        succs = self.succ.get(n,[])
        return succs if n in self.places or not skiptrivial else [
            ( self.succ[p][0] if self.trivialplace(p) and p not in retainset else p ) for p in succs ]

    def printdot(self,nodes=None,flnm='slice.dot',highlight=set(),skiptrivial=True):
        slicenodes = nodes if nodes != None else self.nodes
        skippednodes = { n for n in slicenodes if not self.trivialplace(n) or n in highlight } if skiptrivial else slicenodes
        fp = open(flnm,'w')
        print('digraph {',file=fp)
        for n in skippednodes:
            label='label="'+str(n)+':'+self.labels[n]+'"'
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
