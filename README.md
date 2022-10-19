# petrisimu
A C++ based multithreaded petri net simulator

## Petri net features supported

    - Arc weights

## Petri net features not supported

    - Place capacity
    - Inhibitor arcs
    - Hierarchical Petri nets

## Export formats supported

    - pnml
    - json
    - graphviz dot

## Performance tuning options

    Following environment variables can be tuned for better performance

        NTHREADS    Number of CPU threads used, default 1, typically set it to number of available cores

Note: The LQTHRESHOLD heuristic is temporarily switched off for some issues documented in mtengine.h

        LQTHRESHOLD Local queue size of a thread beyond which it throws work items on global queue. Default 2.
                    Setting higher value reduces thread contention as thread count grows.

## Optional features : logs and interfaces

    Compilation flags and features they enable

        PNDBG   If set, generates petri.log which logs each transition, each
                addition and deduction of tokens to places and `wait' events where transitions
                could not fire (useful to identify deadlock points)

        PN_USE_EVENT_LISTENER   If set, each transition is sent to an
                                eventListener. The eventListener is an optional
                                argument of PetriNet class.

## Installation

This is header-only library. Application just needs to include "petrinet.h". A
systematic way to make the include path available to the application please do
the following:

    1. Define the following environment variable preferably in your shell's rc
       file:

            PETRISIMUDIR    Directory where you have checked out this code

    2. Include Makefile.petrisimu in the Makefile of your application
       E.g.
            include $(PETRISIMUDIR)/Makefile.petrisimu

Exactly 1 of the c++ source files in the application needs to declare a static
variable needed, which is simply by invoking the following macro in global
scope.

    PETRINET_STATICS

## Sample usage

See examples directory.
