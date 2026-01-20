#ifndef USERMODE_H
#define USERMODE_H

#include <stdint.h>

// User mode test function that runs in ring 3
void user_function();

// Available user programs
void shell();
void weakpoint();

#endif // USERMODE_H
