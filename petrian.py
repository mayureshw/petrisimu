class PetriNet:
    def slice(self,seeds,stopset,rel):
        closure = seeds
        newnodes = seeds
        while True:
            newnodes = { newn for n in newnodes if n not in stopset for newn in rel.get(n,[]) if newn not in closure }
            if len(newnodes) == 0 : break
            closure = closure.union(newnodes)
        return closure

    def fwdslice(self,seeds,stopset=set()): return self.slice(seeds,stopset,self.succ)

    def bwdslice(self,seeds,stopset=set()): return self.slice(seeds,stopset,self.pred)

    def chop(self,fwdseeds,bwdseeds,fwdstopset=set(),bwdstopset=set()):
        return self.bwdslice(bwdseeds,bwdstopset).intersection(self.fwdslice(fwdseeds,fwdstopset))

    def printdot(self,nodes=None,flnm='slice.dot'):
        slicenodes = nodes if nodes != None else self.nodes
        fp = open(flnm,'w')
        print('digraph {',file=fp)
        for n in slicenodes:
            shape = 'shape=rectangle' if n in self.transitions else ''
            print(n, '[label=','"'+self.labels[n]+'"', ',',shape,']',file=fp)
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
