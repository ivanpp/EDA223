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
#define DETECTION_INTERVAL 2
#define DETECTION_ANSWER_DEADLINE 2

typedef struct{
    Object super;
    const int rank; // my rank
    int numNodes;
    int nodes[MAX_NODES];
    int nodeStatus[MAX_NODES];
    int conductorRank;
    int lock;
    int vote;
    Msg detectMsg;
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


/* Join Network*/
void claim_existence(Network*, int);
int search_network(Network*, int);
void handle_join_request(Network*, int);
void add_node_ascending(Network*, int);
/* Conductorship */
void claim_conductorship(Network*, int);
void handle_claim_request(Network*, int);
void handle_answer_to_claim(Network*, int);
void obtain_conductorship(Network*, int);
void change_conductor(Network *, int);
/* Lock */
void reset_lock(Network *, int);
/* Node Status */
    /* detection */
void detect_all_nodes(Network *, int);
void detect_node(Network *, int);
void answer_detect_node(Network *, int);
void resolve_detect_node(Network *, int);
void notify_node_offline(Network *, int);
void set_node_offline(Network *, int);
    /* recover */
void handle_login_request(Network *, int);
void node_login(Network *, int);
void node_logout(Network *, int);
int check_self_login(Network *, int);
/* Utils */
int get_node_index(Network *, int);
int get_node_by_index(Network *, int);
int get_next_node(Network *, int);
int get_next_valid_node(Network *, int);
int get_prev_valid_node(Network *, int);
int get_first_valid_node(Network *, int);
int count_valid_voters(Network *, int);
/* Information */
void print_network(Network*, int);
void print_membership(Network *, int);
void print_network_verbose(Network *, int);
/* Testing */
void test_compete_conductor(Network *, int);
void test_reset_condutor(Network *, int);

#endif