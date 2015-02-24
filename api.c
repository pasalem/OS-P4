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
vAddr find_open_spot(int level){
	int counter = 0;
	switch(level){
		case RAM_LEVEL:
			for(counter = 0; counter < SIZE_RAM; counter++){
				if(RAM[counter] == 0){
					return counter;
				}
			}
		break;
		case SSD_LEVEL:
			for(counter = 0; counter < SIZE_SSD; counter++){
				if(SSD[counter] == 0){
					return counter;
				}
			}
		break;
		case HDD_LEVEL:
			for(counter = 0; counter < SIZE_HDD; counter++){
				if(HDD[counter] == 0){
					return counter;
				}
			}
		break;
	}
	return -1;
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
	//While the item at the back of the list has referenced set
	while(top_choice->referenced){
		//TODO:
		//Adjust the queuing mechanism so that we have a queue for each heirarchy
		//Reset the referenced bit
		top_choice->referenced = 0;
		printf(KBLU "Giving page from address %d of level %d a second chance\n" RESET, top_choice->address, top_choice->level);
		//Put the page back at the end of the queue
		deq();
		enq(top_choice);
		top_choice = front->data;
	}
	//top_choice has not been referenced, we can evict it
	printf(KRED "Evicting page from address %d of level %d\n" RESET, top_choice->address, top_choice->level);
	//Free top_choice's spot in memory and let something else use it
	top_choice -> allocated = 0;
	switch( top_choice->level ){
		case(RAM_LEVEL):
			RAM[top_choice->address] = 0;
			break;
		case(SSD_LEVEL):
			SSD[top_choice->address] = 0;
			break;
		case(HDD_LEVEL):
			HDD[top_choice->address] = 0;
			break;
	}

	//Find the next available spot in the next lowest memory location
	vAddr spot = find_open_spot(top_choice->level + 1);
	if(spot == -1){
		//The next level is full too, so we need to evict at the next level, additionally. 
		printf(KRED "Level %d is full, but also the next level, %d is full!\n\n\n\n" RESET, level, top_choice->level + 1 );
		return evict_page(top_choice->level + 1);
	} else{
		//We found space, for the item at the next level, we just need to wait and add a page entry
		printf(KRED "Level %d full, but level %d has a free spot at %d\n" RESET, level, top_choice->level + 1, spot );
		//deq();
		delay(top_choice->level + 1);
		addPage(top_choice->level + 1, spot);
		return spot;
	}
}

vAddr LRU(int level){

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

	//If there is an open spot in RAM
	int open_spot = find_open_spot(RAM_LEVEL);
	if(open_spot >= 0){
		printf("Found an open spot in RAM at position %d\n", open_spot);
		addPage(RAM_LEVEL, open_spot );
		return open_spot;
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
	int index = 0;
	page *best_page = (page *) malloc( sizeof( page ));
	best_page->level = 999;
	//printf("Looking for an item with address %d\n", address);
	for(index = 0; index < SIZE_PAGE_TABLE; index++){
		//If the page table has an entry with this address, and it's at a lower level than anything else we've found
		//And it is currently in use, then update our best_page entry
		page current_page = page_table[index];
		//printf("Checking page table entry with address %d, level %d, allocated %d\n", current_page.address, current_page.level, current_page.allocated);
		if(current_page.address == address && current_page.level < best_page->level && current_page.allocated){
			best_page = &page_table[index];
		}

		//If we found the item in RAM, set it to dirty, lock it, and return the value
		if(best_page->level == RAM_LEVEL){
			best_page->dirty = TRUE;
			best_page->locked = TRUE;
			best_page->referenced = 1;
			return &RAM[best_page->address];
		}
	}

	//We either found nothing, or the item is not in RAM
	if(best_page -> level == 999){
		printf("Tried to access an item that couldn't be found anywhere! ADDRESS %d\n", address);
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
		if(page_table[index].locked == TRUE){
			page_table[index].locked = FALSE;
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
	if(level > 2){
		printf("Tried to add a page to an invalid level\n");
		exit(1);
	}
	page new_page;
	new_page.address = address;		//Page refers to this spot in memory
	new_page.locked = 0;			//Page is unlocked by default
	new_page.referenced = 0;		//Page is unreferenced by default
	new_page.allocated = 1;			//Page is allocated by default
	new_page.level = level;

	switch(level){
		case RAM_LEVEL:
			RAM[address] = 1;
			break;
		case SSD_LEVEL:
			SSD[address] = 1;
			break;
		case HDD_LEVEL:
			HDD[address] = 1;
			break;
	}

	page_table[page_count] = new_page;
	enq( &page_table[page_count] );
	page_count++;
	printf("Added page %d\n", page_count);
	memory_count[level]++;
	//print_queue();
	print_page_table();
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
		indexes[index] = allocateNewInt();
		int *value = accessIntPtr(indexes[index]);
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
	memoryMaxer();
}




