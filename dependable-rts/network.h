#ifndef NETWORK_H
#define NETWORK_H

#include <stdio.h>
#include "TinyTimber.h"
#include "application.h"
#include "systemPorts.h"

#define MAX_NODES 8
#ifndef RANK
    #define RANK 1
#endif
#define NO_CONDUCTOR 0

typedef struct{
    Object super;
    const int rank; // my rank
    int numNodes;
    int nodes[MAX_NODES];
    int conductorRank;
    int lock;
    int vote;
} Network;

#define initNetwork(rank) { \
    initObject(), \
    rank, \
    1, \
    {rank}, \
    NO_CONDUCTOR, \
    0, \
    1, \
}

extern Network network;

void claimExistence(Network*, int);
int searchNetwork(Network*, int);
void handleJoinRequest(Network*, int);
void addNodeAscending(Network*, int);
int sortNetwork(Network*, int);
void printNetwork(Network*, int);
void claimConductorship(Network*, int);
void handleConductorshipRequest(Network*, int);
void handleConductorshipAck(Network*, int);
void obtainConductorship(Network*, int);
void changeConductor(Network *, int);
void printNetworkVerbose(Network *, int);

#endif