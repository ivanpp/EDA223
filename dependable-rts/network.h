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
#define NODE_ONLINE 0
#define NODE_OFFLINE 1

typedef struct{
    Object super;
    const int rank; // my rank
    int numNodes;
    int nodes[MAX_NODES];
    int nodeStatus[MAX_NODES];
    int conductorRank;
    int lock;
    int vote;
} Network;

#define initNetwork(rank) { \
    initObject(), \
    rank, \
    1, \
    {rank}, \
    {NODE_ONLINE}, \
    NO_CONDUCTOR, \
    0, \
    1, \
}

extern Network network;

void claim_existence(Network*, int);
int search_network(Network*, int);
void handle_join_request(Network*, int);
void add_node_ascending(Network*, int);
int sort_network(Network*, int);
/* Conductorship */
void claim_conductorship(Network*, int);
void handle_claim_request(Network*, int);
void handle_answer_to_claim(Network*, int);
void obtain_conductorship(Network*, int);
void change_conductor(Network *, int);
/* Lock */
void reset_lock(Network *, int);
/* Utils */
int get_node_index(Network *, int);
int get_next_node(Network *, int);
/* Information */
void print_network(Network*, int);
void print_membership(Network *, int);
void print_network_verbose(Network *, int);
/* Testing */
void test_compete_conductor(Network *, int);
void test_reset_condutor(Network *, int);

#endif