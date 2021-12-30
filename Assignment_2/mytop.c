#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <math.h>
#include <limits.h>
#include <ctype.h>
#include <dirent.h>
#include <pwd.h>
#include <utmp.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <ncurses.h>

#define BUFFER_SIZE 1024		// 버퍼 최대 길이
#define PATH_SIZE 1024			// 경로 이름 최대 길이
#define TOKEN_LEN 32			// proc/pid/stat에서 얻는 값들의 최대 길이
#define FNAME_LEN 128			// 파일 이름 최대 길이

#define MAX_TOKEN 22			// /proc/pid/stat에서 읽어들일 token 갯수

#define UNAME_LEN 32			// user 이름의 길이
#define TTY_LEN 32				// 터미널 경로 이름의 길이
#define STAT_LEN 8
#define TIME_LEN 16				// 시간을 표시할 때의 길이
#define CMD_LEN 1024

#define PID_MAX 32768			// pid 최대 갯수
#define PROCESS_MAX 4096		// process 최대 갯수

#define TICK_CNT 8				// cpu ticks 개수, us sy ni id wa hi si st로 총 8개

#define SYSTEMD "systemd-"		// system USER명의 앞부분
#define DEVNULL "/dev/null"		// 터미널 없을 시 가리키는 /dev/null 절대 경로
#define PTS "pts/"				// 터미널 문자열의 앞부분

#define PROC "/proc"			// /proc 절대 경로
#define LOADAVG "/proc/loadavg"	// /proc/loadavg 절대 경로
#define UPTIME "/proc/uptime"	// /proc/uptime 절대 경로
#define CPUSTAT "/proc/stat"	// /proc/stat 절대 경로
#define MEMINFO "/proc/meminfo"	// /proc/meminfo 절대 경로
#define DEV "/dev"				// /dev 절대 경로

#define FD__ZERO "/fd/0"		// /proc/pid에서의 0번째 fd 경로
#define STAT "/stat"			// /proc/pid에서의 stat 경로
#define STATUS "/status"		// /proc/pid에서의 status 경로
#define CMDLINE "/cmdline"		// /proc/pid에서의 cmdline 경로
#define VMSIZE "VmSize"			// /proc/pid/status에서 VmSize 확인하기 위한 문자열

// /proc/pid/stat에서의 idx
#define STAT_PID_IDX 0
#define STAT_CMD_IDX 1
#define STAT_STATE_IDX 2
#define STAT_PPID_IDX 3
#define STAT_SID_IDX 5
#define STAT_TTY_NR_IDX 6
#define STAT_TPGID_IDX 7
#define STAT_UTIME_IDX 13
#define STAT_STIME_IDX 14
#define STAT_PRIORITY_IDX 17
#define STAT_NICE_IDX 18
#define STAT_N_THREAD_IDX 19
#define STAT_START_TIME_IDX 21

// /proc/pid/status에서의 row
#define STATUS_VSZ_ROW 18
#define STATUS_VMLCK_ROW 19
#define STATUS_RSS_ROW 22
#define STATUS_SHR_ROW 24

// /proc/stat 에서의 idx
#define CPU_STAT_US_IDX 1
#define CPU_STAT_NI_IDX 2
#define CPU_STAT_SY_IDX 3
#define CPU_STAT_ID_IDX 4
#define CPU_STAT_WA_IDX 5
#define CPU_STAT_HI_IDX 6
#define CPU_STAT_SI_IDX 7
#define CPU_STAT_ST_IDX 8

// /proc/meminfo 에서의 row
#define MEMINFO_MEM_TOTAL_ROW 1
#define MEMINFO_MEM_FREE_ROW 2
#define MEMINFO_MEM_AVAILABLE_ROW 3
#define MEMINFO_BUFFERS_ROW 4
#define MEMINFO_CACHED_ROW 5
#define MEMINFO_SWAP_TOTAL_ROW 15
#define MEMINFO_SWAP_FREE_ROW 16
#define MEMINFO_S_RECLAIMABLE_ROW 24

// column에 출력할 문자열
#define UID_STR "UID"
#define PID_STR "PID"
#define PPID_STR "PPID"
#define USER_STR "USER"
#define PR_STR "PR"
#define NI_STR "NI"
#define VSZ_STR "VSZ"
#define VIRT_STR "VIRT"
#define RSS_STR "RSS"
#define RES_STR "RES"
#define SHR_STR "SHR"
#define S_STR "S"
#define C_STR "C"
#define STAT_STR "STAT"
#define START_STR "START"
#define STIME_STR "STIME"
#define TTY_STR "TTY"
#define CPU_STR "%CPU"
#define MEM_STR "%MEM"
#define TIME_STR "TIME"
#define TIME_P_STR "TIME+"
#define CMD_STR "CMD"
#define COMMAND_STR "COMMAND"

#define TAB_WIDTH 8		// tab 길이

#define COLUMN_CNT 12	// 출력할 column 최대 갯수

// 출력 시 사용할 columnWidth 배열에서의 index
#define PID_IDX 0
#define USER_IDX 1
#define PR_IDX 2
#define NI_IDX 3
#define VIRT_IDX 4
#define RES_IDX 5
#define SHR_IDX 6
#define S_IDX 7
#define CPU_IDX 8
#define MEM_IDX 9
#define TIME_P_IDX 10
#define COMMAND_IDX 11

#define TOP_ROW 0			// top 출력할 행
#define TASK_ROW 1			// task 출력할 행
#define CPU_ROW 2			// cpu 출력할 행
#define MEM_ROW 3			// mem 출력할 행
#define CMD_ROW 5			// kill 명령어 출력할 행
#define COLUMN_ROW 6		// column 출력할 행

#define CPU_MODE 1			// CPU 사용률 순으로 정렬
#define MEM_MODE 2			// 메모리 사용량 순으로 정렬
#define PID_MODE 3			// 프로세스ID 순으로 정렬
#define TIME_MODE 4			// 실행시간 기준 정렬

#define KIB 1
#define MIB 2
#define GIB 3
#define TIB 4
#define PIB 5
#define EIB 6

#define ENTER '\n'			// 실행 중 입력 시 즉시 갱신
#define SPACE ' '			// 실행 중 입력 시 즉시 갱신
#define K 'k'				// k 입력 후 종료할 PID 입력 시 프로세스 종료
#define SHIFT_R 'R'			// 오름차순 or 내림차순 변경
#define SHIFT_P 'P'			// 실행 중 입력 시 CPU 사용률이 큰 순서로 정렬
#define SHIFT_M 'M'			// 실행 중 입력 시 메모리 사용량이 큰 순서로 정렬
#define SHIFT_N 'N'			// 실행 중 입력 시 PID 기준 내림차순 정렬
#define SHIFT_T 'T'			// 실행 중 입력 시 실행된 시간이 큰 순서로 정렬
#define SHIFT_E 'E'			// 실행 중 입력 시 4~5행의 메모리 출력 바이트 단위 변경

// process를 추상화 한 myProc 구조체
typedef struct{
	unsigned long pid;			// 프로세스 id
	unsigned long uid;			// USER 구하기 위한 uid
	unsigned long ppid;			// 프로세스의 부모 id
	unsigned long runtime;		// 프로세스 cpu 사용 시간 정수형
	char user[UNAME_LEN];		// user명
	long double cpu;			// cpu 사용률
	long double mem;			// 메모리 사용률
	unsigned long vsz;			// 가상 메모리 사용량
	unsigned long rss;			// 실제 메모리 사용량
	unsigned long shr;			// 공유 메모리 사용량
	int priority;				// 우선순위
	int nice;					// nice 값
	char tty[TTY_LEN];			// 터미널
	char stat[STAT_LEN];		// 상태
	char start[TIME_LEN];		// 프로세스 시작 시각
	char time[TIME_LEN];		// 총 cpu 사용 시간
	char cmd[CMD_LEN];			// option 없을 경우에만 출력되는 command (short)
	char command[CMD_LEN];		// option 있을 경우에 출력되는 command (long)

}myProc;

void PrintTOP(void);
void SortProc(void);
void ClearSCR(void);
bool isGreater(myProc *, myProc *);
long double RoundDouble(long double src, int rdx);
void GetTTY(char path[PATH_SIZE], char tty[TTY_LEN]);
unsigned long GetUpTime(void);
unsigned long GetTotalMemory(void);
void AddProcessList(char path[PATH_SIZE], unsigned long cpuTimeTable[]);
void SearchProcess(unsigned long cpuTimeTable[]);
void EraseProcess(myProc *proc);
void EraseProcessList(void);
void KillProcess(void);

unsigned long cpuTimeTable[PID_MAX];	// cpu의 이전 시각 저장할 hash table

myProc procList[PROCESS_MAX];	// 완성한 myProc의 포인터 저장 배열
myProc *sorted[PROCESS_MAX];	// procList를 cpu 순으로 sorting한 myProc 포인터 배열

int procCnt = 0;				// 현재까지 완성한 myProc 갯수

time_t before;
time_t now;

unsigned long uptime;			// os 부팅 후 지난 시간
unsigned long beforeUptime;		// 이전 실행 시의 os 부팅 후 지난 시각
unsigned long total_memory;		// 전체 물리 메모리 크기
unsigned long hertz;	 		// 초당 context switching 횟수
unsigned long updateTime;		// d Option 사용 시 값 만큼의 시간마다 화면 갱신
unsigned long updatePeriod;		// n Option 사용 시 값 만큼 출력 반복 후 종료

long double beforeTicks[TICK_CNT] = {0, };	// 이전의 cpu ticks 저장하기 위한 배열

pid_t myPid;					// 자기 자신의 pid
uid_t myUid;					// 자기 자신의 uid
char myPath[PATH_SIZE];			// 자기 자신의 path
char myTTY[TTY_LEN];			// 자기 자신의 tty

int ch;
int row, col;

bool dOption = false;			// mytop -d [sec] 설정된 초 단위 마다 갱신
bool nOption = false;			// mytop -n [iter] 설정된 수 만큼 반복하고 종료
bool killMode = false;			// 입력한 pid 프로세스 종료
bool reverseMode = false;		// 오름차순 / 내림차순 Toggle
int sortMode = CPU_MODE;		// 기본은 %CPU 순으로 정렬
int byteUnit = MIB;				// 기본 Mib 단위로 메모리 출력

int main(int argc, char *argv[]) {

    total_memory = GetTotalMemory();			// 전체 물리 메모리 크기
	hertz = (unsigned int)sysconf(_SC_CLK_TCK);	// 초당 context switching 횟수
	now = time(NULL);

	memset(cpuTimeTable, (unsigned long)0, PID_MAX);

	myPid = getpid();			// 자기 자신의 pid

	char pidPath[FNAME_LEN];
	memset(pidPath, '\0', FNAME_LEN);
	sprintf(pidPath, "/%d", myPid);

	strcpy(myPath, PROC);			// 자기 자신의 /proc 경로 획득
	strcat(myPath, pidPath);

	GetTTY(myPath, myTTY);			// 자기 자신의 tty 획득
	for (int i = strlen(PTS); i < strlen(myTTY); i++)
		if (!isdigit(myTTY[i])) {
			myTTY[i] = '\0';
			break;
		}

	myUid = getuid();		// 자기 자신의 uid

	initscr();				// 출력 윈도우 초기화
	halfdelay(10);			// 0.1초마다 입력 새로 갱신
	noecho();				// echo 제거 - 입력 내용이 화면에 출력되지 않음
	keypad(stdscr, TRUE);	// 특수 키 입력 허용
	curs_set(0);			// 0 : 커서가 보이지 않음, 1 : 커서가 보임

	SearchProcess(cpuTimeTable);

	row = 0, col = 0, ch = 0;

	bool print = false;
	pid_t pid;

	before = time(NULL);

	if (argc > 2) {
		for (int i = 0; i < strlen(argv[1]); i++) {
			if (argv[1][i] == 'd') {
				updateTime = atoi(argv[2]);
				dOption = true;
				break;
			}
			
			if (argv[1][i] == 'n') {
				updatePeriod = atoi(argv[2]);
				nOption = true;
				break;
			}
		} 
	}

	SortProc();				// CPU 사용률 순으로 정렬
	PrintTOP();			    // 초기 출력
	refresh();

	do {					// 무한 반복

		now = time(NULL);	// 현재 시각 갱신

		switch(ch) {		// 실행 중 입력 처리
			case KEY_LEFT:
				col--;
				if(col < 0)
					col = 0;
				print = true;
				break;
			case KEY_RIGHT:
				col++;
				print = true;
				break;
			case KEY_UP:
				row--;
				if(row < 0)
					row = 0;
				print = true;
				break;
			case KEY_DOWN:
				row++;
				if(row > procCnt)
					row = procCnt;
				print = true;
				break;
			case ENTER:
				print = true;
				break;
			case SPACE:
				print = true;
				break;
			case SHIFT_R:
				print = true;
				if (!reverseMode) reverseMode = true;
				else reverseMode = false;
				break;
			case SHIFT_P:
				print = true;
				sortMode = CPU_MODE;
				break;
			case SHIFT_M:
				print = true;
				sortMode = MEM_MODE;
				break;
			case SHIFT_N:
				print = true;
				sortMode = PID_MODE;
				break;
			case SHIFT_T:
				print = true;
				sortMode = TIME_MODE;
				break;
			case SHIFT_E:
				print = true;
				byteUnit++;
				if (byteUnit > 6) byteUnit = KIB;
				break;
			case K:
				print = true;
				killMode = true;
				break;
		}
		
		if (!dOption)
			updateTime = 3;

		if (print || now - before >= updateTime) {	// 화면 갱신
			
			if (ch == ENTER || ch == SPACE)
				updateTime = 0;

			KillProcess();
			erase();
			EraseProcessList();
			SearchProcess(cpuTimeTable);
			SortProc();								// 모드에 따라 프로세스 정렬
			PrintTOP();
			refresh();
			before = now;
			print = false;

			if (nOption) {
				updatePeriod--;
				if (updatePeriod == 0) break;
			}
		}

	} while ((ch = getch()) != 'q');    //q 입력 시 종료

	endwin();

	return 0;	
}

// 실제 화면에 출력하는 함수
void PrintTOP(void) {
	uptime = GetUpTime();			// os 부팅 후 지난 시각
    char buf[BUFFER_SIZE];

	/*****	1행 UPTIME 출력	*****/
	char nowStr[128];				// 현재 시각 문자열
	memset(nowStr, '\0', 128);
	struct tm *tmNow = localtime(&now);
	sprintf(nowStr, "top - %02d:%02d:%02d ", tmNow->tm_hour, tmNow->tm_min, tmNow->tm_sec);

	struct tm *tmUptime = gmtime(&uptime);

	char upStr[128];				// uiptime 문자열
	memset(upStr, '\0', 128);
	if (uptime < 60 * 60)
		sprintf(upStr, "%2d min", tmUptime->tm_min);
	else if (uptime < 60 * 60 * 24)
		sprintf(upStr, "%2d:%02d", tmUptime->tm_hour, tmUptime->tm_min);
	else 
		sprintf(upStr, "%3d days, %02d:%02d", tmUptime->tm_yday, tmUptime->tm_hour, tmUptime->tm_min);

	int users = 0;							// active user 수 구하기

	struct utmp *utmp;
	setutent();								// utmp 처음부터 읽기
	while ((utmp = getutent()) != NULL)		// /proc/utmp 파일에서 null일 때까지 읽어들이기
		if (utmp->ut_type == USER_PROCESS)	// ut_type이 USER일 경우에만 count 증가
			users++;

	FILE *loadAvgFp;
	long double loadAvg[3];
	if ((loadAvgFp = fopen(LOADAVG, "r")) == NULL) {
		fprintf(stderr, "fopen error for %s\n", LOADAVG);
		exit(1);
	}
	memset(buf, '\0', BUFFER_SIZE);
	fgets(buf, BUFFER_SIZE, loadAvgFp);
	fclose(loadAvgFp);
	sscanf(buf, "%Lf%Lf%Lf", &loadAvg[0], &loadAvg[1], &loadAvg[2]);

	mvprintw(TOP_ROW, 0, "%sup %s, %d users, load average: %4.2Lf, %4.2Lf, %4.2Lf", nowStr, upStr, users, loadAvg[0], loadAvg[1], loadAvg[2]);

	/*****	2행 Task 출력	*****/
	char *ptr;
	
	unsigned int total = 0, running = 0, sleeping = 0, stopped = 0, zombie = 0;
	total = procCnt;
	for (int i = 0; i < procCnt; i++) {
		if (!strcmp(procList[i].stat, "R"))
			running++;
		else if (!strcmp(procList[i].stat, "D"))
			sleeping++;
		else if (!strcmp(procList[i].stat, "S"))
			sleeping++;
		else if (!strcmp(procList[i].stat, "I"))
			sleeping++;
		else if (!strcmp(procList[i].stat, "T"))
			stopped++;
		else if (!strcmp(procList[i].stat, "t"))
			stopped++;
		else if (!strcmp(procList[i].stat, "Z"))
			zombie++;
	}
	mvprintw(TASK_ROW, 0, "Tasks: %4u total, %4u running, %4u sleeping, %4u stopped, %4u zombie", total, running, sleeping, stopped, zombie);

	/*****	3행 %CPU 출력	*****/
	long double us, sy, ni, id, wa, hi, si, st;

	FILE *cpuStatFp;
	if ((cpuStatFp = fopen(CPUSTAT, "r")) == NULL) {		// /proc/stat fopen
		fprintf(stderr, "fopen error for %s\n", CPUSTAT);
		exit(1);
	}
	memset(buf, '\0', BUFFER_SIZE);
    fgets(buf, BUFFER_SIZE, cpuStatFp);
	fclose(cpuStatFp);
	ptr = buf;
	while (!isdigit(*ptr)) ptr++;

	long double ticks[TICK_CNT] = {0.0, };

	sscanf(ptr, "%Lf%Lf%Lf%Lf%Lf%Lf%Lf%Lf", ticks+0, ticks+1, ticks+2, ticks+3, 
			ticks+4, ticks+5, ticks+6, ticks+7);	//ticks read

	unsigned long tickCnt = 0;
	long double results[TICK_CNT] = {0.0, }; // 출력할 ticks 값
	if (beforeUptime == 0) {
		tickCnt = uptime * hertz;			 // 부팅 후 현재까지 context switching 횟수
		for(int i = 0; i < TICK_CNT; i++)	 // 읽은 ticks 그대로 출력
			results[i] = ticks[i];
	}
	else {
		tickCnt = (uptime - beforeUptime) * hertz;	// 부팅 후 현재까지 context switching 횟수
		for(int i = 0; i < TICK_CNT; i++)
			results[i] = ticks[i] - beforeTicks[i];	// 이전에 저장한 tick수를 빼서 출력
	}

	for (int i = 0; i < TICK_CNT; i++) {
		results[i] = (results[i] / tickCnt) * 100.0;	//퍼센트로 저장
		if(isnan(results[i]) || isinf(results[i]))		//예외 처리
			results[i] = 0;
	}	

	mvprintw(CPU_ROW, 0, "%%Cpu(s): %4.1Lf us, %4.1Lf sy, %4.1Lf ni, %4.1Lf id, %4.1Lf wa, %4.1Lf hi, %4.1Lf si, %4.1Lf st", results[0], results[2], results[1], results[3], results[4], results[5], results[6], results[7]);

	beforeUptime = uptime;								//갱신
	for(int i = 0; i < TICK_CNT; i++)					
		beforeTicks[i] = ticks[i];

	/*****	4,5행 MEM SWAP출력	*****/
	unsigned long memTotal, memFree, memUsed, memAvailable, buffers, cached, sReclaimable, swapTotal, swapFree, swapUsed;
	long double d_memTotal, d_memFree, d_memUsed, d_buffcache, d_swapTotal, d_swapFree, d_swapUsed, d_memAvailable;

    FILE *meminfoFp;

    if ((meminfoFp = fopen(MEMINFO, "r")) == NULL) {	// /proc/meminfo open
		fprintf(stderr, "fopen error for %s\n", MEMINFO);
        exit(1);
    }

	int i = 0;
	while (i < MEMINFO_MEM_TOTAL_ROW) {	//memTotal read
		memset(buf, '\0', BUFFER_SIZE);
    	fgets(buf, BUFFER_SIZE, meminfoFp);
		i++;
	}
	ptr = buf;
	while (!isdigit(*ptr)) ptr++;
    sscanf(ptr, "%lu", &memTotal);		// /proc/meminfo의 1행에서 memFree read


	while (i < MEMINFO_MEM_FREE_ROW) {	//memFree read
		memset(buf, '\0', BUFFER_SIZE);
    	fgets(buf, BUFFER_SIZE, meminfoFp);
		i++;
	}

	ptr = buf;
	while (!isdigit(*ptr)) ptr++;
    sscanf(ptr, "%lu", &memFree);	// /proc/meminfo의 2행에서 memFree read


	while (i < MEMINFO_MEM_AVAILABLE_ROW) {	//memAvailable read
		memset(buf, '\0', BUFFER_SIZE);
    	fgets(buf, BUFFER_SIZE, meminfoFp);
		i++;
	}
	ptr = buf;
	while (!isdigit(*ptr)) ptr++;
    sscanf(ptr, "%lu", &memAvailable);	// /proc/meminfo의 3행에서 memAvailable read


	while (i < MEMINFO_BUFFERS_ROW){	//buffers read
		memset(buf, '\0', BUFFER_SIZE);
    	fgets(buf, BUFFER_SIZE, meminfoFp);
		i++;
	}
	ptr = buf;
	while (!isdigit(*ptr)) ptr++;
    sscanf(ptr, "%lu", &buffers);	// /proc/meminfo의 4행에서 buffers read


	while (i < MEMINFO_CACHED_ROW) {	//cached read
		memset(buf, '\0', BUFFER_SIZE);
    	fgets(buf, BUFFER_SIZE, meminfoFp);
		i++;
	}
	ptr = buf;
	while (!isdigit(*ptr)) ptr++;
    sscanf(ptr, "%lu", &cached);	// /proc/meminfo의 5행에서 cached read


	while (i < MEMINFO_SWAP_TOTAL_ROW) {//swapTotal read
		memset(buf, '\0', BUFFER_SIZE);
    	fgets(buf, BUFFER_SIZE, meminfoFp);
		i++;
	}
	ptr = buf;
	while (!isdigit(*ptr)) ptr++;
    sscanf(ptr, "%lu", &swapTotal);	// /proc/meminfo의 15행에서 swapTotal read


	while (i < MEMINFO_SWAP_FREE_ROW) {	//swapFree read
		memset(buf, '\0', BUFFER_SIZE);
    	fgets(buf, BUFFER_SIZE, meminfoFp);
		i++;
	}
	ptr = buf;
	while (!isdigit(*ptr)) ptr++;
    sscanf(ptr, "%lu", &swapFree);	// /proc/meminfo의 16행에서 swapFree read


	while (i < MEMINFO_S_RECLAIMABLE_ROW) {	//sReclaimable read
		memset(buf, '\0', BUFFER_SIZE);
    	fgets(buf, BUFFER_SIZE, meminfoFp);
		i++;
	}
	ptr = buf;
	while (!isdigit(*ptr)) ptr++;
    sscanf(ptr, "%lu", &sReclaimable);	// /proc/meminfo의 23행에서 sReclaimable read

	memUsed = memTotal - memFree - buffers - cached - sReclaimable;		//memUsed 계산
	swapUsed = swapTotal - swapFree;	//swapUsed 계산

	// Shift + E 옵션에 따라 단위를 변경
	if (byteUnit == KIB) {
		mvprintw(MEM_ROW, 0, "Kib Mem : %8lu total,  %8lu free,  %8lu used,  %8lu buff/cache", memTotal, memFree, memUsed, buffers+cached+sReclaimable); 
		mvprintw(MEM_ROW+1, 0, "Kib Swap: %8lu total,  %8lu free,  %8lu used,  %8lu avail Mem", swapTotal, swapFree, swapUsed, memAvailable); 
	}
	else {
		d_memTotal = (long double)memTotal;
		d_memFree = (long double)memFree;
		d_memUsed = (long double)memUsed;
		d_buffcache = (long double)(buffers+cached+sReclaimable);
		d_swapTotal = (long double)swapTotal;
		d_swapFree = (long double)swapFree;
		d_swapUsed = (long double)swapUsed;
		d_memAvailable = (long double)memAvailable;

		char byte[4];
		memset(byte, 0, sizeof(byte));	

		switch (byteUnit) {
			case MIB:
				strcpy(byte, "MiB");
				break;
			case GIB:
				strcpy(byte, "GiB");
				break;
			case TIB:
				strcpy(byte, "TiB");
				break;
			case PIB:
				strcpy(byte, "PiB");
				break;
			case EIB:
				strcpy(byte, "EiB");
				break;
		}

		for (int i = KIB; i < byteUnit; i++) {
			d_memTotal /= 1024.0;
			d_memFree /= 1024.0;
			d_memUsed /= 1024.0;
			d_buffcache /= 1024.0;
			d_swapTotal /= 1024.0;
			d_swapFree /= 1024.0;
			d_swapUsed /= 1024.0;
			d_memAvailable /= 1024.0;
		}

		mvprintw(MEM_ROW, 0, "%s Mem : %6.1Lf total, %6.1Lf free, %6.1Lf used, %6.1Lf buff/cache", byte, d_memTotal, d_memFree, d_memUsed, d_buffcache); 
		mvprintw(MEM_ROW+1, 0, "%s Swap: %6.1Lf total, %6.1Lf free, %6.1Lf used, %6.1Lf avail Mem", byte, d_swapTotal, d_swapFree, d_swapUsed, d_memAvailable); 
	}
    
	fclose(meminfoFp);

	int columnWidth[COLUMN_CNT] = {				// column의 x축 길이 저장하는 배열
		strlen(PID_STR), strlen(USER_STR), strlen(PR_STR), strlen(NI_STR),
		strlen(VIRT_STR), strlen(RES_STR), strlen(SHR_STR), strlen(S_STR),
		strlen(CPU_STR), strlen(MEM_STR), strlen(TIME_P_STR), strlen(COMMAND_STR) };

	for(int i = 0; i < procCnt; i++){			// PID 최대 길이 저장
		sprintf(buf, "%lu", procList[i].pid);
		if(columnWidth[PID_IDX] < strlen(buf))
			columnWidth[PID_IDX] = strlen(buf);
	}

	for(int i = 0; i < procCnt; i++)			// USER 최대 길이 저장
		if(columnWidth[USER_IDX] < strlen(procList[i].user)){
			columnWidth[USER_IDX] = strlen(procList[i].user);
		}

	for(int i = 0; i < procCnt; i++){			// PR 최대 길이 저장
		sprintf(buf, "%d", procList[i].priority);
		if(columnWidth[PR_IDX] < strlen(buf))
			columnWidth[PR_IDX] = strlen(buf);
	}

	for(int i = 0; i < procCnt; i++){			// NI 최대 길이 저장
		sprintf(buf, "%d", procList[i].nice);
		if(columnWidth[NI_IDX] < strlen(buf))
			columnWidth[NI_IDX] = strlen(buf);
	}

	for(int i = 0; i < procCnt; i++){			// VIRT 최대 길이 저장
		sprintf(buf, "%lu", procList[i].vsz);
		if(columnWidth[VIRT_IDX] < strlen(buf))
			columnWidth[VIRT_IDX] = strlen(buf);
	}

	for(int i = 0; i < procCnt; i++){			// RES 최대 길이 저장
		sprintf(buf, "%lu", procList[i].rss);
		if(columnWidth[RES_IDX] < strlen(buf))
			columnWidth[RES_IDX] = strlen(buf);
	}

	for(int i = 0; i < procCnt; i++){			// SHR 최대 길이 저장
		sprintf(buf, "%lu", procList[i].shr);
		if(columnWidth[SHR_IDX] < strlen(buf))
			columnWidth[SHR_IDX] = strlen(buf);
	}

	for(int i = 0; i < procCnt; i++){			// S 최대 길이 저장
		if(columnWidth[S_IDX] < strlen(procList[i].stat))
			columnWidth[S_IDX] = strlen(procList[i].stat);
	}


	for(int i = 0; i < procCnt; i++){			// CPU 최대 길이 저장
		sprintf(buf, "%3.1Lf", procList[i].cpu);
		if(columnWidth[CPU_IDX] < strlen(buf))
			columnWidth[CPU_IDX] = strlen(buf);
	}

	for(int i = 0; i < procCnt; i++){			// MEM 최대 길이 저장
		sprintf(buf, "%3.1Lf", procList[i].mem);
		if(columnWidth[MEM_IDX] < strlen(buf))
			columnWidth[MEM_IDX] = strlen(buf);
	}

	for(int i = 0; i < procCnt; i++){			// TIME 최대 길이 저장
		if(columnWidth[TIME_P_IDX] < strlen(procList[i].time))
			columnWidth[TIME_P_IDX] = strlen(procList[i].time);
	}

	for(int i = 0; i < procCnt; i++){			// COMMAND 최대 길이 저장
		if(columnWidth[COMMAND_IDX] < strlen(procList[i].command))
			columnWidth[COMMAND_IDX] = strlen(procList[i].command);
	}

	int startX[COLUMN_CNT] = {2, };				// 각 column의 시작 x좌표

	int startCol = 0, endCol = 0;
	int maxCmd = -1;							// COMMAND 출력 가능한 최대 길이

	if(col >= COLUMN_CNT - 1){					// COMMAND COLUMN만 출력하는 경우 (우측 화살표 많이 누른 경우)
		startCol = COMMAND_IDX;
		endCol = COLUMN_CNT;
		maxCmd = COLS;							// COMMAND 터미널 너비만큼 출력 가능
	}
	else{
		int i;
		for(i = col + 1; i < COLUMN_CNT; i++){
			startX[i] = columnWidth[i-1] + 2 + startX[i-1];
			if(startX[i] >= COLS){				// COLUMN의 시작이 이미 터미널 너비 초과한 경우
				endCol = i;
				break;
			}
		}
		startCol = col;
		if(i == COLUMN_CNT){
			endCol = COLUMN_CNT;				// COLUMN 전부 출력하는 경우
			maxCmd = COLS - startX[COMMAND_IDX];// COMMAND 최대 출력 길이: COMMAND 터미널 너비 - COMMAND 시작 x좌표
		}
	}
	

	/*****		6행 column 출력 시작	*****/
	attron(A_REVERSE);	// 색상 반전효과를 on 해서 column 이름이 시각적으로 잘 보임
	for(int i = 0; i < COLS; i++)
		mvprintw(COLUMN_ROW, i, " ");

	int gap = 0;

	//PID 출력
	if(startCol <= PID_IDX && PID_IDX < endCol){
		gap = columnWidth[PID_IDX] - strlen(PID_STR);	// PID의 길이 차 구함
		mvprintw(COLUMN_ROW, startX[PID_IDX] + gap, "%s", PID_STR);	// 우측 정렬
	}

	//USER 출력
	if(startCol <= USER_IDX && USER_IDX < endCol)
		mvprintw(COLUMN_ROW, startX[USER_IDX], "%s", USER_STR);	// 좌측 정렬

	//PR 출력
	if(startCol <= PR_IDX && PR_IDX < endCol){
		gap = columnWidth[PR_IDX] - strlen(PR_STR);		// PR 의 길이 차 구함
		mvprintw(COLUMN_ROW, startX[PR_IDX] + gap, "%s", PR_STR);	// 우측 정렬
	}

	//NI 출력
	if(startCol <= NI_IDX && NI_IDX < endCol){
		gap = columnWidth[NI_IDX] - strlen(NI_STR);		//NI 의 길이 차 구함
		mvprintw(COLUMN_ROW, startX[NI_IDX] + gap, "%s", NI_STR);	//우측 정렬
	}

	//VIRT 출력
	if(startCol <= VIRT_IDX && VIRT_IDX < endCol){
		gap = columnWidth[VIRT_IDX] - strlen(VIRT_STR);	//VSZ의 길이 차 구함
		mvprintw(COLUMN_ROW, startX[VIRT_IDX] + gap, "%s", VIRT_STR);	//우측 정렬
	}

	//RES 출력
	if(startCol <= RES_IDX && RES_IDX < endCol){
		gap = columnWidth[RES_IDX] - strlen(RES_STR);	//RSS의 길이 차 구함
		mvprintw(COLUMN_ROW, startX[RES_IDX] + gap, "%s", RES_STR);	//우측 정렬
	}

	//SHR 출력
	if(startCol <= SHR_IDX && SHR_IDX < endCol){
		gap = columnWidth[SHR_IDX] - strlen(SHR_STR);	//SHR의 길이 차 구함
		mvprintw(COLUMN_ROW, startX[SHR_IDX] + gap, "%s", SHR_STR);	//우측 정렬
	}

	//S 출력
	if(startCol <= S_IDX && S_IDX < endCol){
		mvprintw(COLUMN_ROW, startX[S_IDX], "%s", S_STR);	//우측 정렬
	}

	//%CPU 출력
	if(startCol <= CPU_IDX && CPU_IDX < endCol){
		gap = columnWidth[CPU_IDX] - strlen(CPU_STR);	//CPU의 길이 차 구함
		mvprintw(COLUMN_ROW, startX[CPU_IDX] + gap, "%s", CPU_STR);	//우측 정렬
	}

	//%MEM 출력
	if(startCol <= MEM_IDX && MEM_IDX < endCol){
		gap = columnWidth[MEM_IDX] - strlen(MEM_STR);	//MEM의 길이 차 구함
		mvprintw(COLUMN_ROW, startX[MEM_IDX] + gap, "%s", MEM_STR);	//우측 정렬
	}

	//TIME+ 출력
	if(startCol <= TIME_P_IDX && TIME_P_IDX < endCol){
		gap = columnWidth[TIME_P_IDX] - strlen(TIME_P_STR);	//TIME의 길이 차 구함
		mvprintw(COLUMN_ROW, startX[TIME_P_IDX] + gap + 1, "%s", TIME_P_STR);	//우측 정렬
	}

	//COMMAND 출력
	mvprintw(COLUMN_ROW, startX[COMMAND_IDX], "%s", COMMAND_STR);	//좌측 정렬

	attroff(A_REVERSE);

	/*****		column 출력 종료	*****/


	/*****		process 출력 시작	*****/

	char token[TOKEN_LEN];
	memset(token, '\0', TOKEN_LEN);

	for(int i = row; i < procCnt; i++){

		// PID 출력
		if(startCol <= PID_IDX && PID_IDX < endCol){
			memset(token, '\0', TOKEN_LEN);
			sprintf(token, "%lu", sorted[i]->pid);
			gap = columnWidth[PID_IDX] - strlen(token);	// PID의 길이 차 구함
			mvprintw(COLUMN_ROW+1+i-row, startX[PID_IDX]+gap, "%s", token);	// 우측 정렬
		}

		//USER 출력
		if(startCol <= USER_IDX && USER_IDX < endCol){
			gap = columnWidth[USER_IDX] - strlen(sorted[i]->user);	//TIME의 길이 차 구함
			mvprintw(COLUMN_ROW+1+i-row, startX[USER_IDX], "%s", sorted[i]->user);	//좌측 정렬
		}

		//PR 출력
		if(startCol <= PR_IDX && PR_IDX < endCol){
			memset(token, '\0', TOKEN_LEN);
			sprintf(token, "%d", sorted[i]->priority);
			gap = columnWidth[PR_IDX] - strlen(token);	//PR의 길이 차 구함
			mvprintw(COLUMN_ROW+1+i-row, startX[PR_IDX]+gap, "%s", token);	//우측 정렬
		}

		//NI 출력
		if(startCol <= NI_IDX && NI_IDX < endCol){
			memset(token, '\0', TOKEN_LEN);
			sprintf(token, "%d", sorted[i]->nice);
			gap = columnWidth[NI_IDX] - strlen(token);	//NI의 길이 차 구함
			mvprintw(COLUMN_ROW+1+i-row, startX[NI_IDX]+gap, "%s", token);	//우측 정렬
		}

		//VIRT 출력
		if(startCol <= VIRT_IDX && VIRT_IDX < endCol){
			memset(token, '\0', TOKEN_LEN);
			sprintf(token, "%lu", sorted[i]->vsz);
			gap = columnWidth[VIRT_IDX] - strlen(token);	//VIRT의 길이 차 구함
			mvprintw(COLUMN_ROW+1+i-row, startX[VIRT_IDX]+gap, "%s", token);	//우측 정렬
		}

		//RES 출력
		if(startCol <= RES_IDX && RES_IDX < endCol){
			memset(token, '\0', TOKEN_LEN);
			sprintf(token, "%lu", sorted[i]->rss);
			gap = columnWidth[RES_IDX] - strlen(token);	//RES의 길이 차 구함
			mvprintw(COLUMN_ROW+1+i-row, startX[RES_IDX]+gap, "%s", token);	//우측 정렬
		}

		//SHR 출력
		if(startCol <= SHR_IDX && SHR_IDX < endCol){
			memset(token, '\0', TOKEN_LEN);
			sprintf(token, "%lu", sorted[i]->shr);
			gap = columnWidth[SHR_IDX] - strlen(token);	//SHR의 길이 차 구함
			mvprintw(COLUMN_ROW+1+i-row, startX[SHR_IDX]+gap, "%s", token);	//우측 정렬
		}

		//S 출력
		if(startCol <= S_IDX && S_IDX < endCol){
			gap = columnWidth[S_IDX] - strlen(sorted[i]->stat);	//S의 길이 차 구함
			mvprintw(COLUMN_ROW+1+i-row, startX[S_IDX], "%s", sorted[i]->stat);	//좌측 정렬
		}

		//%CPU 출력
		if(startCol <= CPU_IDX && CPU_IDX < endCol){
			memset(token, '\0', TOKEN_LEN);
			sprintf(token, "%3.1Lf", sorted[i]->cpu);
			gap = columnWidth[CPU_IDX] - strlen(token);	//CPU의 길이 차 구함
			mvprintw(COLUMN_ROW+1+i-row, startX[CPU_IDX]+gap, "%s", token);	//우측 정렬
		}

		//%MEM 출력
		if(startCol <= MEM_IDX && MEM_IDX < endCol){
			memset(token, '\0', TOKEN_LEN);
			sprintf(token, "%3.1Lf", sorted[i]->mem);
			gap = columnWidth[MEM_IDX] - strlen(token);	//MEM의 길이 차 구함
			mvprintw(COLUMN_ROW+1+i-row, startX[MEM_IDX]+gap, "%s", token);	//우측 정렬
		}

		//TIME+ 출력
		if(startCol <= TIME_P_IDX && TIME_P_IDX < endCol){
			gap = columnWidth[TIME_P_IDX] - strlen(sorted[i]->time);	//TIME의 길이 차 구함
			mvprintw(COLUMN_ROW+1+i-row, startX[TIME_P_IDX] + gap + 1, "%s", sorted[i]->time);	//우측 정렬
		}

		//COMMAND 출력
		int tap = col - COMMAND_IDX;
		if((col == COMMAND_IDX) && (strlen(sorted[i]->command) < tap*TAB_WIDTH))		//COMMAND를 출력할 수 없는 경우
			continue;
		if(col < COLUMN_CNT - 1)	//다른 column도 함께 출력하는 경우
			tap = 0;
		sorted[i]->cmd[maxCmd] = '\0';
		mvprintw(COLUMN_ROW+1+i-row, startX[COMMAND_IDX], "%s", sorted[i]->cmd + tap*TAB_WIDTH);	//좌측 정렬

	}
	/*****		process 출력 종료	*****/

	return;
}

// procList를 모드에 따라 sorting해 sorted 배열을 완성하는 함수
void SortProc(void) {
	for (int i = 0; i < procCnt; i++)		//포인터 복사
		sorted[i] = procList + i;
	for (int i = procCnt - 1; i > 0; i--) {
		for (int j = 0; j < i; j++) {
			if (isGreater(sorted[j], sorted[j+1])) {
				myProc *tmp = sorted[j];
				sorted[j] = sorted[j+1];
				sorted[j+1] = tmp;
			}
		}
	}
	return;
}

// 모드에 따라 비교해서 정렬 기준 return 함수
bool isGreater(myProc *a, myProc *b) {
	bool value;

	if (sortMode == CPU_MODE) {
		if (a->cpu < b->cpu)
			value = true;
		else if (a->cpu > b->cpu)
			value = false;
		else {
			if (a->pid > b->pid)	
				return true;
			else return false;
		}
	}
	else if (sortMode == MEM_MODE) {
		if (a->mem < b->mem)
			value = true;
		else if (a->mem > b->mem)
			value = false;
		else {
			if (a->rss < b->rss)
				value = true;
			else if (a->rss > b->rss)
				value = false;
			else {
				if (a->pid > b->pid)	
					return true;
				else return false;
			}
		}
	}
	else if (sortMode == PID_MODE) {
		if (a->pid > b->pid)
			value = true;
		else value = false;
	}
	else if (sortMode == TIME_MODE) {
		if (a->runtime < b->runtime)
			value = true;
		else if (a->runtime > b->runtime)
			value = false;
		else {
			if (a->pid > b->pid)	
				return true;
			else return false;			
		}
	}

	if (!reverseMode)
		return value;
	else
		return !value;
}

// 화면 출력을 모두 초기화하는 함수
void ClearSCR(void) {
	for (int i = 0; i < LINES; i++)
		for (int j = 0; j < COLS; j++)
			addch(' ');
	return;
}

// src를 소숫점 아래 rdx+1자리에서 반올림하는 함수
long double RoundDouble(long double src, int rdx) {
	if (!rdx)
		return (long double)((unsigned long long)src);

	long double tmp = src;
	for (int i = 0; i <= rdx; i++)	// 소숫점 아래 rdx+1번째 값 구하기
		tmp *= 10;
	int val = (unsigned long long)tmp % 10;	// 소숫점 아래 rdx+1번째 값

	tmp /= 10;			// 소숫점 아래 rdx번째까지만 소숫점 위로 표현

	tmp = (long double)((unsigned long long )tmp);	// rdx 밑의 자릿수 값 버리기

	if (val >= 5)		// 반올림 o
		tmp += 1;

	for (int i = 0; i < rdx; i++)	// 원상 복구
		tmp /= 10;

	return tmp;
}

// path에 대한 tty 얻는 함수
void GetTTY(char path[PATH_SIZE], char tty[TTY_LEN]) {
	char fdZeroPath[PATH_SIZE];			// 0번 fd에 대한 절대 경로
	memset(tty, '\0', TTY_LEN);
	memset(fdZeroPath, '\0', TTY_LEN);
	strcpy(fdZeroPath, path);
	strcat(fdZeroPath, FD__ZERO);

	if (access(fdZeroPath, F_OK) < 0) {	// fd 0이 없을 경우
		char statPath[PATH_SIZE];		// /proc/pid/stat에 대한 절대 경로
		memset(statPath, '\0', PATH_SIZE);
		strcpy(statPath, path);
		strcat(statPath, STAT);

		FILE *statFp;
		if ((statFp = fopen(statPath, "r")) == NULL) {	// /proc/pid/stat open
			fprintf(stderr, "fopen error %s %s\n", strerror(errno), statPath);
			sleep(1);
			return;
		}

		char buf[BUFFER_SIZE];
		for (int i = 0; i <= STAT_TTY_NR_IDX; i++) {	// 7행까지 read해 TTY_NR 획득
			memset(buf, '\0', BUFFER_SIZE);
			fscanf(statFp, "%s", buf);
		}
		fclose(statFp);

		int ttyNr = atoi(buf);		// ttyNr 정수 값으로 저장

		DIR *dp;
		struct dirent *dentry;
		if ((dp = opendir(DEV)) == NULL) {		// 터미널 찾기 위해 /dev 디렉터리 open
			fprintf(stderr, "opendir error for %s\n", DEV);
			exit(1);
		}
		char nowPath[PATH_SIZE];

		while ((dentry = readdir(dp)) != NULL) {// /dev 디렉터리 탐색
			memset(nowPath, '\0', PATH_SIZE);	// 현재 탐색 중인 파일 절대 경로
			strcpy(nowPath, DEV);
			strcat(nowPath, "/");
			strcat(nowPath, dentry->d_name);

			struct stat statbuf;
			if (stat(nowPath, &statbuf) < 0) {	// stat 획득
				fprintf(stderr, "stat error for %s\n", nowPath);
				exit(1);
			}
			if (!S_ISCHR(statbuf.st_mode))		// 문자 디바이스 파일이 아닌 경우 skip
				continue;
			else if (statbuf.st_rdev == ttyNr) {// 문자 디바이스 파일의 디바이스 ID가 ttyNr과 같은 경우
				strcpy(tty, dentry->d_name);	// tty에 현재 파일명 복사
				break;
			}
		}
		closedir(dp);

		if (!strlen(tty))					// /dev에서도 찾지 못한 경우
			strcpy(tty, "?");				// nonTerminal
	}
	else {
		char symLinkName[FNAME_LEN];
		memset(symLinkName, 0, FNAME_LEN);/**/
		if (readlink(fdZeroPath, symLinkName, FNAME_LEN) < 0) { // 심볼릭 링크를 읽음
			fprintf(stderr, "readlink error for %s\n", fdZeroPath);
			exit(1);
		}

		if (!strcmp(symLinkName, DEVNULL))		// symbolic link로 가리키는 파일이 /dev/null일 경우
			strcpy(tty, "?");					// nonTerminal
		else
			sscanf(symLinkName, "/dev/%s", tty);	// 그 외의 경우 tty 획득
	}
	return;
}

// /proc/uptime에서 OS 부팅 후 지난 시간 얻는 함수
unsigned long GetUpTime(void) {
	FILE *fp;
	char buf[BUFFER_SIZE];
	long double time;

	memset(buf, '\0', BUFFER_SIZE);

	if ((fp = fopen(UPTIME, "r")) == NULL){	// /proc/uptime open
		fprintf(stderr, "fopen error for %s\n", UPTIME);
		exit(1);
	}
	fgets(buf, BUFFER_SIZE, fp);
	sscanf(buf, "%Lf", &time);	// /proc/uptime의 첫번째 double 읽기
	fclose(fp);

	return (unsigned long)time;
}

// /proc/meminfo에서 전체 물리 메모리 크기 얻는 함수
unsigned long GetTotalMemory(void) {
	FILE *fp;
	char buf[BUFFER_SIZE];
	unsigned long total_memory;

	if ((fp = fopen(MEMINFO, "r")) == NULL){	// /proc/meminfo open
		fprintf(stderr, "fopen error for %s\n", MEMINFO);
		exit(1);
	}
	int i = 0;
	while (i < MEMINFO_MEM_TOTAL_ROW) {	// total_memory read
		memset(buf, '\0', BUFFER_SIZE);
		fgets(buf, BUFFER_SIZE, fp);
		i++;
	}
	char *ptr = buf;
	while (!isdigit(*ptr)) ptr++;
	sscanf(ptr, "%lu", &total_memory);	// /proc/meminfo의 1행에서 total_memory read
	fclose(fp);

	return total_memory;
}

// pid 디렉터리 내의 파일들을 이용해 myProc 완성하는 함수
void AddProcessList(char path[PATH_SIZE], unsigned long cpuTimeTable[PID_MAX]) {
	if (access(path, R_OK) < 0) {
		fprintf(stderr, "access error for %s\n", path);
		return;
	}
	myProc proc;
	EraseProcess(&proc);

	char statPath[PATH_SIZE];
	strcpy(statPath, path);
	strcat(statPath, STAT);

	if (access(statPath, R_OK) < 0) {
		fprintf(stderr, "access error for %s\n", statPath);
		return;
	}

	FILE *statFp;
	if ((statFp = fopen(statPath, "r")) == NULL) {
		fprintf(stderr, "fopen error %s %s\n", strerror(errno), statPath);
		sleep(1);
		return;
	}

	char statToken[MAX_TOKEN][TOKEN_LEN];
	memset(statToken, '\0', MAX_TOKEN * TOKEN_LEN);
	for (int i = 0; i < MAX_TOKEN; i++)
		fscanf(statFp, "%s", statToken[i]);
	fclose(statFp);

	proc.pid = (unsigned long)atoi(statToken[STAT_PID_IDX]);	// PID 획득
	proc.ppid = (unsigned long)atoi(statToken[STAT_PPID_IDX]);	// PPID 획득

	// USER명 획득
	struct stat statbuf;
	if (stat(statPath, &statbuf) < 0) {
		fprintf(stderr, "stat error for %s\n", statPath);
		return;
	}
	proc.uid = statbuf.st_uid;
	struct passwd *upasswd = getpwuid(statbuf.st_uid);

	char tmp[UNAME_LEN];
	strcpy(tmp, upasswd->pw_name);
	tmp[strlen(SYSTEMD)] = '\0';

	if (!strcmp(tmp, SYSTEMD)) {			// user명이 systemd-로 시작할 경우
		tmp[strlen(SYSTEMD)-1] = '+';		// systemd+를 user명으로 저장
		strcpy(proc.user, tmp);
	}
	else						// user명이 systemd-로 시작하지 않을 경우 그대로 저장
		strcpy(proc.user, upasswd->pw_name);

	// %CPU 계산
	unsigned long utime = (unsigned long)atoi(statToken[STAT_UTIME_IDX]);
	unsigned long stime = (unsigned long)atoi(statToken[STAT_STIME_IDX]);
	unsigned long startTime = (unsigned long)atoi(statToken[STAT_START_TIME_IDX]);
	unsigned long totalTime = utime + stime;
	long double cpu = 0.0;
	bool update = false;

    if (cpuTimeTable != NULL) {			        
		if (cpuTimeTable[proc.pid] != 0) {	// 이전에 실행된 내역이 있을 경우 process 시작 시각
			update = true;					// 이후의 값이 아닌 이전 실행 이후의 값 불러옴
			cpu = (totalTime-cpuTimeTable[proc.pid]) / (long double)(now - before) / hertz * 100;	// %cpu 갱신
		}
		cpuTimeTable[proc.pid] = totalTime;	// 현재까지의 process 사용 내역 저장
	}

	if (!update)
		cpu = ((totalTime) / hertz) / (long double)(uptime - (startTime / hertz) ) * 100;
	if (isnan(cpu) || isinf(cpu) || cpu > 100 || cpu < 0)	// error 처리
		proc.cpu = 0.0;
	else
		proc.cpu = RoundDouble(cpu, 1);	// 소숫점 아래 2자리에서 반올림

	char statusPath[PATH_SIZE];			// /proc/pid/status 경로 획득
	memset(statusPath, '\0', PATH_SIZE);
	strcpy(statusPath, path);
	strcat(statusPath, STATUS);

	FILE *statusFp;						// memory read
	unsigned long vmLck = 0;
	if ((statusFp = fopen(statusPath, "r")) == NULL) {	// /proc/pid/status open
		fprintf(stderr, "fopen error for %s\n", statusPath);
		return;
	}

	char buf[BUFFER_SIZE];
	int cnt = 0;
	while (cnt < STATUS_VSZ_ROW) {				// 18행까지 read
		memset(buf, '\0', BUFFER_SIZE);
		fgets(buf, BUFFER_SIZE, statusFp);
		cnt++;
	}

	char *ptr = buf;
	for (int i = 0; i < strlen(buf); i++)
		if (isdigit(buf[i])) {
			ptr = buf + i;
			break;
		}

	buf[strlen(VMSIZE)] = '\0';
	if (strcmp(buf, VMSIZE)) {		// /proc/pid/status에 메모리 정보 없을 경우
		proc.vsz = 0;
		proc.rss = 0;
		proc.shr = 0;
		proc.mem = 0.0;
	}
	else {							// /proc/pid/status에 메모리 정보 있을 경우
		sscanf(ptr, "%lu", &proc.vsz);				// 18행에서 vsz 획득

		while (cnt < STATUS_VMLCK_ROW) {			// 19행까지 read
			memset(buf, '\0', BUFFER_SIZE);
			fgets(buf, BUFFER_SIZE, statusFp);
			cnt++;
		}
		ptr = buf;
		for (int i = 0; i < strlen(buf); i++)
			if (isdigit(buf[i])) {
				ptr = buf + i;
				break;
			}
		vmLck = 0;									// state에서 사용할 vmLck read
		sscanf(ptr, "%lu", &vmLck);					// 19행에서 vmLck 획득

		while (cnt < STATUS_RSS_ROW) {				// 22행까지 read
			memset(buf, '\0', BUFFER_SIZE);
			fgets(buf, BUFFER_SIZE, statusFp);
			cnt++;
		}
		ptr = buf;
		for (int i = 0; i < strlen(buf); i++)
			if (isdigit(buf[i])) {
				ptr = buf + i;
				break;
			}
		sscanf(ptr, "%lu", &proc.rss);				// 22행에서 rss 획득

		while (cnt < STATUS_SHR_ROW) {				// 24행까지 read
			memset(buf, '\0', BUFFER_SIZE);
			fgets(buf, BUFFER_SIZE, statusFp);
			cnt++;
		}
		ptr = buf;
		for (int i = 0; i < strlen(buf); i++)
			if (isdigit(buf[i])) {
				ptr = buf + i;
				break;
			}
		sscanf(ptr, "%lu", &proc.shr);

		unsigned long shr_add;
		memset(buf, '\0', BUFFER_SIZE);
		fgets(buf, BUFFER_SIZE, statusFp);
		ptr = buf;
		for (int i = 0; i < strlen(buf); i++)
			if (isdigit(buf[i])) {
				ptr = buf + i;
				break;
			}
		sscanf(ptr, "%lu", &shr_add);	
		proc.shr += shr_add;						// 24행에서 shr 획득

		long double mem = (long double)proc.rss / total_memory * 100;	// %MEM 계산
		if (isnan(mem) || isinf(mem) || mem > 100 || mem < 0)			// error 처리
			proc.mem = 0.0;
		else
			proc.mem = RoundDouble(mem, 1);	// 소숫점 아래 2자리에서 반올림
	}
	fclose(statusFp);

	proc.priority = atoi(statToken[STAT_PRIORITY_IDX]);	// priority 획득
	proc.nice = atoi(statToken[STAT_NICE_IDX]);			// nice 획득

	// STATE 획득
	strcpy(proc.stat, statToken[STAT_STATE_IDX]);		// 기본 state 획득

	// START 획득
	unsigned long start = time(NULL) - uptime + (startTime / hertz);
	struct tm *tmStart = localtime(&start);
	if (time(NULL) - start < 24 * 60 * 60) {
		strftime(proc.start, TIME_LEN, "%H:%M", tmStart);
	}
	else if (time(NULL) - start < 7 * 24 * 60 * 60) {
		strftime(proc.start, TIME_LEN, "%b %d", tmStart);
	}
	else {
		strftime(proc.start, TIME_LEN, "%y", tmStart);
	}

	// TIME 획득
	unsigned long cpuTime = totalTime / hertz;
	struct tm *tmCpuTime = gmtime(&cpuTime);
	sprintf(proc.time, "%1d:%02d.%02lu", tmCpuTime->tm_min,	tmCpuTime->tm_sec,totalTime % 100);	
	proc.runtime = totalTime;

	// CMD 획득
	sscanf(statToken[STAT_CMD_IDX], "(%s", proc.cmd);	
	proc.cmd[strlen(proc.cmd)-1] = '\0';	// 마지막 ')' 제거

	// COMMAND 획득
	char cmdLinePath[PATH_SIZE];
	memset(cmdLinePath, '\0', PATH_SIZE);
	strcpy(cmdLinePath, path);
	strcat(cmdLinePath, CMDLINE);

	FILE *cmdLineFp;

	if ((cmdLineFp = fopen(cmdLinePath, "r")) == NULL) {
		fprintf(stderr, "fopen error for %s\n", cmdLinePath);
		return;
	}

	while (true) {
		char c[2] = {'\0', '\0'};
		fread(&c[0], 1, 1, cmdLineFp);
		if (c[0] == '\0') {					// '\0'여도 한 번 더 읽고 판단
			fread(&c[0], 1, 1, cmdLineFp);
			if (c[0] == '\0')
				break;
			else {
				strcat(proc.command, " ");
			}
		}
		strcat(proc.command, c);
	}

	if (!strlen(proc.command))						// cmdline에서 읽은 문자열 길이 0일 경우
		sprintf(proc.command, "[%s]", proc.cmd);	// [cmd]로 채워기

	fclose(cmdLineFp);

	procList[procCnt].pid = proc.pid;
	procList[procCnt].uid = proc.uid;
	procList[procCnt].ppid = proc.ppid;
	procList[procCnt].runtime = proc.runtime;
	strcpy(procList[procCnt].user, proc.user);
	procList[procCnt].cpu = proc.cpu;
	procList[procCnt].mem = proc.mem;
	procList[procCnt].vsz = proc.vsz;
	procList[procCnt].rss = proc.rss;
	procList[procCnt].shr = proc.shr;
	procList[procCnt].priority = proc.priority;
	procList[procCnt].nice = proc.nice;
	strcpy(procList[procCnt].tty, proc.tty);
	strcpy(procList[procCnt].stat, proc.stat);
	strcpy(procList[procCnt].start, proc.start);
	strcpy(procList[procCnt].time, proc.time);
	strcpy(procList[procCnt].cmd, proc.cmd);
	strcpy(procList[procCnt].command, proc.command);
	procCnt++;

	return;
}

// /proc 디렉터리 탐색하는 함수
void SearchProcess(unsigned long cpuTimeTable[PID_MAX]) {
	uptime = GetUpTime();
	DIR *dirp;
	if ((dirp = opendir(PROC)) == NULL) {				// /proc 디렉터리 open
		fprintf(stderr, "dirp error for %s\n", PROC);
		exit(1);
	}

	struct dirent *dentry;
	while ((dentry = readdir(dirp)) != NULL) {			// /proc 디렉터리 내 하위 파일들 탐색 시작
		char path[PATH_SIZE];							// 디렉터리의 절대 경로 저장
		memset(path, '\0', PATH_SIZE);
		strcpy(path, PROC);
		strcat(path, "/");
		strcat(path, dentry->d_name);

		struct stat statbuf;
		if (stat(path, &statbuf) < 0) {					// 디렉터리의 stat 획득
			fprintf(stderr, "stat error for %s\n", path);
			exit(1);
		}

		if (!S_ISDIR(statbuf.st_mode))					// 디렉터리가 아닐 경우 skip
			continue;

		int len = strlen(dentry->d_name);
		bool isPid = true;
		for (int i = 0; i < len; i++) {					// 디렉터리가 PID인지 찾기
			if (!isdigit(dentry->d_name[i])) {			// 디렉터리명 중 숫자 아닌 문자가 있을 경우 PID가 아님
				isPid = false;
				break;
			}
		}
		if (!isPid)										// PID 디렉터리가 아닌 경우 skip
			continue;

		AddProcessList(path, cpuTimeTable);			// PID 디렉터리인 경우 procList에 추가
	}
	closedir(dirp);
	return;
}

// proc의 내용을 지우는 함수
void EraseProcess(myProc *proc) {
	proc->pid = 0;
	proc->uid = 0;
    proc->ppid = 0;
	memset(proc->user, '\0', UNAME_LEN);
	proc->cpu = 0.0;
	proc->mem = 0.0;
	proc->vsz = 0;
	proc->rss = 0;
	proc->shr = 0;
	proc->priority = 0;
	proc->nice = 0;
	memset(proc->tty, '\0', TTY_LEN);
	memset(proc->stat, '\0', STAT_LEN);
	memset(proc->start, '\0', TIME_LEN);
	memset(proc->time, '\0', TIME_LEN);
	memset(proc->cmd, '\0', CMD_LEN);
	memset(proc->command, '\0', CMD_LEN);
	return;
}

// procList 내용 지우는 함수
void EraseProcessList(void) {
	for(int i = 0; i < procCnt; i++)
		EraseProcess(procList + i);
	procCnt = 0;
	return;
}

void KillProcess(void) {
	if (killMode) {
		int input;
		mvprintw(CMD_ROW, 0, "PID to signal/kill : ");
		curs_set(1);
		echo();
		mvscanw(CMD_ROW, 21, "%d", &input);
		kill(input, SIGTERM);
		killMode = false;
		curs_set(0);
		noecho();
	}
}