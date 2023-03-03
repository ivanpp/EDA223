#include "userButton.h"
#include "systemPorts.h"

// 
void reactUserButton(UserButton *self, int unused){
    SCI_WRITE(&sci0, "Invoke from UserButton Object\n");
}