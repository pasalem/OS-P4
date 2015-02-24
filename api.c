#include "api.h"
// Delay times also occur when values are written to these levels

vAddr allocateNewInt();
int * accessIntPtr (vAddr address);
void unlockMemory(vAddr address);
void freeMemory(vAddr address);
void addPage(int level, vAddr address);

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

vAddr LRU(int level){

}

//Evict using the second chance algorithm
vAddr second_chance(int level){
	page *top_choice = front->data;
	//While the item at the back of the list has referenced set
	while(top_choice->referenced){
		//Reset the referenced bit
		top_choice->referenced = 0;
		top_choice->allocated = 0;
		//Put the page back at the end of the queue
		deq();
		enq(top_choice);
		top_choice = front->data;
	}
	//top_choice has not been referenced, we can evict it
	printf("Evicting page from address %d of level %d\n", top_choice->address, top_choice->level);
	deq();
	addPage(top_choice->level + 1, memory_count[top_choice->level + 1]);
	memory_count[top_choice->level]--;



}

//Make space for another page in some level
vAddr evict_page(int level){
	switch(algorithm){
		case 0:
			LRU(level);
			break;
		case 1:
			printf("Evicting page from level %d with second chance\n", level);
			second_chance(level);
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

	//If the number of occupied spots in Ram is less than the size of RAM
	if(memory_count[RAM_LEVEL] < SIZE_RAM ){
		int open_index = memory_count[RAM_LEVEL];
		addPage(RAM_LEVEL, open_index );
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

	page_table[page_count] = new_page;
	enq( &page_table[page_count] );
	page_count++;
	printf("Added page %d\n", page_count);
	memory_count[level]++;
	print_queue();
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




