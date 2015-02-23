#include "api.h"
// Delay times also occur when values are written to these levels

vAddr allocateNewInt();
int * accessIntPtr (vAddr address);
void unlockMemory(vAddr address);
void freeMemory(vAddr address);
void addPage(int level, vAddr address);

//Make space for another page in some level
vAddr evict_page(int level){
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

	if(ram_count < SIZE_RAM ){
		//RAM has a free spot
		addPage(RAM_LEVEL, ram_count);
		RAM[ram_count] = 1;
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
	page *best_page;
	best_page->level = 999;
	for(index = 0; index < SIZE_PAGE_TABLE; index++){
		if(page_table[index].address == address && page_table[index].level < best_page->level){
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
	if(best_page -> level == 999){
		printf("Tried to access an item that couldn't be found anywhere!\n");
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
}

// User can free memory when user is done with the memory page
// Frees page in memory, and deletes any swapped out copies of page
void freeMemory(vAddr address){
	int index = 0;
	for(index = 0; index < SIZE_PAGE_TABLE; index++){
		if( page_table[index].address == address){
			page_table[index].allocated = 0;
		}
	}
}

void addPage(int level, vAddr address){
	page new_page;
	new_page.address = address;	//Page refers to this spot in memory
	new_page.locked = 0;			//Page is unlocked by default
	new_page.referenced = 0;		//Page is unreferenced by default
	new_page.allocated = 1;			//Page is allocated by default
	new_page.level = level;
	page_table[page_count] = new_page;

	switch(level){
		case RAM_LEVEL:
			ram_count++;
			break;
		case SSD_LEVEL:
			ssd_count++;
			break;
		case HDD_LEVEL:
			hdd_count++;
			break;
	}
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
		//int *value = accessIntPtr(indexes[index]);
		//*value = (index * 3);
		unlockMemory(indexes[index]);
	}

	for (index = 0; index < SIZE_PAGE_TABLE; ++index) {
		freeMemory(indexes[index]);
	}
}

//Run the actual memory management tool
int main(){
	srand(time(NULL));
	memoryMaxer();
}




