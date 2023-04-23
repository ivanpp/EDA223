#include "network.h"


Network network = initNetwork(RANK);


// search for existing network by broadcasting a CANMsg
int search_network(Network *self, int unused){
    SCI_WRITE(&sci0, "[NETWORK]: Searching other networks\n");
    CANMsg msg;
    constructCanMessage(&msg, SEARCH_NETWORK, BROADCAST, self->rank);
    CAN_SEND_WR(&can0, &msg); // >> handle_join_request(sender)
    return 0;
}


// to handle join request, with safety check
void handle_join_request(Network *self, int sender){
    if (self->numNodes == MAX_NODES){
        SCI_WRITE(&sci0, "[NETWORK]: MAX nodes in network already achieved\n");
        return;
    }
    if (sender == self->rank) {
#ifdef CAN_LOOPBACK
        SCI_WRITE(&sci0, "[NETWORK]: Join request ignored for LOOPBACK mode\n");
        return;
#else
        SCI_WRITE(&sci0, "[NETWORK ERR]: Node with same rank wants to join\n");
#endif
    } else {
        // check if already joined
        for (size_t i = 0; i < self->numNodes; i++){
            if (self->nodes[i] == sender){
                char debugInfo[48];
                snprintf(debugInfo, 48, "[NETWORK]: Node %d already in netwrok\n", sender);
                SCI_WRITE(&sci0, debugInfo);
                return;
            }
        }
        // confirm joining, force order
        add_node_ascending(self, sender);
        // claim existence to sender too
        claim_existence(self, sender);
        // print network
        print_network(self, 0);
    }
}


// add a node into the network
// NOTE: no safety check in this method, should be applied before calling
void add_node_ascending(Network *self, int sender){
    char nodeInfo[32] = {};
    for(size_t i = self->numNodes; i > 0; i--){
        if (sender < self->nodes[i-1]){
            self->nodes[i] = self->nodes[i-1];
        }
        else{
            self->nodes[i] = sender;
            self->nodeStatus[i] = NODE_ONLINE;
            self->numNodes++;
            snprintf(nodeInfo, 32, "[NETWORK]: node %d added\n", sender);
            SCI_WRITE(&sci0, nodeInfo);
            return;
        }
    }
    self->nodes[0] = sender;
    self->nodeStatus[0] = NODE_ONLINE;
    self->numNodes++;
    snprintf(nodeInfo, 32, "[NETWORK]: node %d added\n", sender);
    SCI_WRITE(&sci0, nodeInfo);
    return;
}


void claim_existence(Network *self, int receiver){
    SCI_WRITE(&sci0, "[NETWORK]: Try to claim my Existence\n");
    CANMsg msg;
    constructCanMessage(&msg, CLAIM_EXISTENCE, receiver, self->rank);
    CAN_SEND_WR(&can0, &msg); // >> handle_join_request(sender)
}


// sort the nodes of the network by rank, no need if we force the order
int sort_network(Network *self, int unused){
    return 0;
}


/* Conductorship */

void claim_conductorship(Network *self, int unused){
    if (self->conductorRank == self->rank){
        SCI_WRITE(&sci0, "[NETWORK]: Already have conductorship\n");
        return;
    }
#ifdef CAN_LOOPBACK
    obtain_conductorship(self, 0);
    return;
#endif
    self->lock = self->rank; // lock self
    SCI_WRITE(&sci0, "[NETWORK]: Try to claim my conductorship\n");
    CANMsg msg;
    constructCanMessage(&msg, CLAIM_CONDUCTORSHIP, BROADCAST, 0);
    if (CAN_SEND_WRN(&can0, &msg, 2)){
        SCI_WRITE(&sci0, "[NETWORK]: reset lock because can fail\n");
        reset_lock(self, unused);
    }
}


void handle_claim_request(Network *self, int sender){
    CANMsg msg;
    if(self->lock < sender){
        self->lock = sender; // lock acquired by conductor
        constructCanMessage(&msg, ANSWER_CLAIM_CONDUCTOR, sender, 1); // agree
    } else {
        char lockInfo[64];
        snprintf(lockInfo, 64, "[LOCK]: My lock is already acquired by: %d\n", self->lock);
        SCI_WRITE(&sci0, lockInfo);
        constructCanMessage(&msg, ANSWER_CLAIM_CONDUCTOR, sender, 0); // disagree
    }
    CAN_SEND_WRN(&can0, &msg, 5); // >> handle_answer_to_claim(answer)
}


void handle_answer_to_claim(Network *self, int agree){
    if(agree){
        if(self->vote == self->numNodes - 1){ // all other agree
            obtain_conductorship(self, 0);
        }else
            self->vote++;
    } else{ // disagree
        self->lock = 0;
        self->vote = 1;
    }
}


// notify all other boards about the conductorship change
void obtain_conductorship(Network *self, int unused){
    if(self->lock != self->rank){
        // (◣_◢)
        SCI_WRITE(&sci0, "[NETWORK]: Claim failed at last step, locked by higher\n");
        reset_lock(self, 0);
    }
    CANMsg msg;
    constructCanMessage(&msg, OBT_CONDUCTORSHIP, BROADCAST, 0);
    CAN_SEND(&can0, &msg);
    self->conductorRank = self->rank;
    reset_lock(self, 0);
    ASYNC(&app, toConductor, 0);
    SCI_WRITE(&sci0, "[NETWORK]: Claimed Conductorship\n");
}


void change_conductor(Network *self, int conductor){
#ifdef CAN_LOOPBACK
    if (self->conductorRank == self->rank)
        return;
#endif
    if (app.mode == CONDUCTOR){
        SCI_WRITE(&sci0, "[NETWORK]: Conductorship Void\n");
    }
    self->conductorRank = conductor;
    self->lock = 0;
    ASYNC(&app, toMusician, 0);
}


/* Lock */

// reset the lock, used when fail to get the conductorship
void reset_lock(Network *self, int unused){
    self->lock = 0;
    self->vote = 1;
}


/* Utils */

// get the index of node based on its rank
int get_node_index(Network *self, int rank){
    for (size_t i = 0; i < self->numNodes; i++){
        if (self->nodes[i] == rank)
            return i;
    }
    SCI_WRITE(&sci0, "WARN: get node index failed\n");
    return -1; // failed search
}


// get the rank of node given its index
int getNodeByIndex(Network *self, int idx){
    if (idx > self->numNodes - 1 || idx < 0){
        SCI_WRITE(&sci0, "WARN: invalid index\n");
        return -1;
    }
    return self->nodes[idx];
}


int get_next_node(Network *self, int unused){
    int idx, next;
    idx = get_node_index(self, self->rank);
    next = (idx + 1) % self->numNodes;
    return self->nodes[next];
}


void voteConductorMinRank(Network *self, int unused){
    ;
}


void voteConductorMaxRank(Network *self, int unused){
    ;
}

/* Information */

// print network information, should be called whenever network changes
void print_network(Network *self, int unused){
    char networkInfo[64];
    snprintf(networkInfo, 64, "Network(%d):\n", self->numNodes);
    SCI_WRITE(&sci0, networkInfo);
    for(size_t i = 0; i < self->numNodes; i++){
        snprintf(networkInfo, 64, "| %d ", self->nodes[i]);
        SCI_WRITE(&sci0, networkInfo);
    }
    SCI_WRITE(&sci0, "|\n");
}


void print_membership(Network *self, int unused){
    char networkInfo[64];
    snprintf(networkInfo, 64, "Membership(%d):\n", self->numNodes);
    SCI_WRITE(&sci0, networkInfo);
    for(size_t i = 0; i < self->numNodes; i++){
        snprintf(networkInfo, 64, "| %d ", self->nodeStatus[i]);
        SCI_WRITE(&sci0, networkInfo);
    }
    SCI_WRITE(&sci0, "|\n");
}


void print_network_verbose(Network *self, int unused){
    char networkInfo[256] = {};
    snprintf(networkInfo, 256,
             "----------------------NETWORK----------------------\n"
             "rank: %d,  conductor: %d\n"
             "lock: %d,  vote: %d\n",
             self->rank, self->conductorRank, self->lock, self->vote);
    SCI_WRITE(&sci0, networkInfo);
    print_network(self, 0);
    print_membership(self, 0);
    //SCI_WRITE(&sci0, "----------------------NETWORK----------------------\n");
}


/* Testing */

void test_compete_conductor(Network *self, int unused){
    CANMsg msg;
    constructCanMessage(&msg, TEST_COMPETE_CONDUCTOR, BROADCAST, 0);
    CAN_SEND(&can0, &msg); // >> claim_conductorship()
    claim_conductorship(self, unused);
}


void test_reset_condutor(Network *self, int unused){
    self->lock = 0;
    self->conductorRank = 0;
    self->vote = 1;
    SCI_WRITE(&sci0, "[TEST]: Conductorship re-initialized\n");
}