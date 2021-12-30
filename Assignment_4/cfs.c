#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

#define WORK 10000

void task(void) {
	int temp = 0;

	for (int i = 1; i <= WORK; i++) {
		for (int j = 1; j <= WORK; j++) {
			temp = i + j;
		}
	}
}

int main(void)
{
	pid_t pid;
	
	printf("\n========= 프로그램 시작 =========\n");
	printf("\n# 프로세스 생성 순서\n");
	for (int i = 0; i < 21; i++) {
		pid = fork();

		if (pid < 0) {
			fprintf(stderr, "fork error\n");
			exit(1);
		}
		else if (pid == 0) {
			task();
			exit(0);
		}
		else {
			printf("[%d] process begins\n", pid);
		}
	}
	
	printf("\n# 프로세스 종료 순서\n");
	while ((pid = wait(NULL)) > 0) {
		printf("[%d] process ends\n", pid);
	}
	
	printf("\n======== 프로그램 종료 ========\n\n");
		
	exit(0);
}


