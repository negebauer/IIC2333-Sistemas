#pragma once

#include "stdlib.h"
#include "stdio.h"

#include "bit.h"

#define uint unsigned int

/*
 * Memory
 */
/* Address width */
#define _adr_width 32
#define _adr_mask (~0)
/* Available Memory width */
#define _mem_width 27
#define _mem_mask  mask_right(_mem_width)
#define _mem_size (1<<_mem_width)
/* Page offset width (Page size)*/
#define _off_width 12
#define _off_mask  mask_right(_off_width)
#define _off_size (1<<_off_width)
/* Page index width */
#define _pagID_width (_adr_width - _off_width)
#define _pagID_mask  (~_off_mask)
#define _pagID_size (1<<_pagID_width)
/* Frame width */
#define _frmID_width (_mem_width - _off_width)
#define _frmID_mask  (_mem_mask & _pagID_mask)
#define _frmID_size (1<<_frmID_width)


#define tlb_memory (1024)
#define tlb_time 1
#define mem_time 100
typedef struct TLB_entry_t {
	int page;
	int frame;      // NOTE: Mapping is irrelevant (for now)
	// char dirty;  // No swap (=

	struct {
		// TODO: extend TLB stats if needed
		//char uses;
		//int timestamp;
	/} stats;
} TLB_entry;
/* TLB width */
#define _tlb_size (tlb_memory / sizeof(TLB_entry))

typedef struct {
	unsigned long long int hits;
	unsigned long long int misses;
	unsigned long long int accesses;

	unsigned long long int time;
} Stats;


char *memory;
int page_to_frame[_pagID_size];
Stats stats;
TLB_entry TLB[_tlb_size];


void vm_init();
int vm_print_memory_layout();
int vm_print_memory_stats();

char vm_read(uint page, uint offset);
void vm_write(uint page, uint offset, char data);
