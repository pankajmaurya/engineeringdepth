#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define DEBUG 0
/* This program takes input 2 files paths and prints the first line where they differ */
int main(int argc, char *argv[])
{
	if (argc != 3) {
		printf("Usage: diff <file1> <file2>\n");
		return 1;
	}

	FILE *fp1 = fopen(argv[1], "r");
	FILE *fp2 = fopen(argv[2], "r");

	if (fp1 == NULL || fp2 == NULL)
		return 2;

	char buf1[100];
	char buf2[100];
	char *res1;
	char *res2;
	int line_num = 1;

	void report_and_exit(int lno, char *r1, char *r2);
	void debug_report(int lno, char *r1, char *r2, char *extra);

	/*
	 * What: Read lines from both files, compare, increment line num at end.
	 * Why: compare only if both results are present. Handle case of early
	 * termination of either file in next block.
	 */
	while ((res1 = fgets(buf1, 100, fp1)) != NULL 
		&& (res2 = fgets(buf2, 100, fp2)) != NULL) {
		int cmp;	
		if ((cmp = strncmp(res1, res2, 100)) != 0) {
		// We can call strncmp with buf or res equivalently.
		// if ((cmp = strncmp(buf1, buf2, 100)) != 0) {
			// Using buf or res is equivalent for reporting.
			// report_and_exit(line_num, res1, res2);
			report_and_exit(line_num, buf1, buf2);
		}
		char debug[10];
		sprintf(debug, "Cmp: %d", cmp); 
		debug_report(line_num, res1, res2, debug);
		line_num++;
	}
	/*
	 * When res1 is NULL, above while loop breaks, hence read fp2.
	 */
	if (res1 == NULL) {
		res2 = fgets(buf2, 100, fp2);
		if (res2 != NULL)
			report_and_exit(line_num, res1, res2);
	} else if (res1 != NULL && res2 == NULL) {
		report_and_exit(line_num, res1, res2);
	}
	
	printf("IDENTICAL FILES\n");	
	return 0;
}
void debug_report(int line_num, char *res1, char *res2, char* extra_info)
{
	if (DEBUG)
		printf("[DEBUG] [line %d] File1: %s, File2: %s, extra info: %s\n", line_num, 
		res1 == NULL ? "<NULL>" : res1, res2 == NULL ? "<NULL>" : res2, extra_info); 		
}
void report_and_exit(int line_num, char *res1, char *res2)
{
	printf("Difference found at line %d\nFile1: %sFile2: %s\n", line_num, 
		res1 == NULL ? "<NULL>" : res1, res2 == NULL ? "<NULL>" : res2); 		
	exit(0);
}

