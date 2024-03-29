#include "network.h"
#include "heartbeat.h"
#include "musicPlayer.h"

Network network = initNetwork(RANK);


    /*
    Join network

    (new member)
    SEARCH_NETWORK  ->
                                        handle_join_request()
                                    <-  CLAIM_EXISTENCE
    handle_join_request()
    CLAIM_EXISTENCE  ->
                                        ignore

    */

// search for existing network by broadcasting a CANMsg
int search_network(Network *self, int unused){
    SCI_WRITE(&sci0, "[NETWORK]: Searching other networks\n");
    CANMsg msg;
    construct_can_message(&msg, SEARCH_NETWORK, BROADCAST, self->rank);
    CAN_SEND_WR(&can0, &msg); // >> handle_join_request(sender)
    return 0;
}


void claim_existence(Network *self, int receiver){
    SCI_WRITE(&sci0, "[NETWORK]: Try to claim my Existence\n");
    CANMsg msg;
    construct_can_message(&msg, CLAIM_EXISTENCE, receiver, self->rank);
    CAN_SEND_WR(&can0, &msg); // >> handle_join_request(sender)
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


/* Conductorship */

    /*
    Claim conductorship

    lock self
    CLAIM_CONDUCTORSHIP  ->
                                    if lock free or locked by lower:
                                        lock acquire by sender
                                    <-  ANSWER_CLAIM_CONDUCTOR(YES)
    (if enough votes)
    OBTAIN_CONDUCTORSHIP  ->
                                        change_conductor(condutor)

                                    if locked by higher:
                                    <-  ANSWER_CLAIM_CONDUCTOR(NO)
    give up, reset

    */

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
        if(self->vote == count_online_nodes(self, 0) - 1){ // all other agree
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
    char condInfo[64];
    snprintf(condInfo, 64, "[NETWORK]: %d is our my conductor\n", self->conductorRank);
    SCI_WRITE(&sci0, condInfo);
}


// reset the lock, used when fail to get the conductorship
void reset_lock(Network *self, int unused){
    self->lock = 0;
    self->vote = 1;
}


/* Node Status */

    /*
    Detection

    detect_node(rank)
    DETECT_OFFLINE_NODE  ->
                                        answer_detect_node()
                                    <-  ANSWER_DETECT_OFFLINE
    if NO answer:
        NOTIFY_NODE_OFFLINE  ->
                                        set_node_offline

    */

void detect_all_nodes(Network *self, int unused){
    int rank;
    int delay = 0;
    for(size_t i = 0; i < self->numNodes; i++){
        rank = self->nodes[i];
        if(rank != self->rank && self->nodeStatus[i] == NODE_ONLINE){
            AFTER(MSEC(delay), self, detect_node, rank);
            delay += DETECTION_INTERVAL;
        }
    }
}


// detect one node at a time
void detect_node(Network *self, int rank){
    self->detectMsg = AFTER(MSEC(1), self, notify_node_offline, rank);
    CANMsg msg;
    construct_can_message(&msg, DETECT_OFFLINE_NODE, rank, 0);
    CAN_SEND(&can0, &msg); // >> answer_detect_node()
#ifdef DEBUG
    char detectInfo[64];
    snprintf(detectInfo, 64, "[NETWORK]: detecting node %d\n", rank);
    SCI_WRITE(&sci0, detectInfo);
#endif
}


void answer_detect_node(Network *self, int sender){
    CANMsg msg;
    construct_can_message(&msg, ANSWER_DETECT_OFFLINE, sender, 0);
    CAN_SEND_WR(&can0, &msg); // >> resolve_detect_node()
}


void resolve_detect_node(Network *self, int sender){
    ABORT(self->detectMsg);
#ifdef DEBUG
    char debugInfo[64];
    snprintf(debugInfo, 64, "[DG DET]: node %d answered\n", sender);
    SCI_WRITE(&sci0, debugInfo);
#endif
}


void notify_node_offline(Network *self, int rank){
    set_node_offline(self, rank);
    CANMsg msg;
    construct_can_message(&msg, NOTIFY_NODE_OFFLINE, BROADCAST, rank);
    CAN_SEND(&can0, &msg); // >> set_node_offline(rank)
    char detectInfo[64];
    snprintf(detectInfo, 64, "[NETWORK]: notify everyone node %d is OFFLINE\n", rank);
    SCI_WRITE(&sci0, detectInfo);
}


void set_node_offline(Network *self, int rank){
    // 1. set the node to OFFLINE
    int idx = get_node_index(self, rank);
    self->nodeStatus[idx] = NODE_OFFLINE;
    // 2. (OPT) elect the new conductor
    if (self->conductorRank == rank){
        self->conductorRank = 0;
        claim_conductorship(self, 0);
    }
    char nodeInfo[64];
    snprintf(nodeInfo, 64, "[NETWORK]: node %d OFFLINE\n", rank);
    SCI_WRITE(&sci0, nodeInfo);
    print_membership(self, 0);
}


    /*
    Rejoin pipeline

    (self)                              (others)
    [1, 1, 1]                           [1, 0, 0]
    NODE_LOGIN_REQUEST  ->
    NODE_LOGIN_REQUEST  ->
    NODE_LOGIN_REQUEST  ->
                                        handle_login_request()
                                    <-  NODE_LOGIN_CONFIRM
    [0, 0, 1]
                                    <-  NODE_LOGIN_CONFIRM
    [0, 0, 0]
                                    <-  OBTAIN_CONDUCTORSHIP @CON
                                    <-  SET_KEY_ALL @CON
                                    <-  SET_TEMPO_ALL @CON
    NODE_LOGIN_SUCCESS  ->
                                        finish_login()
                                        [0, 0, 0]

    */

// used when you get some login request from previous offline node
void handle_login_request(Network *self, int requester){
    // 1. claim your existence to it
    CANMsg msg;
    construct_can_message(&msg, NODE_LOGIN_CONFIRM, requester, self->conductorRank);
    CAN_SEND_WR(&can0, &msg);  // >> node_login(sender)
    // 2. inform conductorship, tempo, key
    if (app.mode == CONDUCTOR){
        construct_can_message(&msg, OBTAIN_CONDUCTORSHIP, requester, 0);
        CAN_SEND(&can0, &msg); // >> change_conductor(new_conductor)
        construct_can_message(&msg, MUSIC_SET_TEMPO_ALL, requester, musicPlayer.tempo);
        CAN_SEND(&can0, &msg);
        construct_can_message(&msg, MUSIC_SET_KEY_ALL, requester, musicPlayer.key);
        CAN_SEND(&can0, &msg);
    }
}


// used when node join request confirmed by other node
void node_login(Network *self, int sender){
    // 0. refresh rejoin task
    ABORT(self->rejoinMsg);
    // 1. node must be a musician when logged in
    if(app.mode == CONDUCTOR){
        SCI_WRITE(&sci0, "[NETWORK ERR]: Can't login as a conductor!\n");
        return;
    }
#ifdef DEBUG
    char debugInfo[64];
    snprintf(debugInfo, 64, "Login confirmed by %d\n", sender);
    SCI_WRITE(&sci0, debugInfo);
#endif
    int idx;
    // 2. set self to ONLINE
    idx = get_node_index(self, self->rank);
    self->nodeStatus[idx] = NODE_ONLINE;
    // 3. set the sender to ONLINE
    idx = get_node_index(self, sender);
    self->nodeStatus[idx] = NODE_ONLINE;
    print_membership(self, 0);
    // 4. schedule the rejoin
    self->rejoinMsg = AFTER(MSEC(5), self, node_login_success, 0);
    // 5. try... to make sure
    SYNC(&musicPlayer, ensemble_set_start, 0);
}


void node_login_success(Network *self, int unused){
    SCI_WRITE(&sci0, "[NETWORK]: login success, notify all to let me join\n");
    print_membership(self, 0);
    SIO_WRITE(&sio0, 0); // lit
    CANMsg msg;
    construct_can_message(&msg, NODE_LOGIN_SUCCESS, BROADCAST, unused);
    CAN_SEND_WR(&can0, &msg); // >> finish_login(requester)
}


void finish_login(Network *self, int requester){
    // 1. set it to online again
    int idx = get_node_index(self, requester);
    self->nodeStatus[idx] = NODE_ONLINE;
    // 2. abort musicbackup once, safety reason
    SYNC(&musicPlayer, abort_all_backup, 0);
#ifdef DEBUG
    SCI_WRITE(&sci0, "ABORT all backup because one node login successfully\n");
#endif
    char loginInfo[64];
    snprintf(loginInfo, 64, "node [%d] login successfully\n", requester);
    SCI_WRITE(&sci0, loginInfo);
    print_membership(self, 0);
    SYNC(&musicPlayer, abort_all_backup, 0);
}


// should be triggled when the node detects the failure of itself
void node_logout(Network *self, int unused){
    SCI_WRITE(&sci0, "[DEBUG]: Logout because I'm the broken one\n");
    if (app.mode == CONDUCTOR)
        SCI_WRITE(&sci0, "[NETWORK]: Conductorship void due to failure\n");
    // 1. set conductor to NULL
    self->conductorRank = NO_CONDUCTOR;
    // 3. set all nodes to offline
    for (size_t i = 0; i < self->numNodes; i++){
        self->nodeStatus[i] = NODE_OFFLINE;
    }
    // 2. change self to musician
    SYNC(&app, to_musician, 0);
    // 4. start the heartbeat (login request)
    SYNC(&heartbeatLogin, enable_heartbeat, 0);
    print_membership(self, 0);
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


int get_first_valid_node(Network *self, int unused){
    for(size_t i = 0; i < self->numNodes; i++){
        if(self->nodeStatus[i] == NODE_ONLINE)
            return self->nodes[i];
    }
    return self->rank;
}


int count_online_nodes(Network *self, int unused){
    int counter = 0;
    for(size_t i = 0; i < self->numNodes; i++){
        if(self->nodeStatus[i] == NODE_ONLINE)
            counter++;
    }
    return counter;
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
    for (size_t i = 0; i < self->numNodes; i++){
        self->nodeStatus[i] = 0;
    }
    SCI_WRITE(&sci0, "[TEST]: Conductorship re-initialized\n");
}