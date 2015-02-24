#include "api.h"
// Delay times also occur when values are written to these levels

vAddr allocateNewInt();
int * accessIntPtr (vAddr address);
void unlockMemory(vAddr address);
void freeMemory(vAddr address);
void addPage(int level, vAddr address);
void print_page_table();
vAddr second_chance(int level);
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
			printf("Dequeued page with level %d and address %d\n", temp->data->level, temp->data->address);
			free(front);
			front = temp -> next;
		} else{
			printf("Dequeued last page with level %d and address %d\n", temp->data->level, temp->data->address);
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
		usleep(250000);
	}
	if(level == HDD_LEVEL){
		usleep(2500000);
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
		printf("Page at level %d and address %d\n", current->data->level, current->data->address);
		current = current -> next;
	}
	if(current -> next == NULL){
		printf("Page at level %d and address %d\nEND\n", current->data->level, current->data->address);
	}
}

//Make space for another page in some level
vAddr evict_page(int level){
	switch(algorithm){
		case 0:
			return LRU(level);
		case 1:
			printf("Evicting page from level %d with second chance\n", level);
			return second_chance(level);
	}
}

//Evict using the second chance algorithm
vAddr second_chance(int level){
	page *top_choice = front->data;

	while(top_choice->referenced){
		//Reset the referenced bit
		top_choice->referenced = 0;
		printf(KBLU "Giving page from address %d of level %d a second chance\n" RESET, top_choice->address, top_choice->level);
		//Put the page back at the end of the queue
		deq();
		enq(top_choice);
		top_choice = front->data;
	}


}

vAddr LRU(int level){

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

//Adds a page entry to the table
void add_page(int level, int physical_address){
	if(level > 2){
		printf("Tried to add a page to an invalid level\n");
		exit(1);
	}

	page new_page;
	new_page.address = physical_address;	//Page refers to this spot in physical memory
	new_page.locked = 0;					//Page is unlocked by default
	new_page.referenced = 0;				//Page is unreferenced by default
	new_page.allocated = 1;					//Page is allocated by default
	new_page.level = level;

	int index = find_open_page();
	page_table[index] = new_page;
	enq(&page_table[index]);

}

// Reserves memory location, sizeof(int)
// Must be created in emulated RAM, pushing other pages 
// into lower layers of hierarchy, if full
// Return -1 if no memory available
vAddr allocateNewInt(){
	int physical_address = find_open_memory(RAM_LEVEL);
	//There is physical space in RAM
	if(physical_address >= 0){
		printf("Found an open spot in RAM at position %d\n", physical_address);
		int virtual_address = find_open_page();
		add_page(RAM_LEVEL, physical_address);
		return virtual_address;
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

void print_page_table(){
	int counter = 0;
	for(counter = 0; counter < SIZE_PAGE_TABLE; counter++){
		if(page_table[counter].allocated){
			printf(KBLU" Page on level %d has address %d \n" RESET, page_table[counter].level, page_table[counter].address);
		}
	}
}

//Allocate, access, update, unlock, and free memory
//As we call allocateNewInt, we need to swap old pages to secondary memory
void memoryMaxer() {
	vAddr indexes[SIZE_PAGE_TABLE];
	int index = 0;
	for (index = 0; index < SIZE_PAGE_TABLE; ++index) {
		indexes[index] = allocateNewInt();				//returns the address of the newly allocated item in RAM
		int *value = accessIntPtr(indexes[index]);		//returns a pointer to the spot in ram
		*value = (index * 3) + 1;
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
	init();
	memoryMaxer();
}




