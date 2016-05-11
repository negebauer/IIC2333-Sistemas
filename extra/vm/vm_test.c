#include "vm.h"

uint data_per_page = 4;
uint remap_p(uint x) {
	return x/data_per_page;
}
uint remap_o(uint x) {
	return x%data_per_page;
}

void test_bbsort(int MAX, int w) {
	for(int i=0; i<MAX; i++)
		vm_write(remap_p(i), remap_o(i), rand()%w);

	for(int k=0; k<MAX; k++)
		for(int i=MAX-1; i>k; i--) {
			int n = i-1;
			int l = vm_read(remap_p(i), remap_o(i));
			int r = vm_read(remap_p(n), remap_o(n));

			if(l>r) {
				vm_write(remap_p(i), remap_o(i), r);
				vm_write(remap_p(n), remap_o(n), l);
			}
		}
}

char dp_1D(int MAX) {
	printf("fib3(%d)...\n", MAX);

	int i;
	vm_write(remap_p(0),0, 0);
	vm_write(remap_p(1),0, 0);
	vm_write(remap_p(2),0, 1);
	for(i=3; i<=MAX; i++) {
		int a = vm_read(remap_p(i-3), 0);
		int b = vm_read(remap_p(i-2), 0);
		int c = vm_read(remap_p(i-1), 0);
		vm_write(i, 0, a+b+c);
	}
	return vm_read(remap_p(i), 0);
}

char cc(int n) {
	int cc = 0;
	int _n = 0;
	while(n>1) {
		vm_read(remap_p(n), 0);
		if(n%2==0)
			n/=2;
		else {
			_n = 3*n+1;
			n = (_n>0) ? _n : n-1;
		}
		cc++;
	}
	return cc;
}

char ccc(int m) {
	for(int i=1; i<m; i++)
		cc(i);
}

int main() {
	srand(0);
	vm_init();
	vm_print_memory_layout();

	printf("Running tests...\n");
	printf("  DP...\n");
	dp_1D(100000);
	printf("  Collatz...\n");
	cc(60975);
	cc(77671);
	printf("  Collatz [1..5000]...\n");
	ccc(5000);
	printf("  bbsort...\n");
	test_bbsort(2000, 4093);

	vm_print_memory_stats();

	return 0;
}
