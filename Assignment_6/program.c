#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

void *job0(void *arg);	// 4 개의 Thread를 관리하는 Thread
void *job1(void *arg);	// P1 출발 차량을 제어하는 Thread
void *job2(void *arg);	// P2 출발 차량을 제어하는 Thread
void *job3(void *arg);	// P3 출발 차량을 제어하는 Thread
void *job4(void *arg);	// P4 출발 차량을 제어하는 Thread

pthread_mutex_t mutex0 = PTHREAD_MUTEX_INITIALIZER;	// Mutex 변수 초기화
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;	// Mutex 변수 초기화
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;	// Mutex 변수 초기화
pthread_mutex_t mutex3 = PTHREAD_MUTEX_INITIALIZER;	// Mutex 변수 초기화
pthread_mutex_t mutex4 = PTHREAD_MUTEX_INITIALIZER;	// Mutex 변수 초기화
pthread_cond_t cond0 = PTHREAD_COND_INITIALIZER;	// Cond 변수 초기화
pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;	// Cond 변수 초기화
pthread_cond_t cond2 = PTHREAD_COND_INITIALIZER;	// Cond 변수 초기화
pthread_cond_t cond3 = PTHREAD_COND_INITIALIZER;	// Cond 변수 초기화
pthread_cond_t cond4 = PTHREAD_COND_INITIALIZER;	// Cond 변수 초기화

int job1_count;	// 1번 스레드 종료를 위한 Count
int job2_count;	// 2번 스레드 종료를 위한 Count
int job3_count;	// 3번 스레드 종료를 위한 Count
int job4_count;	// 4번 스레드 종료를 위한 Count
int start_point_list[20];	// 출발 지점 리스트
int passed_list[10];		// 교차로를 빠져나가는 차량의 정보 리스트
int waiting_list[20];		// 대기 중인 차량의 정보 리스트
int pCount[4];				// 각 출발 지점의 수
int process[4];				// 각 출발 지점에서의 진행 상황 저장

int vehicles;		// 차량의 수
int tick = 1;		// 걸린 시간
int list_index;		// start_point_list 인덱스
int passed_index;	// passed_list 인덱스
int waiting_index;	// waiting_list 인덱스
int key;			// 1 초마다 출발할 차량을 선택하는 변수
int start;			// 1 초마다 start_point_list에서 가져온 값
int where;			// 현재 도로를 점유하는 차량의 출발점 저장한 변수
int where_temp;		// 반대 편에서 출발할 경우를 만족시키기 위한 변수
bool finish[4];		// 4 개의 스레드 종료를 위한 조건을 만족시키는 변수
pthread_t tid[5];	// 제어하는 스레드를 포함한 5 개의 스레드

void create_random_list(void);
void create_thread();
void print_progress(void);
void print_result(void);

int main(void) 
{
	memset(start_point_list, 0, sizeof(start_point_list));
	memset(passed_list, 0, sizeof(passed_list));
	memset(waiting_list, 0, sizeof(waiting_list));
	memset(pCount, 0, sizeof(pCount));
	memset(process, 0, sizeof(process));

	create_random_list();	
	create_thread();

	start = start_point_list[list_index++];

	switch (start) {
		case 1:
			pthread_cond_signal(&cond1);
			break;
		case 2:
			pthread_cond_signal(&cond2);
			break;
		case 3:
			pthread_cond_signal(&cond3);
			break;
		case 4:
			pthread_cond_signal(&cond4);
			break;
	}

	for (int i = 0; i < 5; i++) {
		pthread_join(tid[i], NULL);
	}

	pthread_mutex_destroy(&mutex0);
	pthread_mutex_destroy(&mutex1);
	pthread_mutex_destroy(&mutex2);
	pthread_mutex_destroy(&mutex3);
	pthread_mutex_destroy(&mutex4);
	pthread_cond_destroy(&cond0);
	pthread_cond_destroy(&cond1);
	pthread_cond_destroy(&cond2);
	pthread_cond_destroy(&cond3);
	pthread_cond_destroy(&cond4);
	print_result();	

	return 0;
}

void create_random_list(void)
{
	printf("Total number of vehicles : ");
	scanf("%d", &vehicles);
	printf("Start point : ");
	srand((unsigned int)time(NULL));
	
	for (int i = 0; i < vehicles; i++) {
		int start_point = (rand() % 4) + 1;
		start_point_list[i] = start_point;
		pCount[start_point_list[i] - 1]++;
		printf("%d ", start_point_list[i]);
	}
	printf("\n\n");
}

void create_thread() 
{
	if (pthread_create(&tid[0], NULL, job0, NULL) != 0) {
		fprintf(stderr, "pthread_create error\n");
		exit(1);
	}

	if (pthread_create(&tid[1], NULL, job1, NULL) != 0) {
		fprintf(stderr, "pthread_create error\n");
		exit(1);
	}

	if (pthread_create(&tid[2], NULL, job2, NULL) != 0) {
		fprintf(stderr, "pthread_create error\n");
		exit(1);
	}

	if (pthread_create(&tid[3], NULL, job3, NULL) != 0) {
		fprintf(stderr, "pthread_create error\n");
		exit(1);
	}

	if (pthread_create(&tid[4], NULL, job4, NULL) != 0) {
		fprintf(stderr, "pthread_create error\n");
		exit(1);
	}

	sleep(1);
}

void print_progress(void) 
{
	printf("tick : %d", tick++);
	printf("\n=============================\n");

	printf("Passed Vehicle\nCar ");
	for (int i = 0; i < 10; i++)
		if (passed_list[i] != 0)
			printf("%d ", passed_list[i]);

	printf("\nWaiting Vehicle\nCar ");
	for (int i = 0; i < 20; i++)
		if (waiting_list[i] != 0)
			printf("%d ", waiting_list[i]);

	printf("\n=============================\n");
}

void print_result(void) 
{
	printf("Number of vehicles passed from each start point\n");
	printf("P1 : %d times\n", pCount[0]);
	printf("P2 : %d times\n", pCount[1]);
	printf("P3 : %d times\n", pCount[2]);
	printf("P4 : %d times\n", pCount[3]);
	printf("Total time : %d ticks\n", tick - 1);
}

void *job0(void *arg)
{
	bool temp = false;
	bool init = false;
	bool check = false;
	
	while (1) {
		pthread_mutex_lock(&mutex0);

		if (!init) {
			pthread_cond_wait(&cond0, &mutex0);
			init = true;
		}
		
		print_progress();
		sleep(1);

		passed_index = 0;
		for (int i = 0; i < 10; i++)
			passed_list[i] = 0;

		start = start_point_list[list_index++];
		if (start > 0)
			waiting_list[waiting_index++] = start;
		
		if (temp) {
			where = where_temp;
			temp = false;
		}
		
		int idx;
		check = false;
		key = 0;

		for (int i = 0; i < 20; i++) {
			if ((where == 1 && waiting_list[i] == 3) || (where == 2 && waiting_list[i] == 4) || (where == 3 && waiting_list[i] == 1) || (where == 4 && waiting_list[i] == 2)) {
				key = waiting_list[i];
				waiting_list[i] = 0;
				idx = i;
				check = true;
				break;
			}
		}

		if (!check) {
			for (int i = 0; i < 20; i++) {
				if (waiting_list[i] > 0) {
					key = waiting_list[i];
					waiting_list[i] = 0;
					idx = i;
					break;
				}
			}
		}

		if (finish[0] == true && finish[1] == true && finish[2] == true && finish[3] == true) {
			break;
		}

		switch (where) {
			case 0:
				switch (key) {
					case 1:
						pthread_cond_signal(&cond1);
						break;
					case 2:
						pthread_cond_signal(&cond2);
						break;
					case 3:
						pthread_cond_signal(&cond3);
						break;
					case 4:
						pthread_cond_signal(&cond4);
						break;
				}
				break;
			case 1:
				switch (key) {
					case 1:
						waiting_list[idx] = 1;
						break;
					case 2:
						waiting_list[idx] = 2;
						break;
					case 3:
						pthread_cond_signal(&cond3);
						pthread_cond_wait(&cond0, &mutex0);
						where_temp = where;
						temp = true;
						break;
					case 4:
						waiting_list[idx] = 4;
						break;
				}
				pthread_cond_signal(&cond1);
				break;
			case 2:
				switch (key) {
					case 1:
						waiting_list[idx] = 1;
						break;
					case 2:
						waiting_list[idx] = 2;
						break;
					case 3:
						waiting_list[idx] = 3;
						break;
					case 4:
						pthread_cond_signal(&cond4);
						pthread_cond_wait(&cond0, &mutex0);
						where_temp = where;
						temp = true;
						break;
				}
				pthread_cond_signal(&cond2);
				break;
			case 3:
				switch (key) {
					case 1:
						pthread_cond_signal(&cond1);
						pthread_cond_wait(&cond0, &mutex0);
						where_temp = where;
						temp = true;
						break;
					case 2:
						waiting_list[idx] = 2;
						break;
					case 3:
						waiting_list[idx] = 3;
						break;
					case 4:
						waiting_list[idx] = 4;
						break;
				}
				pthread_cond_signal(&cond3);
				break;
			case 4:
				switch (key) {
					case 1:
						waiting_list[idx] = 1;
						break;
					case 2:
						pthread_cond_signal(&cond2);
						pthread_cond_wait(&cond0, &mutex0);
						where_temp = where;
						temp = true;
						break;
					case 3:
						waiting_list[idx] = 3;
						break;
					case 4:
						waiting_list[idx] = 4;
						break;
				}
				pthread_cond_signal(&cond4);
				break;
		}
		
		pthread_cond_wait(&cond0, &mutex0);
		pthread_mutex_unlock(&mutex0);
	}
	pthread_exit(NULL);
	return NULL;
}

void *job1(void *arg)
{
	bool init = false;

	if (pCount[0] == 0) {
		finish[0] = true;
		pthread_exit(NULL);
	}

	while (1) {
		pthread_mutex_lock(&mutex1);

		if (!init)	{
			init = true;
			pthread_cond_wait(&cond1, &mutex1);
		}

		process[0]++;
		if (process[0] == 1) {
			where = 1;
		}
		else if (process[0] == 2) {
			passed_list[passed_index++] = 1;
			job1_count++;
			process[0] = 0;
			where = 0;
		}

		if (job1_count == pCount[0]) {
			finish[0] = true;
			pthread_cond_signal(&cond0);
			pthread_mutex_unlock(&mutex1);
			break;
		}
		
		pthread_cond_signal(&cond0);
		pthread_cond_wait(&cond1, &mutex1);
		pthread_mutex_unlock(&mutex1);
	}
	return NULL;
}

void *job2(void *arg) 
{
	bool init = false;

	if (pCount[1] == 0) {
		finish[1] = true;
		pthread_exit(NULL);
	}

	while (1) {
		pthread_mutex_lock(&mutex2);

		if (!init)	{
			init = true;
			pthread_cond_wait(&cond2, &mutex2);
		}
		
		process[1]++;
		if (process[1] == 1) {
			where = 2;
		}
		else if (process[1] == 2) {
			passed_list[passed_index++] = 2;
			job2_count++;
			process[1] = 0;
			where = 0;
		}
		
		if (job2_count == pCount[1]) {
			finish[1] = true;
			pthread_cond_signal(&cond0);
			pthread_mutex_unlock(&mutex2);
			break;
		}

		pthread_cond_signal(&cond0);
		pthread_cond_wait(&cond2, &mutex2);
		pthread_mutex_unlock(&mutex2);
	}
	return NULL;
}

void *job3(void *arg) 
{
	bool init = false;

	if (pCount[2] == 0) {
		finish[2] = true;
		pthread_exit(NULL);
	}

	while (1) {
		pthread_mutex_lock(&mutex3);

		if (!init)	{
			init = true;
			pthread_cond_wait(&cond3, &mutex3);
		}

		process[2]++;
		if (process[2] == 1) {
			where = 3;
		}
		else if (process[2] == 2) {
			passed_list[passed_index++] = 3;
			job3_count++;
			process[2] = 0;
			where = 0;
		}

		if (job3_count == pCount[2]) {
			finish[2] = true;
			pthread_cond_signal(&cond0);
			pthread_mutex_unlock(&mutex3);
			break;
		}

		pthread_cond_signal(&cond0);
		pthread_cond_wait(&cond3, &mutex3);
		pthread_mutex_unlock(&mutex3);
	}
	return NULL;
}

void *job4(void *arg) 
{
	bool init = false;

	if (pCount[3] == 0) {
		finish[3] = true;
		pthread_exit(NULL);
	}

	while (1) {
		pthread_mutex_lock(&mutex4);

		if (!init)	{
			init = true;
			pthread_cond_wait(&cond4, &mutex4);
		}

		process[3]++;
		if (process[3] == 1) {
			where = 4;
		}
		else if (process[3] == 2) {
			passed_list[passed_index++] = 4;
			job4_count++;
			process[3] = 0;
			where = 0;
		}

		if (job4_count == pCount[3]) {
			finish[3] = true;
			pthread_cond_signal(&cond0);
			pthread_mutex_unlock(&mutex4);
			break;
		}

		pthread_cond_signal(&cond0);
		pthread_cond_wait(&cond4, &mutex4);
		pthread_mutex_unlock(&mutex4);
	}
	return NULL;
}
