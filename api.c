#include "api.h"
// Delay times also occur when values are written to these levels

vAddr allocateNewInt();
int * accessIntPtr (vAddr address);
void unlockMemory(vAddr address);
void freeMemory(vAddr address);
void addPage(int level, vAddr *location);


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
		RAM[ram_count] = ram_count;
		addPage(RAM_LEVEL, &RAM[ram_count]);
	} else if(ssd_count < SIZE_SSD){
		//SSD has a free spot
		SSD[ssd_count] = ssd_count;
		addPage(SSD_LEVEL, &SSD[ssd_count]);
	} else if(hdd_count < SIZE_HDD){
		//HDD has a free spot
		HDD[hdd_count] = hdd_count;
		addPage(HDD_LEVEL, &HDD[hdd_count]);
	} else{
		//Nothing is free!
		printf("The entire heirarchy has been filled %d %d %d %d\n", ram_count, ssd_count, hdd_count, page_count);
	}
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
	int index = 0;
	for(index = 0; index < SIZE_PAGE_TABLE; index++){
		if( *page_table[index].location == address){
			page_table[index].allocated = 0;
		}
	}
}

void addPage(int level, vAddr *location){
	page new_page;
	new_page.location = location;	//Page refers to this spot in memory
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
	memoryMaxer();
}




