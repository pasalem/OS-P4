#include "api.h"
// Delay times also occur when values are written to these levels

vAddr allocateNewInt();
int * accessIntPtr (vAddr address);
void unlockMemory(vAddr address);
void freeMemory(vAddr address);
void print_page_table();
vAddr second_chance(int level);
vAddr add_page(int level, int physical_address);
vAddr LRU(int level);
void init();

//Add an item to the end of the queue
void enq(page *data){
	if (rear == NULL){
		rear = (page_node *)malloc(sizeof(page_node));
		rear-> next = NULL;
		rear-> data = data;
		front = rear;
	} else{
		page_node *temp = (page_node *)malloc(sizeof(page_node));
		temp -> data = data;
		temp -> next = NULL;
		rear-> next = temp;
		rear = temp;
	}
}

//Pops the top element from the queue
void deq(){
	page_node *temp = (page_node *)malloc(sizeof(page_node));
	temp = front;
	if(temp == NULL){
		printf("Can't dequeue empty queue\n");
		return;
	} else{
		if( temp -> data != NULL){
			free(front);
			front = temp -> next;
		} else{
			free(front);
			front = NULL;
			rear = NULL;
		}
	}
}

//Delay the transfer
delay(int level){
	if(level == RAM_LEVEL){
		return;
	}
	if(level == SSD_LEVEL){
		usleep(25000);
	}
	if(level == HDD_LEVEL){
		usleep(25000);
	}
}


//Returns -1 if the level is full, 
//otherwise returns the address of the first open position
vAddr find_open_memory(int level){
	int counter = 0;
	int size;
	int *memory;
	if(level == 0){
		size = SIZE_RAM;
		memory = &RAM[0];
	} else if(level == 1){
		size = SIZE_SSD;
		memory = &SSD[0];
	} else{
		size = SIZE_HDD;
		memory = &HDD[0];
	}

	for(counter = 0; counter < size; counter++){
		if(*(memory + counter) == 0){
			return counter;
		}
	}

	return -1;
}

void init(){
	int counter;
	for(counter = 0; counter < SIZE_PAGE_TABLE; counter++){
		if(counter < 25)
			RAM[counter] = 0;
		if(counter < 100)
			SSD[counter] = 0;
		if( counter < 1000){
			HDD[counter] = 0;
			page_table[counter].allocated = 0;
		}
	}
}


//Print the contents of th queue
void print_queue(){
	page_node *current = (page_node *)malloc(sizeof(page_node));
	current = front;
	if(front == NULL && rear == NULL){
		printf("Queue empty\n");
		return;
	}
	while(current -> next != NULL){
		printf("Page at level %d and address %d with referenced %d\n", current->data->level, current->data->address, current->data->referenced);
		current = current -> next;
	}
	if(current -> next == NULL){
		printf("Page at level %d and address %d with referenced %d\nEND\n", current->data->level, current->data->address, current->data->referenced);
	}
}

//Make space for another page in some level
vAddr evict_page(int level){
	switch(algorithm){
		case 0:
			return LRU(level);
		case 1:
			return second_chance(level);
	}
}

void clear_memory_position(page *page_to_clear){
	switch( page_to_clear->level ){
		case(RAM_LEVEL):
			RAM[page_to_clear->address] = 0;
			break;
		case(SSD_LEVEL):
			SSD[page_to_clear->address] = 0;
			break;
		case(HDD_LEVEL):
			HDD[page_to_clear->address] = 0;
			break;
	}
	page_to_clear -> allocated = 0;
}

//Evict using the second chance algorithm
vAddr second_chance(int level){
	page_node *current = front;

	while(current -> data -> referenced || current -> data -> level != level || current -> data -> allocated == FALSE){
		if( current -> data -> level != level || current -> data -> allocated == FALSE){
			current = current -> next;
			continue;
		}
		current -> data -> referenced = FALSE;
		deq();
		enq( current -> data );
		current = front;
	}
	page *top_choice = current -> data;
	//Find the next available spot in the next lowest memory location
	printf(KRED "Trying to evict a page from level %d\n" RESET, level);
	vAddr free_physical_address = find_open_memory(top_choice->level + 1);
	deq();
	delay(top_choice->level + 1);
	int addr = add_page(top_choice->level + 1, free_physical_address);
	clear_memory_position(top_choice);
}

vAddr LRU(int level){
	printf(KRED "Trying to evict a page from level %d\n" RESET, level);
	page *page_item = (page *)malloc(sizeof(page));
	int counter;
	int match = FALSE;
	gettimeofday(&page_item->last_used,NULL);

	for(counter = 0; counter < SIZE_PAGE_TABLE; counter++ ){
		if( page_table[counter].level != level || !page_table[counter].allocated || page_table[counter].locked){
			continue;
		}
		double compare_time = (page_table[counter].last_used.tv_usec/1000000.0) + page_table[counter].last_used.tv_sec;
		double best_time = (page_item->last_used.tv_usec/1000000.0) + page_item->last_used.tv_sec;
		if( compare_time < best_time ) {
			page_item = &page_table[counter];
			match = TRUE;
		}
	}

	if(match == FALSE){
		printf("Nothing to evict\n");
		exit(1);
	}

	//Find the next available spot in the next lowest memory location
	vAddr free_physical_address = find_open_memory(page_item->level + 1);
	delay(page_item->level + 1);
	vAddr addr = add_page(page_item->level +1, free_physical_address);
	clear_memory_position(page_item);
	printf("Evicting page to level %d to make room at level %d\n", page_item->level + 1, page_item->level);
	return addr;
}

//Finds the next unused page table index
int find_open_page(){
	int counter;
	for(counter = 0; counter < SIZE_PAGE_TABLE; counter++){
		if( page_table[counter].allocated == 0){
			return counter;
		}
	}
	printf("Page table is full!\n");
	exit(1);
}

//Adds a page entry to the table and allocates the open spot
//Returns the virtual address of the new added page
vAddr add_page(int level, int physical_address){
	if(level > 2){
		printf("Tried to add a page to an invalid level\n");
		exit(1);
	}

	if(physical_address == -1){
		evict_page(level);
		physical_address = find_open_memory(level);
	}

	page new_page;
	new_page.address = physical_address;	//Page refers to this spot in physical memory
	new_page.locked = 0;					//Page is unlocked by default
	new_page.referenced = 0;				//Page is unreferenced by default
	new_page.allocated = 1;					//Page is allocated by default
	new_page.level = level;
	gettimeofday(&new_page.last_used, NULL);

	if(level == RAM_LEVEL)
		RAM[physical_address] = 1;
	else if(level == SSD_LEVEL)
		SSD[physical_address] = 1;
	else
		HDD[physical_address] = 1;

	int index = find_open_page();
	page_table[index] = new_page;
	enq(&page_table[index]);
	return index;
}

// Reserves memory location, sizeof(int)
// Must be created in emulated RAM, pushing other pages 
// into lower layers of hierarchy, if full
// Return -1 if no memory available
vAddr allocateNewInt(){
	int physical_address = find_open_memory(RAM_LEVEL);
	//There is physical space in RAM
	if(physical_address >= 0){
		return add_page(RAM_LEVEL, physical_address);
	//RAM is full
	} else{
		evict_page(RAM_LEVEL);
		return allocateNewInt();
	}
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
	//If the page is in RAM already, just return a pointer to it
	if(page_item->level == RAM_LEVEL){
		printf("Found vAddr %d in RAM\n", address);
		return &RAM[page_item->address];
	} else{
		//Find an open spot in the next lowest level
		int free_memory = find_open_memory(page_item->level -1);
		add_page(page_item->level -1, free_memory);
		return accessIntPtr(address);
	}
}

// Memory must be unlocked when user is done with it
// Swap to disk if needed
// Any previous pointers in memory are considered invalid and can't be used
// if user calls this function
void unlockMemory(vAddr address){
	page *page_to_unlock = &page_table[address];
	page_to_unlock->locked = FALSE;

}

// User can free memory when user is done with the memory page
// Frees page in memory, and deletes any swapped out copies of page
void freeMemory(vAddr address){
	page *page_to_free = &page_table[address];
	page_to_free -> allocated = 0;
	page_to_free -> locked = 0;
	page_to_free -> referenced = 0;
	page_to_free -> locked = 0;
}

void print_page_table(){
	int counter = 0;
	for(counter = 0; counter < SIZE_PAGE_TABLE; counter++){
		if(page_table[counter].allocated){
			printf(KBLU" Page on level %d has address %d and allocated %d\n" RESET, page_table[counter].level, page_table[counter].address, page_table[counter].allocated);
		}
	}
}

//Allocate, access, update, unlock, and free memory
//As we call allocateNewInt, we need to swap old pages to secondary memory
void memoryMaxer() {
	vAddr indexes[SIZE_PAGE_TABLE];
	int index = 0;
	for (index = 0; index < 150; ++index) {
		printf("Counter has value %d\n", index);
		indexes[index] = allocateNewInt();				//returns the address of the newly allocated item in RAM
		int *value = accessIntPtr(indexes[index]);		//returns a pointer to the spot in ram
		*value = (index * 3) + 1;
		unlockMemory(indexes[index]);
	}
	print_page_table();
	for (index = 0; index < 150; ++index) {
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
		printf("Accessing vAddr %d\n", random);
		int *value = accessIntPtr(indexes[random]);		//returns a pointer to the spot in ram
		*value = (index * 3) + 1;
		unlockMemory(indexes[index]);
	}
	print_page_table();
	for (index = 0; index < 1000; ++index) {
		freeMemory(indexes[index]);
	}
}



void usage(){
	printf("Please specify proper arguments:\n\t0 - LRU \n\t1 - Second Chance\n");
	exit(1);
}

//Run the actual memory management tool
int main(int argc, char * argv[]){
	srand(time(NULL));
	if( argc != 2){
		usage();
	}

	algorithm = atoi( argv[1] );
	if(algorithm != 0 && algorithm != 1){
		usage();
	}

	init();
	thrash();
}




