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

    def printdot(self,nodes=None,flnm='slice.dot',highlight=set()):
        slicenodes = nodes if nodes != None else self.nodes
        fp = open(flnm,'w')
        print('digraph {',file=fp)
        for n in slicenodes:
            label='label="'+self.labels[n]+'"'
            shape = 'shape=rectangle' if n in self.transitions else ''
            style = 'style=filled' if n in highlight else ''
            props = [shape, style, label]
            propstr = '[' + ','.join(p for p in props if p != '') + ']'
            print(n, propstr, file=fp)
            for s in self.succ.get(n,[]):
                if s in slicenodes: print(n,'->',s,file=fp)
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
