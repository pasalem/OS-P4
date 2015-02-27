#include "api.h"
// Delay times also occur when values are written to these levels

vAddr allocateNewInt();
int * accessIntPtr (vAddr address);
void unlockMemory(vAddr address);
void freeMemory(vAddr address);
void print_page_table();
vAddr random_evict(int level);
vAddr add_page(int level, int physical_address);
vAddr LRU(int level);

int find_open_memory(int level);
void move_page(page *page_to_move,int level);
void allocate_memory(int level, int physical_address);
void init();

void init(){
	int counter;
	sem_init(&table_spot_lock,0,1);
	sem_init(&print_mutex,0,1);

	for(counter = 0; counter < SIZE_PAGE_TABLE; counter++){
		if(counter < 3){
			sem_init(&open_spot_lock[counter],0,1);
		}
		if(counter < 25){
			RAM[counter] = 0;
			sem_init( &RAM_lock[counter],0,1);
		}
		if(counter < 100){
			sem_init(&SSD_lock[counter],0,1);
			SSD[counter] = 0;
		}
		if( counter < 1000){
			HDD[counter] = 0;
			page_table[counter].allocated = 0;
			sem_init(&HDD_lock[counter],0,1);
			sem_init(&page_table[counter].page_lock,0,1);
		}
	}
}

//Delay the transfer
delay(int level){
	if(level == RAM_LEVEL){
		return;
	}
	if(level == SSD_LEVEL){
		usleep(250000);
	}
	if(level == HDD_LEVEL){
		usleep(2500000);
	}
}

//Make space for another page in some level
vAddr evict_page(int level){
	switch(aaron){
		case 0:
			return LRU(level);
		case 1:
			return random_evict(level);
	}
}

void unallocate_memory(page *page_to_clear){
	switch( page_to_clear->level ){
		case(RAM_LEVEL):
			RAM[page_to_clear->address] = 0;
			sem_post(&RAM_lock[page_to_clear->address]);
			break;
		case(SSD_LEVEL):
			SSD[page_to_clear->address] = 0;
			sem_post(&SSD_lock[page_to_clear->address]);
			break;
		case(HDD_LEVEL):
			HDD[page_to_clear->address] = 0;
			sem_post(&HDD_lock[page_to_clear->address]);
			break;
	}
	page_to_clear -> allocated = 0;
}

void allocate_memory(int level, int physical_address){
	if(level == RAM_LEVEL){
		RAM[physical_address] = 1;
	}else if(level == SSD_LEVEL){
		SSD[physical_address] = 1;
	}else{
		HDD[physical_address] = 1;
	}
}

//Randomly evict a valid page
vAddr random_evict(int level){
	int counter;
	int match = -1;
	page *options[SIZE_SSD]; //We'll never have more than 100 eviction candidates
	int matches = 0;
	for(counter = 0; counter < SIZE_PAGE_TABLE; counter++ ){
		if( page_table[counter].level != level || !page_table[counter].allocated || page_table[counter].locked){
			continue;
		}
		options[matches++] = &page_table[counter];
	}

	//We found no suitable page to evict
	if(matches == 0){
		printf("Nothing to evict\n");
		pthread_exit();
	}else{
		int random = rand() % matches;
		while( sem_trywait(&(options[random]->page_lock)) != 0){
			random = rand() % matches;
		}

		//e found a valid page to evict in the level that we want to make space in
		printf("Evicting page %d to level %d to make room at level %d\n", random, options[random]->level + 1, options[random]->level);
		move_page(options[random], options[random]->level + 1);
		sem_post(&options[random]->page_lock);
	}
}

//Evicts the page that has been accessed least recently
vAddr LRU(int level){
	page *page_item = (page *)malloc(sizeof(page));
	int counter;
	int match = -1;
	gettimeofday(&page_item->last_used,NULL);

	for(counter = 0; counter < SIZE_PAGE_TABLE; counter++ ){
		if( page_table[counter].level != level || !page_table[counter].allocated || page_table[counter].locked){
			continue;
		}
		if(sem_trywait(&page_table[counter].page_lock) != 0){
			//The page we were attempting to evict is locked
			continue;
		}
		//Compare the eviction candidate's last used entry with the current OS time, find the page that was used least recently
		double compare_time = (page_table[counter].last_used.tv_usec/1000000.0) + page_table[counter].last_used.tv_sec;
		double best_time = (page_item->last_used.tv_usec/1000000.0) + page_item->last_used.tv_sec;
		if( compare_time < best_time ) {
			//Unlock the old page before we replace it
			sem_post(&page_item->page_lock);
			page_item = &page_table[counter];
			match = counter;
		} else{
			//Release the lock of this page since it didn't make the cut
			sem_post(&page_table[counter].page_lock);
		}
	}

	//We found no suitable page to evict
	if(match == -1){
		printf("Nothing to evict\n");
		pthread_exit();
	} else{
		//We found a valid page to evict in the level that we want to make space in
		printf("Evicting page to level %d to make room at level %d\n", page_item->level + 1, page_item->level);
		move_page(page_item, page_item -> level + 1);
		sem_post(&page_table[match].page_lock);
	}
}

//Finds the next unused page table index
vAddr find_open_page(){
	int counter;
	for(counter = 0; counter < SIZE_PAGE_TABLE; counter++){
		if( page_table[counter].allocated == 0){
			if( sem_trywait(&page_table[counter].page_lock) == 0){
				//printf("Took lock on page %d\n", counter);
				return counter;
			} else{
				printf("Tried to take page %d but it was locked\n", counter);
				continue;
			}
		}
	}
	printf("Page table is full!\n");
	exit(1);
}


//Returns -1 if the level is full, 
//otherwise returns the address of the first open position
int find_open_memory(int level){
	sem_wait(&open_spot_lock[level]);
	int counter = 0;
	int size;
	int *memory;
	sem_t *memory_lock;
	if(level == 0){
		size = SIZE_RAM;
		memory = &RAM[0];
		memory_lock = &RAM_lock[0];
	} else if(level == 1){
		size = SIZE_SSD;
		memory = &SSD[0];
		memory_lock = &SSD_lock[0];
	} else{
		size = SIZE_HDD;
		memory = &HDD[0];
		memory_lock = &HDD_lock[0];
	}

	for(counter = 0; counter < size; counter++){
		if(*(memory + counter) == 0){
			//printf("Took lock address %d on page %d\n", counter, level);
			if( sem_trywait(&memory_lock[counter]) == 0){
				sem_post(&open_spot_lock[level]);
				return counter;
			}
		}
	}
	sem_post(&open_spot_lock[level]);
	return -1;
}

//Adds a page entry to the table and allocates the open spot
//Returns the virtual address of the new added page
vAddr add_page(int level, int physical_address){
	if(level > 2){
		printf("Tried to add a page to an invalid level\n");
		exit(1);
	}

	while(physical_address == -1){
		evict_page(level);
		physical_address = find_open_memory(level);
	}

	sem_wait(&table_spot_lock);
	int index = find_open_page();
	sem_post(&table_spot_lock);

	page_table[index].address = physical_address;	//Page refers to this spot in physical memory
	page_table[index].locked = 0;					//Page is unlocked by default
	page_table[index].allocated = 1;					//Page is allocated by default
	page_table[index].level = level;
	gettimeofday(&page_table[index].last_used, NULL);
	allocate_memory(level, physical_address);
	sem_post(&page_table[index].page_lock);
	return index;
}

void move_page(page *page_to_move,int desired_level){
	//Can't move more than 1 level up or down
	if( abs(page_to_move->level - desired_level) != 1){
		printf(KRED "Can't move more than 1 level up or down\n" RESET);
		exit(1);
	}

	//Is there space available in the level we are looking for?
	int physical_address = find_open_memory(desired_level);

	printf(KRED "Moving page from level %d to level %d\n" RESET, page_to_move -> level, desired_level);

	while(physical_address == -1){
		evict_page(desired_level);
		physical_address = find_open_memory(desired_level);
	}

	unallocate_memory(page_to_move);
	delay(desired_level);
	page_to_move->level = desired_level;
	page_to_move -> address = physical_address;
	page_to_move -> allocated = 1;
	allocate_memory(desired_level, physical_address);
}

// Reserves memory location, sizeof(int)
// Must be created in emulated RAM, pushing other pages 
// into lower layers of hierarchy, if full
// Return -1 if no memory available
vAddr allocateNewInt(){
	int physical_address = find_open_memory(RAM_LEVEL);
	return add_page(RAM_LEVEL, physical_address);
}

// Obtains indicated memory page
// Returns an int pointer to the location in emulated RAM
// Page locked in memory and considered "dirty"
// Returns NULL if pointer cannot be provided 
// (if page must be brought into RAM, and all RAM is locked)
int * accessIntPtr (vAddr address){
	int counter;
	page *page_item = (page *)malloc(sizeof(page));
	page_item = &page_table[address];
	page_item->locked = TRUE;
	gettimeofday(&page_item->last_used, NULL);
	//If the page is in RAM already, just return a pointer to it
	if(page_item->level == RAM_LEVEL){
		return &RAM[page_item->address];
	} else{
		//Find an open spot in the next lowest level
		sem_wait(&page_table[address].page_lock);
		move_page(page_item, page_item -> level - 1);
		sem_post(&page_table[address].page_lock);
		return accessIntPtr(address);
	}
	return NULL;
}

// Memory must be unlocked when user is done with it
// Any previous pointers in memory are considered invalid and can't be used
void unlockMemory(vAddr address){
	page_table[address].locked = 0;
}

// User can free memory when user is done with the memory page
// Frees page in memory, and deletes any swapped out copies of page
void freeMemory(vAddr address){
	page *page_to_free = &page_table[address];
	page_to_free -> allocated = 0;
	page_to_free -> locked = 0;
	page_to_free -> locked = 0;
}

void print_page_table(){
	sem_wait(&print_mutex);
	int counter = 0;
	printf(KRED"------------START--------------\n" RESET);
	for(counter = 0; counter < SIZE_PAGE_TABLE; counter++){
		if(page_table[counter].level == 0){
			printf(KBLU" Page w/ vAddr %d on level %d has address %d\n" RESET, counter, page_table[counter].level, page_table[counter].address);
		}
	}
	printf(KRED"------------END--------------\n" RESET);
	sem_post(&print_mutex);
}

//Allocate, access, update, unlock, and free memory
//As we call allocateNewInt, we need to swap old pages to secondary memory
void memoryMaxer() {
	vAddr indexes[SIZE_PAGE_TABLE];
	int index = 0;
	for (index = 0; index < 1000; ++index) {
		printf("Counter has value %d\n", index);
		indexes[index] = allocateNewInt();				//returns the address of the newly allocated item in RAM
		int *value = accessIntPtr(indexes[index]);		//returns a pointer to the spot in ram
		*value = (index * 3) + 1;
		unlockMemory(indexes[index]);
	}
	print_page_table();
	for (index = 0; index < 1000; ++index) {
		freeMemory(indexes[index]);
	}
}

void thrash() {
	vAddr indexes[SIZE_PAGE_TABLE];
	int index = 0;
	for (index = 0; index < 1000; ++index) {
		printf("Allocating new int  %d\n", index);
		indexes[index] = allocateNewInt();

		int random = rand() % (index + 1) ;
		printf("Accessing vAddr %d\n", indexes[random]);
		int *value = accessIntPtr(indexes[random]);		//returns a pointer to the spot in ram
		print_page_table();
		printf("Accessed page %d\n", indexes[random]);
		*value = (index * 3) + 1;
		unlockMemory(indexes[random]);
	}
	for (index = 0; index < 1000; ++index) {
		freeMemory(indexes[index]);
	}
	print_page_table();
}

void usage(){
	printf("Please specify proper arguments:\n\t0 - LRU \n\t1 - Random eviction\n");
	exit(1);
}

int main(int argc, char * argv[]){
	srand(time(NULL));
	if( argc != 2){
		usage();
	}

	//This variable was originally called "algorithm", 
	//but my friend requested that I name a variable after him so...
	aaron = atoi( argv[1] );
	if(aaron != 0 && aaron != 1){
		usage();
	}
	init();
	pthread_t thread1, thread2, thread3;
	pthread_create(&thread1, NULL, &thrash, NULL);
	pthread_create(&thread2, NULL, &thrash, NULL);
	pthread_join(thread1, NULL);
	pthread_join(thread2, NULL);
}