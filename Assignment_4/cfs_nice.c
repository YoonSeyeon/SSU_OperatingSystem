#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/syscall.h>

#define EASY 20000
#define NORMAL 50000
#define HARD 100000

pid_t begin_order[21];
pid_t end_order[21];

void easy_task(void) {
	int temp;
	for (int i = 0; i < EASY; i++) {
		for (int j = 0; j < EASY; j++) {
			temp = i + j;
		}

		if (i % (EASY / 10) == 0) printf("EASY WORK\n");
	}
}

void normal_task(void) {
	int temp;
	for (int i = 0; i < NORMAL; i++) {
		for (int j = 0; j < NORMAL; j++) {
			temp = i + j;
		}
		
		if (i % (NORMAL / 10) == 0) printf("NORMAL WORK\n");
	}
}

void hard_task(void) {
	int temp;
	for (int i = 0; i < HARD; i++) {
		for (int j = 0; j < HARD; j++) {
			temp = i + j;
		}
		
		if (i % (HARD / 10) == 0) printf("HARD WORK\n");
	}
}

int main(void)
{
	pid_t pid;
	
	printf("\n========= 프로그램 시작 =========\n");
	printf("\n# 프로세스 작업 상황\n");
	for (int i = 0; i < 21; i++) {
		pid = fork();

		if (pid < 0) {
			fprintf(stderr, "fork error\n");
			exit(1);
		}
		else if (pid == 0) {
			if ((i / 7) == 0) {
				nice(19);
				easy_task();
			}
			else if ((i / 7) == 1) {
				nice(0);
				normal_task();
			}
			else if ((i / 7) == 2) {
				nice(-20);
				hard_task();
			}
			exit(0);
		}
		else {
			begin_order[i] = pid;
		}
	}

	int idx = 0;
	while ((pid = wait(NULL)) > 0) {
		end_order[idx] = pid;
		idx++;
	}
	printf("\n# 프로세스 생성 순서\n");
	for (int i = 0; i < 21; i++) printf("[%d] process begins\n", begin_order[i]);

	printf("\n# 프로세스 종료 순서\n");
	for (int i = 0; i < 21; i++) printf("[%d] process ends\n", end_order[i]);

	printf("\n======== 프로그램 종료 ========\n\n");

	exit(0);
}


