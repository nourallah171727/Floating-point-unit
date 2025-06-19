#include <stdio.h> 
#include <stdlib.h>
#include "../include/structs.h"

extern struct Result run_simulation();

int main() {

	struct Result r = run_simulation();

    return 0;
}