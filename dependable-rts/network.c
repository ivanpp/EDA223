#include "network.h"


Network network = initNetwork(RANK);


// search for existing network by broadcasting a CANMsg
int searchNetwork(Network *self, int unused){
    SCI_WRITE(&sci0, "[NETWORK]: Searching other networks\n");
    CANMsg msg;
    constructCanMessage(&msg, SEARCH_NETWORK, BROADCAST, self->rank);
    CAN_SEND(&can0, &msg);
    return 0;
}


// to handle join request, with safety check
void handleJoinRequest(Network *self, int sender){
    if (self->numNodes == MAX_NODES){
        SCI_WRITE(&sci0, "[NETWORK]: MAX nodes in network already achieved\n");
        return;
    }
    if (sender == self->rank) {
#ifdef __CAN_LOOPBACK
        SCI_WRITE(&sci0, "[NETWORK]: Join request ignored for LOOPBACK mode\n")
        return;
#else
        SCI_WRITE(&sci0, "[NETWORK ERR]: Node with same rank wants to join\n");
#endif
    } else {
        // check if already joined
        for (size_t i = 0; i < self->numNodes; i++){
            if (self->nodes[i] == sender){
                SCI_WRITE(&sci0, "[NETWORK]: Node already in netwrok\n");
                return;
            }
        }
        // confirm joining, force order
        addNodeAscending(self, sender);
        // claim existence to sender too
        claimExistence(self, sender);
        // print network
        printNetwork(self, 0);
    }
}


// add a node into the network
// NOTE: no safety check in this method, should be applied before calling
void addNodeAscending(Network *self, int sender){
    char nodeInfo[32] = {};
    for(size_t i = self->numNodes - 1; i >= 0; i--){
        if (sender < self->nodes[i])
            self->nodes[i+1] = self->nodes[i];
        else{
            self->nodes[i+1] = sender;
            self->numNodes++;
            snprintf(nodeInfo, 32, "[NETWORK]: node %d added\n", sender);
            SCI_WRITE(&sci0, nodeInfo);
            return;
        }
    }
    self->nodes[0] = sender;
    self->numNodes++;
    snprintf(nodeInfo, 32, "[NETWORK]: node %d added\n", sender);
    SCI_WRITE(&sci0, nodeInfo);
    return;
}


void claimExistence(Network *self, int receiver){
    SCI_WRITE(&sci0, "[NETWORK]: Try to claim my Existence\n");
    CANMsg msg;
    constructCanMessage(&msg, CLAIM_EXISTENCE, receiver, self->rank);
    CAN_SEND(&can0, &msg);
}


// sort the nodes of the network by rank, no need if we force the order
int sortNetwork(Network *self, int unused){
    return 0;
}


// print network information, should be called whenever network changes
void printNetwork(Network *self, int unused){
    char networkInfo[64];
    snprintf(networkInfo, 64, "Network(%d):\n", self->numNodes);
    SCI_WRITE(&sci0, networkInfo);
    for(size_t i = 0; i < 8; i++){
        snprintf(networkInfo, 64, "| %d ", self->nodes[i]);
        SCI_WRITE(&sci0, networkInfo);
    }
    SCI_WRITE(&sci0, "|\n");
}


void claimConductorship(Network *self, int unused){
    if (self->conductorRank == self->rank){
        SCI_WRITE(&sci0, "[NETWORK]: Already have conductorship\n");
        return;
    }
    self->lock = self->rank; // lock self
    SCI_WRITE(&sci0, "[NETWORK]: Try to claim my conductorship\n");
    CANMsg msg;
    constructCanMessage(&msg, CLAIM_CONDUCTORSHIP, BROADCAST, 0);
    CAN_SEND(&can0, &msg);
}


void handleConductorshipRequest(Network *self, int sender){
    CANMsg msg;
    if(self->lock == 0){
        self->lock = sender; // lock acquired by conductor
        constructCanMessage(&msg, ACK_CONDUCTORSHIP, sender, 1); // agree
    } else {
        char lockInfo[32];
        snprintf(lockInfo, 32, "Lock already acquired by: %d\n", self->lock);
        SCI_WRITE(&sci0, lockInfo);
        constructCanMessage(&msg, ACK_CONDUCTORSHIP, sender, 0); // disagree
    }
    CAN_SEND(&can0, &msg);
}


void handleConductorshipAck(Network *self, int agree){
    if(agree){
        if(self->vote == self->numNodes - 1){ // all other agree
            obtainConductorship(self, 0);
        }else
            self->vote++;
    } else{ // disagree
        self->lock = 0;
        self->vote = 1;
    }
}


void acknowledgeConductorship(Network *self, int conductor){
    ;
}


// notify all other boards about the conductorship change
void obtainConductorship(Network *self, int unused){
    CANMsg msg;
    constructCanMessage(&msg, OBT_CONDUCTORSHIP, BROADCAST, 0);
    CAN_SEND(&can0, &msg);
    self->conductorRank = self->rank;
    self->lock = 0;
    self->vote = 1;
    ASYNC(&app, toConductor, 0);
    SCI_WRITE(&sci0, "[NETWORK]: Claimed Conductorship\n");
}


void changeConductor(Network *self, int conductor){
    if (app.mode == CONDUCTOR){
        SCI_WRITE(&sci0, "[NETWORK]: Conductorship Void\n");
    }
    self->conductorRank = conductor;
    self->lock = 0;
    ASYNC(&app, toMusician, 0);
}


// get the index of node based on its rank
int getNodeIndex(Network *self, int rank){
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

int getNextNode(Network *self, int unused){
    int idx, next;
    idx = getNodeIndex(self, self->rank);
    next = (idx + 1) % self->numNodes;
    return self->nodes[next];
}


void voteConductorMinRank(Network *self, int unused){
    ;
}


void voteConductorMaxRank(Network *self, int unused){
    ;
}


void printNetworkVerbose(Network *self, int unused){
    char networkInfo[256] = {};
    snprintf(networkInfo, 256,
             "----------------------NETWORK----------------------\n"
             "rank: %d,  conductor: %d\n"
             "lock: %d,  vote: %d\n",
             self->rank, self->conductorRank, self->lock, self->vote);
    SCI_WRITE(&sci0, networkInfo);
    printNetwork(self, 0);
    SCI_WRITE(&sci0, "----------------------NETWORK----------------------\n");
}