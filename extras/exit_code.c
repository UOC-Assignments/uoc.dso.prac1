#include <asm/unistd.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char*argv[])
{
	int code = (argc==1)? 0 : atoi(argv[1]);

	/* Performs, for sure, _exit system call */
	/* exit() may call exit_group system call */
	syscall(__NR_exit, code);
	return(0);
}
