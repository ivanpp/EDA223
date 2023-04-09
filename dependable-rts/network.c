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