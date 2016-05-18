#include "assert.h"

#include "vm.h"

/*  Methods

0 = Just put it in TLB[0]:
  * Accesses: 8781841
  * Hits:     7940996 (90.43%)
  * Misses:   840845 (9.57%)
  * Avg time:  10.57 (9.46x faster)

1 = Random:
  * Accesses: 8781835
  * Hits:     7948664 (90.51%)
  * Misses:   833171 (9.49%)
  * Avg time:  10.49 (9.54x faster)

2 = Remove least used:
	* Accesses: 8781837
	* Hits:     7847097 (89.36%)
	* Misses:   934740 (10.64%)
	* Avg time:  11.64 (8.59x faster)

*/

void vm_init_TLB() {
	method = 0;
	for(uint i=0; i<_tlb_size; i++) {
		TLB[i].page = -1;
		TLB[i].frame = -1;
	}
}

/*
 * Updates the hit TLB entry.
 */
void vm_hit(TLB_entry *entry) {
	// TODO: update TLB entry
	//entry->stats.timestamp=stats.accesses;
	switch (method) {
		case 2:
			entry->stats.uses++;
		default:
			break;
	}
}

/*
 * Updates the worst TLB entry with a new one.
 */
void vm_miss(uint page, uint frame) {
	// Look for worst page 'w'
	switch (method) {
		case 0:
			TLB[0].page = page;
			TLB[0].frame = frame;
		case 1:
			TLB[rand() % _tlb_size].page = page;
			TLB[rand() % _tlb_size].frame = frame;
		case 2: {
			int table_index = 0;
			int min = stats.accesses;
			for(int i=0; i<_tlb_size; i++) {
				int uses = TLB[i].stats.uses;
				if (uses < min) {
					table_index = i;
					min = uses;
				}
			}
			TLB[table_index].page = page;
			TLB[table_index].frame = frame;
		}
		default:
			break;
	}
	//TLB[w].page = page;
	//TLB[w]...
}



/*
 * Initializes sim
 */
void vm_init() {
	memory = (char*) calloc(_mem_size, sizeof(char));

	stats.hits = 0;
	stats.misses = 0;
	stats.accesses = 0;
	stats.time = 0;

	vm_init_TLB();
}

/*
 * Checks the TLB to avoid loading the real page table if possible
 */
int vm_in_tlb(int page, int *frame, int *idx) {
	stats.time += tlb_time;
	for(uint i=0; i<_tlb_size; i++)
		if(TLB[i].page==page) {
			*frame = TLB[i].frame;
			*idx = i;
			return 1;
		}

	/* Page direction is not on the table */
	*frame = -1;
	*idx = -1;
	return 0;
}

/*
 * Computes Page's real location on memory.
 */
int vm_get_page_frame(int page) {
	//printf("page: %08x > ", page);
	page %= _pagID_size;
	//printf("%08x\n", page);
	stats.accesses++;

	/* Search TLB */
	int frame;
	int idx;
	if(!vm_in_tlb(page, &frame, &idx)) {
		// =(
		stats.misses++;

		/* Read page table from main memory */
		stats.time += mem_time;
		frame = page_to_frame[page];  /* Needs reading (part of) the page_to_frame index first */

		vm_miss(page, frame);
	}
	else {
		// =)
		stats.hits++;
		assert(idx < _tlb_size);
		vm_hit(&TLB[idx]);
	}

	return frame;
}




/*
 * Reads memory data
 */
char vm_read(uint page, uint offset) {
	int frame = vm_get_page_frame(page);
	frame  &= _frmID_mask;
	offset &= _off_mask;

	return memory[frame+offset];
}

/*
 * Writes memory data
 */
void vm_write(uint page, uint offset, char data) {
	int frame = vm_get_page_frame(page);
	frame  &= _frmID_mask;
	offset &= _off_mask;

	memory[frame+offset] = data;
}




void print_mask(int mask, char *l, char *s, char *r) {
	printf("0x%08x: ", mask);

	printf("[");
	char past = 0;
	for(int i=_adr_width-1; i>=0; i--) {
		if(mask&bit_i(i)) {
			printf("#");
			past=1;
		}
		else
			printf("%s", past? r:l);
		if(i && i%8==0)
			printf(" ");
	}
	printf("]  ");
}


int vm_print_memory_layout() {
	printf("Memory:\n");

	printf("  * Addressable:   ");
	print_mask(_adr_mask, ".", "#", "-");
	printf("  (%d MB)\n",        (1<<(_adr_width-20)));

	printf("  * Available Mem: ");
	print_mask(_mem_mask, ".", "#", "-");
	printf("  (%d MB)\n",        (1<<(_mem_width-20)));

	printf("  * PageID:        ");
	print_mask(_pagID_mask, ".", "#", "-");
	printf("  (%d pages)\n",     (1<<(_pagID_width)));

	printf("  * Offset:        ");
	print_mask(_off_mask, ".", "#", "-");
	printf("  (%d Bytes/Page)\n", _off_size);

	printf("  * FrameID:       ");
	print_mask(_frmID_mask, ".", "#", "-");
	printf("  (%d frames)\n",     _frmID_size);

	printf("  * Page->Frame table: %dMB (>>> CPU cache :/)\n", sizeof(page_to_frame)/1024/1024);
	printf("  * TLB size: %dB (holds %d %d-Bytes entries)\n", tlb_memory, _tlb_size, sizeof(TLB_entry));
}

int vm_print_memory_stats() {
	printf("\n");
	printf("Stats:\n");

	double hr = stats.hits;
	double mr = stats.misses;
	hr /= stats.accesses/100;
	mr /= stats.accesses/100;
	printf("  * Accesses: %llu\n", stats.accesses);
	printf("  * Hits:     %llu (%.2f%%)\n", stats.hits, hr);
	printf("  * Misses:   %llu (%.2f%%)\n", stats.misses, mr);

	double amt = stats.time;
	amt /= stats.accesses;
	printf("  * Avg time:  %.2f", amt);
	if(amt<mem_time)
		printf(" (%.2fx faster)\n", mem_time/amt);
	else
		printf(" (%.2fx SLOWER!!)\n", amt/mem_time);
}
