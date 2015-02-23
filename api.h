#include <unistd.h>
#include <string.h>

#define MAX_VADDRESSES 1000	// System limited to 1000 virtual memory pages
#define MAX_RAM 25			// Access time immediate
#define MAX_SSD 1000 		// Access time usleep 0.25 sec
#define MAX_DISK 1000		// Access time usleep 2.5 sec

typedef signed short vAddr;