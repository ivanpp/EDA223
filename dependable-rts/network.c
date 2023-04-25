#include "network.h"
#include "heartbeat.h"

Network network = initNetwork(RANK);


// search for existing network by broadcasting a CANMsg
int search_network(Network *self, int unused){
    SCI_WRITE(&sci0, "[NETWORK]: Searching other networks\n");
    CANMsg msg;
    construct_can_message(&msg, SEARCH_NETWORK, BROADCAST, self->rank);
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
            self->nodeStatus[i] = self->nodeStatus[i-1];
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
    construct_can_message(&msg, CLAIM_EXISTENCE, receiver, self->rank);
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
    construct_can_message(&msg, CLAIM_CONDUCTORSHIP, BROADCAST, 0);
    if (CAN_SEND_WRN(&can0, &msg, 2)){
        SCI_WRITE(&sci0, "[NETWORK]: reset lock because can fail\n");
        reset_lock(self, unused);
    }
}


void handle_claim_request(Network *self, int sender){
    CANMsg msg;
    if(self->lock < sender){
        self->lock = sender; // lock acquired by conductor
        construct_can_message(&msg, ANSWER_CLAIM_CONDUCTOR, sender, 1); // agree
    } else {
        char lockInfo[64];
        snprintf(lockInfo, 64, "[LOCK]: My lock is already acquired by: %d\n", self->lock);
        SCI_WRITE(&sci0, lockInfo);
        construct_can_message(&msg, ANSWER_CLAIM_CONDUCTOR, sender, 0); // disagree
    }
    CAN_SEND_WRN(&can0, &msg, 5); // >> handle_answer_to_claim(answer)
}


void handle_answer_to_claim(Network *self, int agree){
    if(agree){
        if(self->vote == count_valid_voters(self, 0) - 1){ // all other agree
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
    construct_can_message(&msg, OBTAIN_CONDUCTORSHIP, BROADCAST, 0);
    CAN_SEND(&can0, &msg);
    self->conductorRank = self->rank;
    reset_lock(self, 0);
    ASYNC(&app, to_conductor, 0);
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
    ASYNC(&app, to_musician, 0);
}


/* Lock */

// reset the lock, used when fail to get the conductorship
void reset_lock(Network *self, int unused){
    self->lock = 0;
    self->vote = 1;
}


/* Node Status */

/*
Detection

detect_node(rank)
DETECT_OFFLINE_NODE ->
                                    answer_detect_node()
                                <-  ANSWER_DETECT_OFFLINE
if NO answer:
    NOTIFY_NODE_OFFLINE ->

*/

void detect_all_nodes(Network *self, int unused){
    // FIXME: test detect node 1 first
    detect_node(self, 1);
}


// detect one node at a time
void detect_node(Network *self, int rank){
    self->detectMsg = AFTER(MSEC(10), self, notify_node_offline, rank);
    CANMsg msg;
    construct_can_message(&msg, DETECT_OFFLINE_NODE, rank, 0);
    CAN_SEND(&can0, &msg); // >> answer_detect_node()
#ifdef DEBUG
    char detectInfo[64];
    snprintf(detectInfo, 64, "[NETWORK]: detect node %d\n", rank);
    SCI_WRITE(&sci0, detectInfo);
#endif
}


void answer_detect_node(Network *self, int sender){
    CANMsg msg;
    construct_can_message(&msg, ANSWER_DETECT_OFFLINE, sender, 0);
    CAN_SEND(&can0, &msg); // >> resolve_detect_node()
}


void resolve_detect_node(Network *self, int unused){
    ABORT(self->detectMsg);
    // make sure set to online maybe
}


void notify_node_offline(Network *self, int rank){
    set_node_offline(self, rank);
    CANMsg msg;
    construct_can_message(&msg, NOTIFY_NODE_OFFLINE, BROADCAST, rank);
    CAN_SEND(&can0, &msg); // >> set_node_offline(rank)
#ifdef DEBUG
    char detectInfo[64];
    snprintf(detectInfo, 64, "[NETWORK]: notify everyone node %d is OFFLINE\n", rank);
    SCI_WRITE(&sci0, detectInfo);
#endif
}

/*
Notify offline

NOTIFY_NODE_OFFLINE ->
                                    set_node_offline(rank)

*/
void set_node_offline(Network *self, int rank){
    // 1. set the node to OFFLINE
    int idx = get_node_index(self, rank);
    self->nodeStatus[idx] = NODE_OFFLINE;
    // 2. (OPT) elect the new conductor
    if (self->conductorRank == rank){
        self->conductorRank = 0;
        claim_conductorship(self, 0);
    }
#ifdef DEBUG
    print_membership(self, 0);
#endif
}


// used when you get some login request from offline node
void handle_login_request(Network *self, int requester){
    // 1. set it to online again
    int idx = get_node_index(self, requester);
    self->nodeStatus[idx] = NODE_ONLINE;
    // 2. claim your existence to it
    CANMsg msg;
    construct_can_message(&msg, NODE_LOGIN_CONFIRM, requester, self->conductorRank);
    CAN_SEND_WR(&can0, &msg);  // >> node_login(sender)
    // 3. also tell it the current conductor's rank
    if (app.mode == CONDUCTOR){
        construct_can_message(&msg, OBTAIN_CONDUCTORSHIP, requester, 0);
        CAN_SEND(&can0, &msg); // >> change_conductor(new_conductor)
    }
    print_membership(self, 0);
}


// used when node join request confirmed by other node
void node_login(Network *self, int sender){
    // 0. node must be a musician when logged in
    if(app.mode == CONDUCTOR){
        SCI_WRITE(&sci0, "[NETWORK ERR]: Can't login as a conductor!\n");
        return;
    }
    // 1. set self to ONLINE
    SCI_WRITE(&sci0, "Login confirmed\n");
    int idx;
    idx = get_node_index(self, self->rank);
    self->nodeStatus[idx] = NODE_ONLINE;
    // 2. set the sender to ONLINE
    idx = get_node_index(self, sender);
    self->nodeStatus[idx] = NODE_ONLINE;
    print_membership(self, 0);
}


// should be triggled when the node detects the failure of itself
void node_logout(Network *self, int unused){
    if (app.mode == CONDUCTOR)
        SCI_WRITE(&sci0, "[NETWORK]: Conductorship void due to failure\n");
    // 1. set conductor to NULL
    self->conductorRank = NO_CONDUCTOR;
    // 2. change self to musician
    ASYNC(&app, to_musician, 0);
    // 3. set all nodes to offline
    for (size_t i = 0; i < self->numNodes; i++){
        self->nodeStatus[i] = NODE_OFFLINE;
    }
    print_membership(self, 0);
    // 4. start the heartbeat (login request)
    SYNC(&heartbeatLogin, enable_heartbeat, 0);
}


int check_self_login(Network *self, int unused){
    int idx, status;
    idx = get_node_index(self, self->rank);
    if(idx  == -1){
        SCI_WRITE(&sci0, "[NETWORK ERR]: self node doesn't exist\n");
        return NODE_OFFLINE;
    }
    status = self->nodeStatus[idx];
    return status;
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
int get_node_by_index(Network *self, int idx){
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


int get_next_valid_node(Network *self, int unused){
    int idx, next;
    idx = get_node_index(self, self->rank);
    next = (idx + 1) % self->numNodes;
    while(self->nodeStatus[next] == NODE_OFFLINE){
        next = (next + 1) % self->numNodes;
    }
    return self->nodes[next];
}


int get_prev_valid_node(Network *self, int unused){
    int idx, prev;
    idx = get_node_index(self, self->rank);
    prev = (idx + self->numNodes - 1) % self->numNodes;
    while(self->nodeStatus[prev] == NODE_OFFLINE){
        prev = (prev + self->numNodes - 1) % self->numNodes;
    }
    return self->nodes[prev];
}


int count_valid_voters(Network *self, int unused){
    int counter = 0;
    for(size_t i = 0; i < self->numNodes; i++){
        if(self->nodeStatus[i] == NODE_ONLINE)
            counter++;
    }
    return counter;
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
    construct_can_message(&msg, TEST_COMPETE_CONDUCTOR, BROADCAST, 0);
    CAN_SEND(&can0, &msg); // >> claim_conductorship()
    claim_conductorship(self, unused);
}


void test_reset_condutor(Network *self, int unused){
    self->lock = 0;
    self->conductorRank = 0;
    self->vote = 1;
    SCI_WRITE(&sci0, "[TEST]: Conductorship re-initialized\n");
}