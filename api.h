#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <semaphore.h>

#define SIZE_RAM 25			// Access time immediate
#define SIZE_SSD 100 		// Access time usleep 0.25 sec
#define SIZE_HDD 1000		// Access time usleep 2.5 sec
#define SIZE_PAGE_TABLE 1000
#define TRUE 1
#define FALSE 0

#define KBLU  "\x1B[34m"
#define KCYN  "\x1B[36m"
#define KRED  "\x1B[31m"
#define RESET "\033[0m"

#define RAM_LEVEL 0
#define SSD_LEVEL 1
#define HDD_LEVEL 2

typedef signed short vAddr;

//Page node
typedef struct page{
	int address;
	int locked;			//whether or not the page is locked
	int referenced;		//Whether or not the page has been recently accessed (SC Algorithm)
	int allocated;		//Whether or not this page is taken or not
	int level;
	sem_t page_lock;
	struct timeval last_used;
} page;


//Queue node
typedef struct page_node{
	page *data;
	struct page_node *next;
}page_node;

page_node *front;
page_node *rear;

int algorithm; //0 - LRU, 1 - Second Chance
int memory_count[3];
int page_count = 0;

int RAM[SIZE_RAM];
int SSD[SIZE_SSD];
int HDD[SIZE_HDD];

sem_t open_spot_lock[3];
sem_t table_spot_lock;
sem_t RAM_lock[SIZE_RAM];
sem_t SSD_lock[SIZE_SSD];
sem_t HDD_lock[SIZE_HDD];
sem_t print_mutex;


page page_table[SIZE_PAGE_TABLE];
