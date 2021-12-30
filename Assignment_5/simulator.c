#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define NAME_SIZE 100

int main(void) {
	FILE *fp;
	char fname[NAME_SIZE], method[NAME_SIZE];
	int frame_size;
	int page_reference[31];
	int temp, string_size = 0;
	bool check, replace;

	printf("파일의 이름을 입력하세요 : ");
	scanf("%s", fname);

	if ((fp = fopen(fname, "r")) == NULL) {
		fprintf(stderr, "open error for %s\n", fname);
		exit(1);
	}
	else {
		fscanf(fp, "%d", &frame_size);
		while (fscanf(fp, "%d", &temp) != EOF) {
			page_reference[string_size] = temp;
			string_size++;
		}
	}


	while (1) {
		int page_frame[frame_size], reference_bit[frame_size];
		int page_fault = 0;
		memset(page_frame, -1, sizeof(page_frame));
		memset(reference_bit, 0, sizeof(reference_bit));

		printf("Used method : ");
		scanf("%s", method);

		if (strcmp(method, "exit") == 0) {
			break;
		}
		else if (strcmp(method, "OPT") == 0) {
			printf("page reference string : ");
			for (int i = 0; i < string_size; i++) 
				printf("%d ", page_reference[i]);
			printf("\n\n");
			printf("        frame   ");
			for (int i = 1; i <= frame_size; i++)
				printf("%d       ", i);
			printf("page fault\ntime\n");

			for (int i = 0; i < string_size; i++) {
				check = false;
				replace = false;

				for (int j = 0; j < frame_size; j++) {
					if (page_frame[j] == page_reference[i]) {
						check = true;
						break;
					}

					if (page_frame[j] < 0) {
						page_fault++;
						page_frame[j] = page_reference[i];
						check = true;
						replace = true;
						break;
					}
				}

				if (!check) {				
					int location = -1, victim = -1;
					for (int j = 0; j < frame_size; j++) {
						bool flag = false;
						for (int k = i + 1; k < string_size; k++) {
							if (page_frame[j] == page_reference[k]) {
								if (location < k) {
									location = k;
									victim = j;
								}
								flag = true;
								break;
							}
						}

						if (!flag) {
							victim = j;
							break;
						}
					}

					page_frame[victim] = page_reference[i];
					page_fault++;
					replace = true;
				}

				if (i < 9) printf("%d               ", i + 1);
				else printf("%d              ", i + 1);
				for (int j = 0; j < frame_size; j++) {
					if (page_frame[j] < 0) printf("        ");
					else printf("%d       ", page_frame[j]);
				}
				if (replace) printf("F\n");
				else printf("\n");				
			}
			printf("Number of page faults : %d times\n\n", page_fault);
		}
		else if (strcmp(method, "FIFO") == 0) {
			printf("page reference string : ");
			for (int i = 0; i < string_size; i++) 
				printf("%d ", page_reference[i]);
			printf("\n\n");
			printf("        frame   ");
			for (int i = 1; i <= frame_size; i++)
				printf("%d       ", i);
			printf("page fault\ntime\n");

			int victim = 0;
			for (int i = 0; i < string_size; i++) {
				check = false;
				replace = false;

				for (int j = 0; j < frame_size; j++) {
					if (page_frame[j] == page_reference[i]) {
						check = true;
						break;
					}

					if (page_frame[j] < 0) {
						page_fault++;
						page_frame[j] = page_reference[i];
						check = true;
						replace = true;
						break;
					}
				}

				if (!check) {			
					page_frame[victim] = page_reference[i];
					victim++;
					page_fault++;
					replace = true;
				}

				if (victim == frame_size)
					victim = 0;

				if (i < 9) printf("%d               ", i + 1);
				else printf("%d              ", i + 1);
				for (int j = 0; j < frame_size; j++) {
					if (page_frame[j] < 0) printf("        ");
					else printf("%d       ", page_frame[j]);
				}
				if (replace) printf("F\n");
				else printf("\n");				
			}
			printf("Number of page faults : %d times\n\n", page_fault);
		}
		else if (strcmp(method, "LRU") == 0) {
			printf("page reference string : ");
			for (int i = 0; i < string_size; i++) 
				printf("%d ", page_reference[i]);
			printf("\n\n");
			printf("        frame   ");
			for (int i = 1; i <= frame_size; i++)
				printf("%d       ", i);
			printf("page fault\ntime\n");

			for (int i = 0; i < string_size; i++) {
				check = false;
				replace = false;

				for (int j = 0; j < frame_size; j++) {
					if (page_frame[j] == page_reference[i]) {
						check = true;
						break;
					}

					if (page_frame[j] < 0) {
						page_fault++;
						page_frame[j] = page_reference[i];
						check = true;
						replace = true;
						break;
					}
				}

				if (!check) {				
					int location = 99, victim = -1;
					for (int j = 0; j < frame_size; j++) {
						for (int k = i - 1; k >= 0; k--) {
							if (page_frame[j] == page_reference[k]) {
								if (location > k) {
									location = k;
									victim = j;
								}
								break;
							}
						}
					}

					page_frame[victim] = page_reference[i];
					page_fault++;
					replace = true;
				}

				if (i < 9) printf("%d               ", i + 1);
				else printf("%d              ", i + 1);
				for (int j = 0; j < frame_size; j++) {
					if (page_frame[j] < 0) printf("        ");
					else printf("%d       ", page_frame[j]);
				}
				if (replace) printf("F\n");
				else printf("\n");				
			}
			printf("Number of page faults : %d times\n\n", page_fault);
		}
		else if (strcmp(method, "Second-Chance") == 0) {
			printf("page reference string : ");
			for (int i = 0; i < string_size; i++) 
				printf("%d ", page_reference[i]);
			printf("\n\n");
			printf("        frame   ");
			for (int i = 1; i <= frame_size; i++)
				printf("%d       ", i);
			printf("page fault\ntime\n");

			int victim = 0;
			for (int i = 0; i < string_size; i++) {
				check = false;
				replace = false;

				for (int j = 0; j < frame_size; j++) {
					if (page_frame[j] == page_reference[i]) {
						reference_bit[j] = 1;
						check = true;
						break;
					}

					if (page_frame[j] < 0) {
						page_fault++;
						page_frame[j] = page_reference[i];
						reference_bit[j] = 0;
						check = true;
						replace = true;
						break;
					}
				}

				if (!check) {
					bool flag = false;
					for (int j = victim; j < frame_size; j++) {
						if (reference_bit[j] == 0) {
							page_frame[j] = page_reference[i];
							page_fault++;
							replace = true;
							flag = true;
							victim = j + 1;
							break;
						}
						else if (reference_bit[j] == 1) {
							reference_bit[j] = 0;
						}
					}

					if (!flag) {
						for (int j = 0; j < victim; j++) {
							if (reference_bit[j] == 0) {
								page_frame[j] = page_reference[i];
								page_fault++;
								replace = true;
								victim = j + 1;
								flag = true;
								break;
							}
							else if (reference_bit[j] == 1) {
								reference_bit[j] = 0;
							}
						}
					}

					if (!flag) {
						page_frame[victim] = page_reference[i];
						victim++;
						page_fault++;
						replace = true;
					}
				}

				if (victim == frame_size)
					victim = 0;

				if (i < 9) printf("%d               ", i + 1);
				else printf("%d              ", i + 1);
				for (int j = 0; j < frame_size; j++) {
					if (page_frame[j] < 0) printf("        ");
					else printf("%d       ", page_frame[j]);
				}
				if (replace) printf("F\n");
				else printf("\n");				
			}
			printf("Number of page faults : %d times\n\n", page_fault);
		}
	}

	fclose(fp);

	return 0;
}
