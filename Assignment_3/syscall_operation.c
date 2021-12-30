#include <stdio.h>
#include <stdlib.h>
#include <linux/kernel.h>
#include <sys/syscall.h>
#include <unistd.h>

int main() {
	char op;
	int num1, num2, check;
	int *result;

	system("clear");
	printf("[4가지 연산 이항 계산기 프로그램]  ");
	printf("(^C를 누르면 종료합니다...)\n");
	while (1) {
		printf(">> ");
		if (scanf("%d %c %d", &num1, &op, &num2) != 3) {
			printf("[Error] 입력 오류! 다시 입력해주세요.\n\n");
			while (getchar() != '\n');
			continue;
		}
		
		switch (op) {
			case '+':
				check = syscall(443, num1, num2, result);
				break;
			case '-':
				check = syscall(444, num1, num2, result);
				break;
			case '*':
				check = syscall(445, num1, num2, result);
				break;
			case '%':
				check = syscall(446, num1, num2, result);
				break;
			default:
				printf("[Error] +, -, *, %% 연산만 가능합니다.\n\n");
				continue;
		}
		
		printf("[결과] %d %c %d = %d\n\n", num1, op, num2, *result);
	}
	return 0;
}

