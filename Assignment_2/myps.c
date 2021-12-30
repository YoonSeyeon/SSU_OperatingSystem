#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
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

#define BUFFER_SIZE 1024			// 버퍼 최대 길이
#define PATH_SIZE 1024				// 경로 이름 최대 길이
#define TOKEN_LEN 32				// proc/pid/stat에서 얻는 값들의 최대 길이
#define FNAME_LEN 128				// 파일 이름 최대 길이

#define MAX_TOKEN 22				// /proc/pid/stat에서 읽어들일 token 갯수

#define UNAME_LEN 32				// user 이름의 길이
#define TTY_LEN 32					// 터미널 경로 이름의 길이
#define STAT_LEN 8
#define TIME_LEN 16					// 시간을 표시할 때의 길이
#define CMD_LEN 1024

#define PID_MAX 32768				// pid 최대 갯수
#define PROCESS_MAX 4096			// process 최대 갯수

#define SYSTEMD "systemd-"			// system USER명의 앞부분
#define DEVNULL "/dev/null"			// 터미널 없을 시 가리키는 /dev/null 절대 경로
#define PTS "pts/"					// 터미널 문자열의 앞부분

#define PROC "/proc"				// /proc 절대 경로
#define LOADAVG "/proc/loadavg"		// /proc/loadavg 절대 경로
#define UPTIME "/proc/uptime"		// /proc/uptime 절대 경로
#define CPUSTAT "/proc/stat"		// /proc/stat 절대 경로
#define MEMINFO "/proc/meminfo"		// /proc/meminfo 절대 경로
#define DEV "/dev"					// /dev 절대 경로

#define FD__ZERO "/fd/0"			// /proc/pid 에서의 0번째 fd 경로
#define STAT "/stat"				// /proc/pid 에서의 stat 경로
#define STATUS "/status"			// /proc/pid 에서의 status 경로
#define CMDLINE "/cmdline"			// /proc/pid 에서의 cmdline 경로
#define VMSIZE "VmSize"				// /proc/pid/status 에서 VmSize있는지 확인하기 위한 문자열

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
#define STATUS_HWM_ROW 21
#define STATUS_RSS_ROW 22
#define STATUS_SHR_ROW 24

// /proc/stat 에서의 idx
#define CPU_STAT_US_IDX 1
#define CPU_STAT_SY_IDX 2
#define CPU_STAT_NI_IDX 3
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

#define COLUMN_CNT 16	// 출력할 column 최대 갯수

// 출력 시 사용할 columnWidth 배열에서의 index
#define USER_IDX 0
#define UID_IDX 1
#define PID_IDX 2
#define PPID_IDX 3
#define C_IDX 4
#define STIME_IDX 5
#define CPU_IDX 6
#define MEM_IDX 7
#define VSZ_IDX 8
#define RSS_IDX 9
#define TTY_IDX 10
#define STAT_IDX 11
#define START_IDX 12
#define TIME_IDX 13
#define CMD_IDX 14
#define COMMAND_IDX 15

// process를 추상화 한 myProc 구조체
typedef struct{
	unsigned long pid;			// 프로세스 id
	unsigned long uid;			// USER 구하기 위한 uid
	unsigned long ppid;			// 프로세스의 부모 id
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

void PrintPS(void);
long double RoundDouble(long double src, int rdx);
unsigned long kib_to_kb(unsigned long kib);
void GetTTY(char path[PATH_SIZE], char tty[TTY_LEN]);
unsigned long GetUpTime(void);
unsigned long GetTotalMemory(void);
void AddProcessList(char path[PATH_SIZE], unsigned long cpuTimeTable[]);
void SearchProcess(unsigned long cpuTimeTable[]);
void EraseProcess(myProc *proc);
void EraseProcessList(void);
void ErrorFunc(void);

myProc procList[PROCESS_MAX];	// 완성한 myProc의 포인터 저장 배열
int procCnt = 0;				// 현재까지 완성한 myProc 갯수
int pidListIdx = 0;				// pidList index

unsigned long uptime;			// os 부팅 후 지난 시간
unsigned long beforeUptime;		// 이전 실행 시의 os 부팅 후 지난 시각
unsigned long total_memory;		// 전체 물리 메모리 크기
unsigned long hertz;	 		// os의 hertz값 얻기(초당 context switching 횟수)
unsigned long pidList[PID_MAX];	// p Option 사용 시 pid 저장하는 배열

pid_t myPid;					// 자기 자신의 pid
uid_t myUid;					// 자기 자신의 uid
char myPath[PATH_SIZE];			// 자기 자신의 path
char myTTY[TTY_LEN];			// 자기 자신의 tty

bool aOption = false;			// a Option	stat, command 활성화
bool eOption = false;			// e Option 커널 프로세스를 제외한 모든 프로세스를 보여줌
bool fOption = false;			// f Option ppid, stime, cpu, command 활성화
bool hOption = false;			// h Option 헤더라인을 보여주지 않음
bool pOption = false;			// p Option pid를 선택하여 지정한 pid 내용만 출력
bool uOption = false;			// u Option	user, cpu, mem, vsz, rss, start, command 활성화
bool xOption = false;			// x Option	stat, command 활성화

int termWidth;					// 현재 터미널 너비

int main(int argc, char *argv[]) {
	total_memory = GetTotalMemory();				// 전체 물리 메모리 크기
	hertz = (unsigned long)sysconf(_SC_CLK_TCK);	// 초당 context switching 횟수

	myPid = getpid();		// 자기 자신의 pid 얻어오기
	myUid = getuid();		// 자기 자신의 uid 얻어오기

	char pidPath[FNAME_LEN];
	memset(pidPath, '\0', FNAME_LEN);
	sprintf(pidPath, "/%d", myPid); // 자신의 pid를 경로로 만들어 저장

	strcpy(myPath, PROC);			// 자기 자신의 /proc 경로 획득
	strcat(myPath, pidPath);

	GetTTY(myPath, myTTY);			// 자기 자신의 tty 획득
	for (int i = strlen(PTS); i < strlen(myTTY); i++) {
		if (!isdigit(myTTY[i])) {
			myTTY[i] = '\0';
			break;
		}
	}

	initscr();				// 출력 화면 지워지고 초기화, ncurses 자료 구조들을 초기화
	termWidth = COLS;		// term 너비 획득
	endwin();				// 출력 윈도우 종료

	for (int i = 1; i < argc; i++) {	// Option 구현 부분
		if (pOption) {
			for (int j = 0; j < strlen(argv[i]); j++) {
				if (!isdigit(argv[i][j]))
					ErrorFunc();
			}
			pidList[pidListIdx] = atoi(argv[i]);
			pidListIdx++;
		}

		for (int j = 0; j < strlen(argv[i]); j++) {
			switch (argv[i][j]) {
				case 'a':
					aOption = true;
					break;
				case 'e':
					eOption = true;
					aOption = true;
					xOption = true;
					break;
				case 'f':
					fOption = true;
					break;
				case 'h':
					hOption = true;
					break;
				case 'p':
					pOption = true;
					aOption = true;
					xOption = true;
					break;
				case 'u':
					uOption = true;
					break;
				case 'x':
					xOption = true;
					break;
			}
		}
	}

	SearchProcess(NULL);
	PrintPS();

	return 0;
}

// 실제 화면에 출력하는 함수
void PrintPS(void) {
	int columnWidth[COLUMN_CNT] = {				// column의 x축 길이 저장하는 배열
		strlen(USER_STR), strlen(UID_STR), strlen(PID_STR), strlen(PPID_STR),
		strlen(C_STR), strlen(STIME_STR), strlen(CPU_STR), strlen(MEM_STR),
		strlen(VSZ_STR), strlen(RSS_STR), strlen(TTY_STR), strlen(STAT_STR),
		strlen(START_STR), strlen(TIME_STR), strlen(CMD_STR), strlen(COMMAND_STR)
	};

	char buf[BUFFER_SIZE];

	for (int i = 0; i < procCnt; i++)			// USER 최대 길이 저장
		if (columnWidth[USER_IDX] < strlen(procList[i].user)) {
			columnWidth[USER_IDX] = strlen(procList[i].user);
		}

	for (int i = 0; i < procCnt; i++)			// UID 최대 길이 저장
		if (columnWidth[UID_IDX] < strlen(procList[i].user)) {
			columnWidth[UID_IDX] = strlen(procList[i].user);
		}

	for (int i = 0; i < procCnt; i++) {			// PID 최대 길이 저장
		sprintf(buf, "%lu", procList[i].pid);
		if(columnWidth[PID_IDX] < strlen(buf))
			columnWidth[PID_IDX] = strlen(buf);
	}

	for (int i = 0; i < procCnt; i++) {			// PPID 최대 길이 저장
		sprintf(buf, "%lu", procList[i].ppid);
		if(columnWidth[PPID_IDX] < strlen(buf))
			columnWidth[PPID_IDX] = strlen(buf);
	}

	for (int i = 0; i < procCnt; i++) {			// C 최대 길이 저장
		sprintf(buf, "%1d", (int)procList[i].cpu);
		if(columnWidth[C_IDX] < strlen(buf))
			columnWidth[C_IDX] = strlen(buf);
	}

	for (int i = 0; i < procCnt; i++) {			// STIME 최대 길이 저장
		if(columnWidth[STIME_IDX] < strlen(procList[i].start))
			columnWidth[STIME_IDX] = strlen(procList[i].start);
	}

	for (int i = 0; i < procCnt; i++) {			// %CPU 최대 길이 저장
		sprintf(buf, "%3.1Lf", procList[i].cpu);
		if(columnWidth[CPU_IDX] < strlen(buf))
			columnWidth[CPU_IDX] = strlen(buf);
	}

	for (int i = 0; i < procCnt; i++) {			// %MEM 최대 길이 저장
		sprintf(buf, "%3.1Lf", procList[i].mem);
		if(columnWidth[MEM_IDX] < strlen(buf))
			columnWidth[MEM_IDX] = strlen(buf);
	}

	for (int i = 0; i < procCnt; i++) {			// VSZ 최대 길이 저장
		sprintf(buf, "%lu", procList[i].vsz);
		if(columnWidth[VSZ_IDX] < strlen(buf))
			columnWidth[VSZ_IDX] = strlen(buf);
	}

	for (int i = 0; i < procCnt; i++) {			// RSS 최대 길이 저장
		sprintf(buf, "%lu", procList[i].rss);
		if(columnWidth[RSS_IDX] < strlen(buf))
			columnWidth[RSS_IDX] = strlen(buf);
	}

	for (int i = 0; i < procCnt; i++) {			// TTY 최대 길이 저장
		if(columnWidth[TTY_IDX] < strlen(procList[i].tty))
			columnWidth[TTY_IDX] = strlen(procList[i].tty);
	}

	for (int i = 0; i < procCnt; i++) {			// STAT 최대 길이 저장
		if(columnWidth[STAT_IDX] < strlen(procList[i].stat))
			columnWidth[STAT_IDX] = strlen(procList[i].stat);
	}

	for (int i = 0; i < procCnt; i++) {			// START 최대 길이 저장
		if(columnWidth[START_IDX] < strlen(procList[i].start))
			columnWidth[START_IDX] = strlen(procList[i].start);
	}

	for (int i = 0; i < procCnt; i++) {			// TIME 최대 길이 저장
		if(columnWidth[TIME_IDX] < strlen(procList[i].time))
			columnWidth[TIME_IDX] = strlen(procList[i].time);
	}

	for (int i = 0; i < procCnt; i++) {			// CMD 최대 길이 저장
		if(columnWidth[CMD_IDX] < strlen(procList[i].cmd))
			columnWidth[CMD_IDX] = strlen(procList[i].cmd);
	}

	for (int i = 0; i < procCnt; i++) {			// COMMAND 최대 길이 저장
		if(columnWidth[COMMAND_IDX] < strlen(procList[i].command))
			columnWidth[COMMAND_IDX] = strlen(procList[i].command);
	}

	// columnWidth 완성
	if (fOption) {
		columnWidth[USER_IDX] = -1;		// USER 출력 X
		columnWidth[MEM_IDX] = -1;		// MEM 출력 X
		columnWidth[VSZ_IDX] = -1;		// VSZ 출력 X
		columnWidth[RSS_IDX] = -1;		// RSS 출력 X
		columnWidth[STAT_IDX] = -1;		// STAT 출력 X
		columnWidth[COMMAND_IDX] = -1;	// COMMAND 출력 X
	}
	else {
		columnWidth[PPID_IDX] = -1;		// UID 출력 X
		columnWidth[PPID_IDX] = -1;		// PPID 출력 X
		columnWidth[C_IDX] = -1;		// C 출력 X
		columnWidth[STIME_IDX] = -1;	// STIME 출력 X

		if (!aOption && !uOption && !xOption) {
			columnWidth[USER_IDX] = -1;		// USER 출력 X
			columnWidth[CPU_IDX] = -1;		// CPU 출력 X
			columnWidth[MEM_IDX] = -1;		// MEM 출력 X
			columnWidth[VSZ_IDX] = -1;		// VSZ 출력 X
			columnWidth[RSS_IDX] = -1;		// RSS 출력 X
			columnWidth[STAT_IDX] = -1;		// STAT 출력 X
			columnWidth[START_IDX] = -1;	// START 출력 X
			columnWidth[COMMAND_IDX] = -1;	// COMMAND 출력 X
		}
		else if (!uOption) { 
			columnWidth[USER_IDX] = -1;		// USER 출력 X
			columnWidth[CPU_IDX] = -1;		// CPU 출력 X
			columnWidth[MEM_IDX] = -1;		// MEM 출력 X
			columnWidth[VSZ_IDX] = -1;		// VSZ 출력 X
			columnWidth[RSS_IDX] = -1;		// RSS 출력 X
			columnWidth[START_IDX] = -1;	// START 출력 X
			columnWidth[CMD_IDX] = -1;		// CMD 출력 X
		}
		else { 				
			columnWidth[CMD_IDX] = -1;		// CMD 출력 X
		}
	}

	/*****		column 출력 시작	*****/
	int gap = 0;
	memset(buf, '\0', BUFFER_SIZE);

	if (!hOption) {

		// USER 출력
		if (uOption) {
			gap = columnWidth[USER_IDX] - strlen(USER_STR);	// USER의 길이 차 구함
			strcat(buf, USER_STR);							// USER 좌측 정렬
			for (int i = 0; i < gap; i++)					
				strcat(buf, " ");
			strcat(buf, " ");
		}

		if (fOption) {
			gap = columnWidth[UID_IDX] - strlen(UID_STR);	// UID의 길이 차 구함
			strcat(buf, UID_STR);							// UID 좌측 정렬
			for (int i = 0; i < gap; i++)					
				strcat(buf, " ");
			strcat(buf, " ");
		}
		
		strcat(buf, "  ");

		// PID 출력
		gap = columnWidth[PID_IDX] - strlen(PID_STR);	// PID의 길이 차 구함
		for (int i = 0; i < gap + 1; i++)				// PID 우측 정렬
			strcat(buf, " ");
		strcat(buf, PID_STR);
		strcat(buf, " ");

		// PPID 출력
		if (fOption) {
			gap = columnWidth[PPID_IDX] - strlen(PPID_STR);	// PID의 길이 차 구함
			for (int i = 0; i < gap; i++)					// PID 우측 정렬
				strcat(buf, " ");
			strcat(buf, PPID_STR);
			strcat(buf, " ");
		}

		// C 출력
		if (fOption) {
			gap = columnWidth[C_IDX] - strlen(C_STR);		// CPU의 길이 차 구함
			for (int i = 0; i < gap; i++)					// CPU 우측 정렬
				strcat(buf, " ");
			strcat(buf, C_STR);
			strcat(buf, " ");
		}

		// STIME 출력
		if (fOption) {
			gap = columnWidth[STIME_IDX] - strlen(STIME_STR);	// START의 길이 차 구함
			strcat(buf, STIME_STR);
			for(int i = 0; i < gap; i++)						// START 좌측 정렬
				strcat(buf, " ");

			strcat(buf, " ");
		}

		// %CPU 출력
		if (uOption) {
			gap = columnWidth[CPU_IDX] - strlen(CPU_STR);	// CPU의 길이 차 구함
			for (int i = 0; i < gap; i++)					// CPU 우측 정렬
				strcat(buf, " ");
			strcat(buf, CPU_STR);
			strcat(buf, " ");
		}

		// %MEM 출력
		if (uOption) {
			gap = columnWidth[MEM_IDX] - strlen(MEM_STR);	// MEM의 길이 차 구함
			for (int i = 0; i < gap; i++)					// MEM 우측 정렬
				strcat(buf, " ");
			strcat(buf, MEM_STR);
			strcat(buf, " ");
		}

		// VSZ 출력
		if (uOption) {
			gap = columnWidth[VSZ_IDX] - strlen(VSZ_STR);	// VSZ의 길이 차 구함
			for (int i = 0; i < gap; i++)					// VSZ 우측 정렬
				strcat(buf, " ");
			strcat(buf, VSZ_STR);
			strcat(buf, " ");
		}

		// RSS 출력
		if (uOption) {
			gap = columnWidth[RSS_IDX] - strlen(RSS_STR);	// RSS의 길이 차 구함
			for (int i = 0; i < gap; i++)					// RSS 우측 정렬
				strcat(buf, " ");
			strcat(buf, RSS_STR);
			strcat(buf, " ");
		}

		// TTY 출력
		gap = columnWidth[TTY_IDX] - strlen(TTY_STR);	//TTY의 길이 차 구함
		strcat(buf, TTY_STR);
		for (int i = 0; i < gap; i++)				//TTY 좌측 정렬
			strcat(buf, " ");
		strcat(buf, " ");

		// STAT 출력
		if (!eOption && (aOption || uOption || xOption)) {
			gap = columnWidth[STAT_IDX] - strlen(STAT_STR);	// STAT의 길이 차 구함
			for (int i = 0; i < gap + 3; i++)				// STAT 좌측 정렬
				strcat(buf, " ");
			strcat(buf, STAT_STR);		
			strcat(buf, " ");
		}

		// START 출력
		if (uOption) {
			gap = columnWidth[START_IDX] - strlen(START_STR);	// START의 길이 차 구함
			strcat(buf, START_STR);
			for(int i = 0; i < gap; i++)						// START 좌측 정렬
				strcat(buf, " ");

			strcat(buf, " ");
		}

		// TIME 출력
		gap = columnWidth[TIME_IDX] - strlen(TIME_STR);	// TIME의 길이 차 구함
		for (int i = 0; i < gap + 2; i++)				// TIME 우측 정렬
			strcat(buf, " ");
		strcat(buf, TIME_STR);
		strcat(buf, " ");

		// COMMAND 또는 CMD 출력
		if (eOption || fOption) {
			strcat(buf, CMD_STR);					// CMD 바로 출력
		}
		else {
			if (aOption || uOption || xOption)
				strcat(buf, COMMAND_STR);
			else
				strcat(buf, CMD_STR);				// CMD 바로 출력
		}

		buf[COLS] = '\0';							// 터미널 너비만큼 잘라 출력
		printf("%s\n", buf);
	}

	/*****		column 출력 종료	*****/


	/*****		process 출력 시작	*****/

	char token[TOKEN_LEN];
	memset(token, '\0', TOKEN_LEN);

	for (int i = 0; i < procCnt; i++) {
		bool pCheck = false;	// pOption에 따라 건너뛰기 / 진행 결정
		memset(buf, '\0', BUFFER_SIZE);

		if (pOption) {			// pOption일 경우, 해당되지 않는 PID는 출력하지 않음
			for (int j = 0; j < pidListIdx; j++) {
				if (procList[i].pid == pidList[j]) {
					pCheck = true;
					break;
				}
			}

			if (!pCheck)
				continue;
		}

		// USER 출력
		if (uOption) {
			gap = columnWidth[USER_IDX] - strlen(procList[i].user);	// USER의 길이 차 구함
			strcat(buf, procList[i].user);
			for (int i = 0; i < gap; i++)			// USER 좌측 정렬
				strcat(buf, " ");
			strcat(buf, " ");
		}

		// UID 출력
		if (fOption) {
			gap = columnWidth[UID_IDX] - strlen(procList[i].user);	// UID의 길이 차 구함
			strcat(buf, procList[i].user);
			for (int i = 0; i < gap; i++)			// UID 좌측 정렬
				strcat(buf, " ");
			strcat(buf, " ");
		}

		strcat(buf, "  ");		

		// PID 출력
		memset(token, '\0', TOKEN_LEN);
		sprintf(token, "%lu", procList[i].pid);
		gap = columnWidth[PID_IDX] - strlen(token);		// PID의 길이 차 구함
		for (int i = 0; i < gap + 1; i++)				// PID 우측 정렬
			strcat(buf, " ");
		strcat(buf, token);
		strcat(buf, " ");

		// PPID 출력
		if (fOption) {
			memset(token, '\0', TOKEN_LEN);
			sprintf(token, "%lu", procList[i].ppid);
			gap = columnWidth[PPID_IDX] - strlen(token);	// PPID의 길이 차 구함
			for (int i = 0; i < gap; i++)					// PPID 우측 정렬
				strcat(buf, " ");
			strcat(buf, token);
			strcat(buf, " ");
		}

		// C 출력
		if (fOption) {
			memset(token, '\0', TOKEN_LEN);
			sprintf(token, "%1d", (int)procList[i].cpu);
			gap = columnWidth[C_IDX] - strlen(token);	// CPU의 길이 차 구함
			for (int i = 0; i < gap; i++)				// CPU 우측 정렬
				strcat(buf, " ");
			strcat(buf, token);
			strcat(buf, " ");
		}

		// STIME 출력
		if (fOption) {
			gap = columnWidth[STIME_IDX] - strlen(procList[i].start);	// STIME의 길이 차 구함
			strcat(buf, procList[i].start);
			for (int i = 0; i < gap; i++)			// STIME 좌측 정렬
				strcat(buf, " ");
			strcat(buf, " ");
		}

		// %CPU 출력
		if (uOption) {
			memset(token, '\0', TOKEN_LEN);
			sprintf(token, "%3.1Lf", procList[i].cpu);
			gap = columnWidth[CPU_IDX] - strlen(token);	// CPU의 길이 차 구함
			for (int i = 0; i < gap; i++)				// CPU 우측 정렬
				strcat(buf, " ");
			strcat(buf, token);
			strcat(buf, " ");
		}

		// %MEM 출력
		if (uOption) {
			memset(token, '\0', TOKEN_LEN);
			sprintf(token, "%3.1Lf", procList[i].mem);
			gap = columnWidth[MEM_IDX] - strlen(token);	// MEM의 길이 차 구함
			for (int i = 0; i < gap; i++)				// MEM 우측 정렬
				strcat(buf, " ");
			strcat(buf, token);
			strcat(buf, " ");
		}

		// VSZ 출력
		if (uOption) {
			memset(token, '\0', TOKEN_LEN);
			sprintf(token, "%lu", procList[i].vsz);
			gap = columnWidth[VSZ_IDX] - strlen(token);	// VSZ의 길이 차 구함
			for (int i = 0; i < gap; i++)				// VSZ 우측 정렬
				strcat(buf, " ");
			strcat(buf, token);
			strcat(buf, " ");
		}

		// RSS 출력
		if (uOption) {
			memset(token, '\0', TOKEN_LEN);
			sprintf(token, "%lu", procList[i].rss);
			gap = columnWidth[RSS_IDX] - strlen(token);	// RSS의 길이 차 구함
			for (int i = 0; i < gap; i++)				// RSS 우측 정렬
				strcat(buf, " ");
			strcat(buf, token);
			strcat(buf, " ");
		}

		// TTY 출력
		gap = columnWidth[TTY_IDX] - strlen(procList[i].tty);	// TTY의 길이 차 구함
		strcat(buf, procList[i].tty);
		for (int i = 0; i < gap; i++)							// TTY 좌측 정렬
			strcat(buf, " ");
		strcat(buf, " ");

		// STAT 출력
		if (!eOption && (aOption || uOption || xOption)) {
			gap = columnWidth[STAT_IDX] - strlen(procList[i].stat);	// STAT의 길이 차 구함
			strcat(buf, "   ");
			strcat(buf, procList[i].stat);
			for (int i = 0; i < gap; i++)						// STAT 좌측 정렬
				strcat(buf, " ");
			strcat(buf, " ");
		}

		// START 출력
		if (uOption) {
			gap = columnWidth[START_IDX] - strlen(procList[i].start);	// START의 길이 차 구함
			strcat(buf, procList[i].start);
			for (int i = 0; i < gap; i++)			// START 좌측 정렬
				strcat(buf, " ");
			strcat(buf, " ");
		}

		// TIME 출력
		gap = columnWidth[TIME_IDX] - strlen(procList[i].time);	// TIME의 길이 차 구함
		for (int i = 0; i < gap + 2; i++)			// TIME 우측 정렬
			strcat(buf, " ");
		strcat(buf, procList[i].time);
		strcat(buf, " ");

		// COMMAND 또는 CMD 출력
		if (aOption || uOption || xOption || fOption) {
			strcat(buf, procList[i].command);		// COMMAND 바로 출력
		}
		else {
			strcat(buf, procList[i].cmd);			// CMD 바로 출력
		}

		buf[COLS] = '\0';							// 터미널 너비만큼만 출력
		printf("%s\n", buf);
	}

	/*****		process 출력 종료	*****/
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

// Kib 단위를 Kb로 변환시키는 함수
unsigned long kib_to_kb(unsigned long kib) {
	unsigned long kb = kib * 1024 / 1000;
	return kb;
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
			else if (statbuf.st_rdev == ttyNr) {// 디바이스 ID가 ttyNr과 같은 경우
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

		if (!strcmp(symLinkName, DEVNULL))		// symbolic link가 /dev/null일 경우
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

	total_memory = kib_to_kb(total_memory);	// Kib 단위를 Kb로 변환

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

	if (!strcmp(tmp, SYSTEMD)) {		// user명이 systemd-로 시작할 경우
		tmp[strlen(SYSTEMD)-1] = '+';	// systemd+를 user명으로 저장
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

	FILE *statusFp;					
	unsigned long vmLck = 0;
	if ((statusFp = fopen(statusPath, "r")) == NULL) {
		fprintf(stderr, "fopen error for %s\n", statusPath);
		return;
	}

	char buf[BUFFER_SIZE];
	int cnt = 0;
	while (cnt < STATUS_VSZ_ROW) {			// 18행까지 read
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
		sscanf(ptr, "%lu", &proc.vsz);			// 18행에서 vsz 획득

		while (cnt < STATUS_VMLCK_ROW) {		// 19행까지 read
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
		sscanf(ptr, "%lu", &proc.shr);				// 24행에서 shr 획득

		long double mem = (long double)proc.rss / total_memory * 100;	// %MEM 계산
		if (isnan(mem) || isinf(mem) || mem > 100 || mem < 0)			// error 처리
			proc.mem = 0.0;
		else
			proc.mem = RoundDouble(mem, 1);	// 소숫점 아래 2자리에서 반올림
	}
	fclose(statusFp);

	GetTTY(path, proc.tty);				// TTY 획득

	proc.priority = atoi(statToken[STAT_PRIORITY_IDX]);	// priority 획득
	proc.nice = atoi(statToken[STAT_NICE_IDX]);			// nice 획득

	// STATE 획득
	strcpy(proc.stat, statToken[STAT_STATE_IDX]);		// 기본 state 획득

	// 세부 state 추가
	if (proc.nice < 0)					// nice하지 않은 경우
		strcat(proc.stat, "<");			// < 추가
	else if (proc.nice > 0)				// nice한 경우
		strcat(proc.stat, "N");			// N 추가

	if (vmLck > 0)						// memory lock을 했을 경우
		strcat(proc.stat, "L");			// L 추가

	int sid = atoi(statToken[STAT_SID_IDX]);	// session id 획득
	if (sid == proc.pid)				// session leader일 경우
		strcat(proc.stat, "s");			// s 추가

	int threadCnt = atoi(statToken[STAT_N_THREAD_IDX]);	// thread 개수 획득
	if (threadCnt > 1)					// multi thread일 경우
		strcat(proc.stat, "l");			// l 추가

	int tpgid = atoi(statToken[STAT_TPGID_IDX]);	// foreground process group id 획득
	if (tpgid != -1)
		strcat(proc.stat, "+");			// + 추가	


	// START 획득
	unsigned long start = time(NULL) - uptime + (startTime/hertz);
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
	if (eOption || (!aOption && !uOption && !xOption))	// 옵션이 없을 경우
		sprintf(proc.time, "%02d:%02d:%02d", tmCpuTime->tm_hour, tmCpuTime->tm_min, tmCpuTime->tm_sec);
	else
		sprintf(proc.time, "%1d:%02d", tmCpuTime->tm_min, tmCpuTime->tm_sec);

	sscanf(statToken[STAT_CMD_IDX], "(%s", proc.cmd);	// cmd 획득
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

	if (!strlen(proc.command))				// cmdline에서 읽은 문자열 길이 0일 경우
		sprintf(proc.command, "[%s]", proc.cmd);	// [cmd]로 채워기

	fclose(cmdLineFp);

	procList[procCnt].pid = proc.pid;
	procList[procCnt].uid = proc.uid;
	procList[procCnt].ppid = proc.ppid;
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
	if ((dirp = opendir(PROC)) == NULL) {		// /proc 디렉터리 open
		fprintf(stderr, "dirp error for %s\n", PROC);
		exit(1);
	}

	struct dirent *dentry;
	while ((dentry = readdir(dirp)) != NULL) {	// /proc 디렉터리 내 하위 파일들 탐색
		char path[PATH_SIZE];					// 디렉터리의 절대 경로 저장
		memset(path, '\0', PATH_SIZE);
		strcpy(path, PROC);
		strcat(path, "/");
		strcat(path, dentry->d_name);

		struct stat statbuf;
		if (stat(path, &statbuf) < 0) {			// 디렉터리의 stat 획득
			fprintf(stderr, "stat error for %s\n", path);
			exit(1);
		}

		if (!S_ISDIR(statbuf.st_mode))			// 디렉터리가 아닐 경우 skip
			continue;

		int len = strlen(dentry->d_name);
		bool isPid = true;
		for (int i = 0; i < len; i++) {			// 디렉터리가 PID인지 찾기
			if (!isdigit(dentry->d_name[i])) {	// 디렉터리명 문자가 있을 경우 PID가 아님
				isPid = false;
				break;
			}
		}
		if (!isPid)						// PID 디렉터리가 아닌 경우 skip
			continue;

		if (!aOption)					// aOption이 없을 경우 자신의 process만 보임
			if(statbuf.st_uid != myUid)	// uid가 자기 자신과 다를 경우 skip
				continue;

		if (!xOption) {					// xOption이 없을 경우 nonTerminal process는 생략
			char tty[TTY_LEN];
			memset(tty, '\0', TTY_LEN);
			GetTTY(path, tty);						// TTY 획득
			if (!strlen(tty) || !strcmp(tty, "?"))	// nonTerminal일 경우
				continue; 
		}

		if (!aOption && !uOption && !xOption) {	// 모든 Option 없을 경우
			char tty[TTY_LEN];
			memset(tty, '\0', TTY_LEN);
			GetTTY(path, tty);					// TTY 획득
			if (strcmp(tty, myTTY))				// 자기 자신과 tty 다를 경우
				continue;
		}

		AddProcessList(path, cpuTimeTable);		// PID 디렉터리인 경우 procList에 추가
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

// 입력 양식에 문제가 있을 경우 에러 메시지 출력하고 종료
void ErrorFunc(void) {
	fprintf(stderr, "error: syntax error\n\n");
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, " ps [options]\n\n");
	fprintf(stderr, " Try 'ps --help <simple|list|output|threads|misc|all>'\n");
	fprintf(stderr, "  or 'ps --help <s|l|o|t|m|a>'\\n");
	fprintf(stderr, " for addtional help text.\n\n");
	fprintf(stderr, "For more details see ps(1).\n");

	exit(0);
}