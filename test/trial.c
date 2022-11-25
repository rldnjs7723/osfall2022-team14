#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>

#define ROTLOCK_READ 399
#define ROTUNLOCK_READ 401


void prime_factor(int num) {
	printf("%d = ", num);
	int i = 2;
	while (i < num) {
		if (num % i)
			i++;
		else {
			num /= i;
			printf("%d * ", i);
		}
	}
	printf("%d\n", num);
}

int main(int argc, char *argv[]) {
	if (argc != 2) {
		printf("Type \"/root/selector [process_num]\"\n");
		return -1;
	}
	int num;
	int process_num = atoi(argv[1]);
    FILE *fp;

	while (1) {
		if (syscall(ROTLOCK_READ, 90, 90) < 0)
			return -1;
		while (1) {
			syscall(ROTUNLOCK_READ, 90, 90);
			if (fp = fopen("integer", "r")) {
				syscall(ROTLOCK_READ, 90, 90);
				break;
			}
		}
		fscanf(fp, "%d", &num);
		printf("trial-%d: ", process_num);
		prime_factor(num);
		fclose(fp);
		if (syscall(ROTUNLOCK_READ, 90, 90) < 0)
			return -1;
        sleep(1);
	}
	return 0;
}
