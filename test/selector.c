#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>

#define ROTLOCK_WRITE 400
#define ROTUNLOCK_WRITE 402


int main(int argc, char *argv[]){
	if (argc != 2) {
		printf("Type \"/root/selector [start num]\"\n");
		return -1;
	}
	int num = atoi(argv[1]);
    	FILE *fp;
	
	while(1) {
		if (syscall(ROTLOCK_WRITE, 90, 90) < 0)
			return -1;
		if (!(fp = fopen("integer","w")))
			return -1;
		fprintf(fp, "%d", num);
		fclose(fp);
		printf("selector: %d\n", num++);
		if (syscall(ROTUNLOCK_WRITE, 90, 90) < 0)
			return -1;
        	sleep(1);
	}
	return 0;
}
