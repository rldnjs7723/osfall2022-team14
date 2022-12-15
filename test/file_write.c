#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char** argv){
	if(argc != 2){
		printf("Type \"./file_write [filename]\"\n");
		return -1;
	}
	char* tmp = malloc(32);
	strcpy(tmp, "/root/proj4/");
	strcat(tmp,argv[1]);
	FILE *fp = fopen(tmp,"w");
	fprintf(fp, "Temp\n");
	free(tmp);	
	return 0;
}