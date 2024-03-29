#include "assert.h"
//#include "time.h"

#include "vm.h"

#define LIMIT_TIME_UNUSED 20

/*  Methods

0 = FIFO:
	* Accesses: 8781835
	* Hits:     7947484 (90.50%)
	* Misses:   834351 (9.50%)
	* Avg time:  10.50 (9.52x faster)

1 = Random:
	* Accesses: 8781839
	* Hits:     7896204 (89.92%)
	* Misses:   885635 (10.08%)
	* Avg time:  11.08 (9.02x faster)

2 = Remove least used: (remover la menos usada)
	* Accesses: 8781835
	* Hits:     7884792 (89.79%)
	* Misses:   897043 (10.21%)
	* Avg time:  11.21 (8.92x faster)

3 = Remove most used:
	* Accesses: 8781835
	* Hits:     6643842 (75.65%)
	* Misses:   2137993 (24.35%)
	* Avg time:  25.35 (3.95x faster)

4 = Statistics merge, between Uses and time not used


5 = Remove Least Recently Used (remover la que se uso hace mas tiempo)
	* Accesses: 8781835
	* Hits:     7865534 (89.57%)
	* Misses:   916301 (10.43%)
	* Avg time:  11.43 (8.75x faster)

6 = Remove Least Recently Used, with second chance
	* Accesses: 8781835
	* Hits:     7859490 (89.50%)
	* Misses:   922345 (10.50%)
	* Avg time:  11.50 (8.69x faster)

*/

void vm_init_TLB() {
	for(uint i = 0; i < _tlb_size; i++) {
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
	entry->stats.uses++;
	entry->stats.last_used_time = stats.time;
}

/*
 * Updates the worst TLB entry with a new one.
 */
void vm_miss(uint page, uint frame) {
	// Look for worst page 'w'
	method = (stats.accesses % 5); // Random case between FIFO, RND, LFU, MFU and LRU
	switch (method) {
		case 0:
			TLB[stats.accesses % _tlb_size].page = page;
			TLB[stats.accesses % _tlb_size].frame = frame;
			TLB[stats.accesses % _tlb_size].stats.uses++;
			TLB[stats.accesses % _tlb_size].stats.last_used_time = stats.time;
			break;

		case 1:
			TLB[rand() % _tlb_size].page = page;
			TLB[rand() % _tlb_size].frame = frame;
			TLB[rand() % _tlb_size].stats.uses++;
			TLB[rand() % _tlb_size].stats.last_used_time = stats.time;
			break;

		case 2: {
			int table_index = 0;
			int min = stats.accesses;
			int uses;
			for(int i = 0; i < _tlb_size; i++) {
				uses = TLB[i].stats.uses;
				if (uses < min) {
					table_index = i;
					min = uses;
				}
			}
			TLB[table_index].page = page;
			TLB[table_index].frame = frame;
			TLB[table_index].stats.uses++;
			TLB[table_index].stats.last_used_time = stats.time;
			break;
		}

		case 3: {
			int table_index = 0;
			int max = stats.accesses;
			int uses;
			for(int i = 0; i < _tlb_size; i++) {
				uses = TLB[i].stats.uses;
				if (uses > max) {
					table_index = i;
					max = uses;
				}
			}
			TLB[table_index].page = page;
			TLB[table_index].frame = frame;
			TLB[table_index].stats.uses++;
			TLB[table_index].stats.last_used_time = stats.time;
			break;
		}

		case 4: {
			int table_index = 0;
			float time_unused, uses;
			float priority;
			time_unused = stats.time - TLB[0].stats.last_used_time;
			uses = TLB[0].stats.uses;
			float min_priority = uses + (10 / time_unused);
			for(int i = 0; i < _tlb_size; i++) {
				time_unused = stats.time - TLB[i].stats.last_used_time;
				uses = TLB[i].stats.uses;
				priority = uses + (10 / time_unused);//calcular prioridad
				if (priority < min_priority) {
					table_index = i;
					min_priority = priority;
				}
			}
			TLB[table_index].page = page;
			TLB[table_index].frame = frame;
			TLB[table_index].stats.last_used_time = stats.time;
			TLB[table_index].stats.uses++;
			break;
		}

		case 5: {
			int table_index = 0;
			int max_unused_time = 0;
			int time_unused;
			for(int i = 0; i < _tlb_size; i++) {
				time_unused = stats.time - TLB[i].stats.last_used_time;
				if (time_unused > max_unused_time) {
					table_index = i;
					max_unused_time = time_unused;
				}
			}
			TLB[table_index].page = page;
			TLB[table_index].frame = frame;
			TLB[table_index].stats.last_used_time = stats.time;
			TLB[table_index].stats.uses++;
			break;
		}

		//case 6: {
		//	int time_unused;
		//	for(int i = 0; i < _tlb_size; i++) {
		//		time_unused = stats.time - TLB[i].stats.last_used_time;
		//		if (time_unused > (stats.time % LIMIT_TIME_UNUSED)) {
		//			TLB[i].stats.referenced = true;
		//		}
		//	}

		//	bool removed = false;
		//	int i = 0;
		//	while (!removed)
		//	{
		//		int index = i % _tlb_size;
		//		if (TLB[index].stats.referenced)
		//		{
		//			if (!TLB[index].stats.second_chance)
		//			{
		//				TLB[index].page = page;
		//				TLB[index].frame = frame;
		//				TLB[index].stats.last_used_time = stats.time;
		//				TLB[index].stats.uses++;
		//				break;
		//			}
		//			else
		//			{
		//				TLB[index].stats.second_chance = false;
		//			}
		//		}
		//		i++;
		//	}

		//	break;
		//}

		default:
			break;
	}

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

	printf("  * Page->Frame table: %luMB (>>> CPU cache :/)\n", sizeof(page_to_frame)/1024/1024);
	printf("  * TLB size: %dB (holds %lu %lu-Bytes entries)\n", tlb_memory, _tlb_size, sizeof(TLB_entry));
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
