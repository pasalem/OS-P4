#include <unistd.h>
#include <string.h>

#define MAX_VADDRESSES 1000	// System limited to 1000 virtual memory pages
#define MAX_RAM 25			// Access time immediate
#define MAX_SSD 1000 		// Access time usleep 0.25 sec
#define MAX_DISK 1000		// Access time usleep 2.5 sec

// Delay times also occur when values are written to these levels


vAddr allocateNewInt();
int * accessIntPtr (vAddr address);
void unlockMemory(vAddr address);
void freeMemory(vAddr address);

vAddr indexes[i];


// Reserves memory location, sizeof(int)
// Must be created in emulated RAM, pushing other pages 
	// into lower layers of hierarchy, if full
// Return -1 if no memory available
vAddr allocateNewInt(){


}


// Obtains indicated memory page
// Returns an int pointer to the location in emulated RAM
// Page locked in memory and considered "dirty"
// Returns NULL if pointer cannot be provided 
	// (if page must be brought into RAM, and all RAM is locked)
int * accessIntPtr (vAddr address){


}


// Memory must be unlocked when user is done with it
// Swap to disk if needed
// Any previous pointers in memory are considered invalid and can't be used
	// if user calls this function
void unlockMemory(vAddr address){


}


// User can free memory when user is done with the memory page
// Frees page in memory, and deletes any swapped out copies of page
void freeMemory(vAddr address){


}




