#include "vm.h"

char dp_1D(int MAX) {
	printf("fib3(%d)...\n", MAX);

	int i;
	vm_write(0,0, 0);
	vm_write(1,0, 0);
	vm_write(2,0, 1);
	for(i=3; i<=MAX; i++) {
		int a = vm_read(i-3, 0);
		int b = vm_read(i-2, 0);
		int c = vm_read(i-1, 0);
		vm_write(i, 0, a+b+c);
	}
	return vm_read(i, 0);
}

char cc(int n) {
	int cc = 0;
	int _n = 0;
	while(n>1) {
		vm_read(n, 0);
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
	printf("Collatz [1..%d]\n", m);
	for(int i=1; i<m; i++)
		cc(i);
}

int main() {
	vm_init();
	vm_print_memory_layout();

	printf("Running tests...\n");
	dp_1D(10000);
	ccc(5000);
	cc(60975);
	cc(77671);
	//cc(113383);
	//cc(159487);
	vm_print_memory_stats();

	return 0;
}
