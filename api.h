#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define SIZE_RAM 25			// Access time immediate
#define SIZE_SSD 100 		// Access time usleep 0.25 sec
#define SIZE_HDD 1000		// Access time usleep 2.5 sec
#define SIZE_PAGE_TABLE 1000
#define TRUE 1
#define FALSE 0

#define RAM_LEVEL 0
#define SSD_LEVEL 1
#define HDD_LEVEL 2

typedef signed short vAddr;

typedef struct page{
	vAddr *location;	//A pointer to the page
	int locked;			//whether or not the page is locked
	int referenced;		//Whether or not the page has been recently accessed
	int allocated;		//Whether or not this page is taken or not
	int level;
} page;


int ram_count = 0;
int ssd_count = 0;
int hdd_count = 0;
int page_count = 0;
vAddr RAM[SIZE_RAM];
vAddr SSD[SIZE_SSD];
vAddr HDD[SIZE_HDD];
page page_table[SIZE_PAGE_TABLE];