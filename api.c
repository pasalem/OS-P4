#include "api.h"
// Delay times also occur when values are written to these levels

vAddr allocateNewInt();
int * accessIntPtr (vAddr address);
void unlockMemory(vAddr address);
void freeMemory(vAddr address);
void addPage(int level, vAddr address);

vAddr LRU(){

}

vAddr second_chance(){
	
}

//Make space for another page in some level
vAddr evict_page(int level){
	switch(algorithm){
		case 0:
			LRU();
			break;
		case 1:
			second_chance();
			break;
	}

	//Take a parameter when the program is run to determine which eviction algorithm to use
	
	//We take a level parameter because sometimes it is necessary 
	//to move a page from, for example, SSD to HDD (not always RAM to SSD)

	//Check if the level we want to evict from is full
	//If the level is full, then run eviction on the level.
		//move evicted_page out
		//Put inserted_page in it's place
		//Add a new page table entries for both pages
	//If the level isn't full
		//add a new page table entry for inserted_page
}

// Reserves memory location, sizeof(int)
// Must be created in emulated RAM, pushing other pages 
// into lower layers of hierarchy, if full
// Return -1 if no memory available
vAddr allocateNewInt(){
	if( page_count > SIZE_PAGE_TABLE ){
		//Nothing is free!
		printf("The page table is full. Free some pages\n");
		return -1;
	}

	if(memory_count[RAM_LEVEL] < SIZE_RAM ){
		//RAM has a free spot
		int open_index = memory_count[RAM_LEVEL];
		addPage(RAM_LEVEL, open_index );
		memory_count[RAM_LEVEL]++;
		RAM[open_index] = rand() % 100;
		return memory_count[RAM_LEVEL] - 1;
	} else{
		//Still have to write this
		evict_page(RAM_LEVEL);
	}
}

// Obtains indicated memory page
// Returns an int pointer to the location in emulated RAM
// Page locked in memory and considered "dirty"
// Returns NULL if pointer cannot be provided 
// (if page must be brought into RAM, and all RAM is locked)
int * accessIntPtr (vAddr address){
	int index = 0;
	page *best_page = (page *) malloc( sizeof( page ));
	best_page->level = 999;
	printf("Looking for an item with address %d\n", address);
	for(index = 0; index < SIZE_PAGE_TABLE; index++){
		//If the page table has an entry with this address, and it's at a lower level than anything else we've found
		//And it is currently in use, then update our best_page entry
		page current_page = page_table[index];
		printf("Checking page table entry with address %d, level %d, allocated %d\n", current_page.address, current_page.level, current_page.allocated);
		if(current_page.address == address && current_page.level < best_page->level && current_page.allocated){
			printf("Found a best match in RAM\n");
			best_page = &page_table[index];
		}

		//If we found the item in RAM, set it to dirty, lock it, and return the value
		if(best_page->level == 0){
			best_page->dirty = TRUE;
			best_page->locked = TRUE;
			return &RAM[best_page->address];
		}
	}

	//We either found nothing, or the item is not in RAM
	if(best_page -> level == 999){}

		printf("Tried to access an item that couldn't be found anywhere!\n");
		exit(1);
	} else{
		//The item we wanted isn't in RAM, evict a page if necessary to put it there
		evict_page( RAM_LEVEL );
	}
}

// Memory must be unlocked when user is done with it
// Swap to disk if needed
// Any previous pointers in memory are considered invalid and can't be used
// if user calls this function
void unlockMemory(vAddr address){
	int index = 0;

	for(index = 0; index < SIZE_PAGE_TABLE; index++){
		if(page_table[index]->locked == TRUE){
			page_table[index]->locked = FALSE;
			if(memory_count[RAM_LEVEL] == SIZE_RAM){
				if(memory_count[SSD_LEVEL] < SIZE_SSD){
					evict_page( SSD_LEVEL );
				}
				else {
					evict_page( HDD_LEVEL );
				}
			}
		}
	}

}

// User can free memory when user is done with the memory page
// Frees page in memory, and deletes any swapped out copies of page
void freeMemory(vAddr address){
	int index = 0;
	for(index = 0; index < SIZE_PAGE_TABLE; index++){
		if( page_table[index].address == address){
			page_table[index].allocated = 0;
			int level = page_table[index].level;
			//Decrement the number of open slots at this memory level
			memory_count[level]--;
		}
	}
}

void addPage(int level, vAddr address){
	page new_page;
	new_page.address = address;		//Page refers to this spot in memory
	new_page.locked = 0;			//Page is unlocked by default
	new_page.referenced = 0;		//Page is unreferenced by default
	new_page.allocated = 1;			//Page is allocated by default
	new_page.level = level;

	page_table[page_count] = new_page;
	page_count++;
	printf("Added page %d\n", page_count);
}

//Allocate, access, update, unlock, and free memory
//As we call allocateNewInt, we need to swap old pages to secondary memory
void memoryMaxer() {
	vAddr indexes[SIZE_PAGE_TABLE];
	int index = 0;
	for (index = 0; index < SIZE_PAGE_TABLE; ++index) {
		indexes[index] = allocateNewInt();
		int *value = accessIntPtr(indexes[index]);
		*value = (index * 3);
		unlockMemory(indexes[index]);
	}

	for (index = 0; index < SIZE_PAGE_TABLE; ++index) {
		freeMemory(indexes[index]);
	}
}

void usage(){
	printf("Please specify proper arguments:\n\t0 - LRU \n\t1 - Second Chance\n");
	exit(1);
}

//Run the actual memory management tool
int main(int argc, char * argv[]){
	if( argc != 2){
		usage();
	}

	algorithm = atoi( argv[1] );
	if(algorithm != 0 && algorithm != 1){
		usage();
	}

	srand(time(NULL));
	memoryMaxer();
}




