#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <ctype.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/utsname.h>
#include <ncurses.h>

#define BUFFER_SIZE 1024		// 버퍼 최대 길이
#define PATH_SIZE 1024			// 경로 이름 최대 길이
#define INFO_LEN 1024           // CPU 정보들의 길이
#define FLAG_LEN 2048           // cpuinfo의 flags 길이

#define L1d_CACHE 0
#define L1i_CACHE 1
#define L2_CACHE 2
#define L3_CACHE 3

#define CPUINFO "/proc/cpuinfo"	            // /proc/cpuinfo 절대 경로
#define SYSTEM "/sys/devices/system"        // /sys/devices/system/cpu 절대 경로
#define NODE "/sys/devices/system/node"
#define ONLINE "/sys/devices/system/cpu/online"
#define IDX0 "/sys/devices/system/cpu/cpu0/cache/index0/size"
#define IDX1 "/sys/devices/system/cpu/cpu0/cache/index1/size"
#define IDX2 "/sys/devices/system/cpu/cpu0/cache/index2/size"
#define IDX3 "/sys/devices/system/cpu/cpu0/cache/index3/size"
#define VUL "/sys/devices/system/cpu/vulnerabilities/"

// /proc/cpuinfo 에서의 row
#define CPUINFO_VENDER_ID 2
#define CPUINFO_CPU_FAMILY 3
#define CPUINFO_MODEL 4
#define CPUINFO_MODEL_NAME 5
#define CPUINFO_STEPPING 6
#define CPUINFO_CPU_MHZ 8
#define CPUINFO_SIBLINGS 11
#define CPUINFO_CPU_CORES 13
#define CPUINFO_FLAGS 20
#define CPUINFO_CPU_BOGOMIPS 22
#define CPUINFO_ADDR_SIZE 26

// CPU 정보들을 저장하는 cpuInfo 구조체
typedef struct{
    char arch[INFO_LEN];
    char opmode[INFO_LEN];
    char byteorder[INFO_LEN];
    char addrsize[INFO_LEN];
    char online[INFO_LEN];
    char venderid[INFO_LEN];
    char cpufamily[INFO_LEN];
    char model[INFO_LEN];
    char modelname[INFO_LEN];
    char stepping[INFO_LEN];
    char speed[INFO_LEN];
    char bogomips[INFO_LEN];
    char vulnerability[9][INFO_LEN];
    char flags[FLAG_LEN];
    unsigned long numofcpu;
    unsigned long threads;
    unsigned long cores;
    unsigned long sockets;
    unsigned long siblings;
}cpuInfo;

void PrintLSCPU(void);
void PrintTerminal(char *);
void ReadCpuInfo(void);
void ReadFileSystemCPU(void);
void ReadFileSystemNode(void);
void ReadFileSystemVul(void);

cpuInfo info;
char *blank = "                                 ";
char cacheStr[4][INFO_LEN];
int cacheVal[4];
int nodeCnt = 0;
int termWidth;              // 터미널 넓이

int main(int argc, char *argv[]) {
    struct utsname un;
    int endian = 0x12345678;
    char *eptr = (char *)&endian;

    initscr();				// 출력 화면 지워지고 초기화, ncurses 자료 구조들을 초기화
	termWidth = COLS;		// term 너비 획득
	endwin();				// 출력 윈도우 종료

    uname(&un);
    strcpy(info.arch, un.machine);                  // 아키텍쳐를 utsname 구조체로 부터 얻음
    strcpy(info.opmode, "32-bit, 64-bit");

    if (eptr[0] > 0x20)                             
        strcpy(info.byteorder, "Little Endian");    // Byte Order Little Endian / Big Endian 판별
    else
        strcpy(info.byteorder, "Big Endian");

    ReadCpuInfo();
    ReadFileSystemCPU();
    ReadFileSystemNode();
    ReadFileSystemVul();
    PrintLSCPU();
}

void PrintLSCPU(void) {
    printf("Architecture:                    %s\n", info.arch);
    printf("CPU op-mode(s):                  %s\n", info.opmode);
    printf("Byte Order:                      %s\n", info.byteorder);
    printf("Address sizes:                   %s", info.addrsize);
    printf("CPU(s):                          %lu\n", info.numofcpu);
    printf("On-line CPU(s) list:             %s", info.online);
    printf("Thread(s) per core:              %lu\n", info.threads);
    printf("Core(s) per socket:              %lu\n", info.cores);
    printf("Socket(s):                       %lu\n", info.sockets);
    printf("NUMA node(s):                    %d\n", nodeCnt);
    printf("Vendor ID:                       "); PrintTerminal(info.venderid);
    printf("CPU family:                      "); PrintTerminal(info.cpufamily);
    printf("Model:                           "); PrintTerminal(info.model);
    printf("Model name:                      "); PrintTerminal(info.modelname);
    printf("Stepping:                        "); PrintTerminal(info.stepping);
    printf("CPU MHz:                         "); PrintTerminal(info.speed);
    printf("BogoMIPS:                        "); PrintTerminal(info.bogomips);
    printf("L1d cache:                       %d %s\n", cacheVal[L1d_CACHE], cacheStr[L1d_CACHE]);
    printf("L1i cache:                       %d %s\n", cacheVal[L1i_CACHE], cacheStr[L1i_CACHE]);
    printf("L2 cache:                        %d %s\n", cacheVal[L2_CACHE], cacheStr[L2_CACHE]);
    printf("L3 cache:                        %d %s\n", cacheVal[L3_CACHE], cacheStr[L3_CACHE]);
    printf("NUMA node0 CPU(s):               "); PrintTerminal(info.online);
    printf("Vulnerability Itlb multihit:     "); PrintTerminal(info.vulnerability[0]);
    printf("Vulnerability L1tf:              "); PrintTerminal(info.vulnerability[1]);
    printf("Vulnerability Mds:               "); PrintTerminal(info.vulnerability[2]);
    printf("Vulnerability Meltdown:          "); PrintTerminal(info.vulnerability[3]);
    printf("Vulnerability Spec store bypass: "); PrintTerminal(info.vulnerability[4]);
    printf("Vulnerability Spectre v1:        "); PrintTerminal(info.vulnerability[5]);
    printf("Vulnerability Spectre v2:        "); PrintTerminal(info.vulnerability[6]);
    printf("Vulnerability Srbds:             "); PrintTerminal(info.vulnerability[7]);
    printf("Vulnerability Tsx async abort:   "); PrintTerminal(info.vulnerability[8]);
    printf("Flags:                           "); PrintTerminal(info.flags);
}

void PrintTerminal(char *p) {
    int i = 0;
    while (p[i] != '\n') {
        if (i != 0 && (i % (termWidth - 33) == 0)) {
            printf("%s", blank);
        }
        printf("%c", p[i]);        
        i++;
    }
    printf("\n");
}

void ReadCpuInfo(void) {
    FILE *fp;
    char buf[BUFFER_SIZE];
    char *ptr;
	int cnt = 0, num_of_cpu = 0;

    if ((fp = fopen(CPUINFO, "r")) == NULL) {
        fprintf(stderr, "fopen error for %s\n", CPUINFO);
		exit(1);
    }

    memset(buf, '\0', BUFFER_SIZE);
    while (fgets(buf, BUFFER_SIZE, fp) != NULL) {
        if (strncmp(buf, "processor", 9) == 0) {
            num_of_cpu++;
        }
        memset(buf, '\0', BUFFER_SIZE);
    }
    rewind(fp);
    info.numofcpu = (unsigned long)num_of_cpu;
	
	while (cnt < CPUINFO_VENDER_ID) { 		
		memset(buf, '\0', BUFFER_SIZE);
		fgets(buf, BUFFER_SIZE, fp);            
		cnt++;
	}
    ptr = buf;
    for (int i = 0; i < strlen(buf); i++) {
        if (buf[i] == ':') {
            ptr = buf + i + 2;
            break;
        }
    }
    strcpy(info.venderid, ptr);

    while (cnt < CPUINFO_CPU_FAMILY) { 		
		memset(buf, '\0', BUFFER_SIZE);
		fgets(buf, BUFFER_SIZE, fp);            
		cnt++;
	}
    ptr = buf;
    for (int i = 0; i < strlen(buf); i++) {
        if (buf[i] == ':') {
            ptr = buf + i + 2;
            break;
        }
    }
    strcpy(info.cpufamily, ptr);
    
    while (cnt < CPUINFO_MODEL) {		
		memset(buf, '\0', BUFFER_SIZE);
		fgets(buf, BUFFER_SIZE, fp);            
		cnt++;
	}
    ptr = buf;
    for (int i = 0; i < strlen(buf); i++) {
        if (buf[i] == ':') {
            ptr = buf + i + 2;
            break;
        }
    }
    strcpy(info.model, ptr);
    
    while (cnt < CPUINFO_MODEL_NAME) {	
		memset(buf, '\0', BUFFER_SIZE);
		fgets(buf, BUFFER_SIZE, fp);            
		cnt++;
	}
    ptr = buf;
    for (int i = 0; i < strlen(buf); i++) {
        if (buf[i] == ':') {
            ptr = buf + i + 2;
            break;
        }
    }
    strcpy(info.modelname, ptr);

    while (cnt < CPUINFO_STEPPING) {	
		memset(buf, '\0', BUFFER_SIZE);
		fgets(buf, BUFFER_SIZE, fp);            
		cnt++;
	}
    ptr = buf;
    for (int i = 0; i < strlen(buf); i++) {
        if (buf[i] == ':') {
            ptr = buf + i + 2;
            break;
        }
    }
    strcpy(info.stepping, ptr);

    while (cnt < CPUINFO_CPU_MHZ) {	
		memset(buf, '\0', BUFFER_SIZE);
		fgets(buf, BUFFER_SIZE, fp);            
		cnt++;
	}
    ptr = buf;
    for (int i = 0; i < strlen(buf); i++) {
        if (buf[i] == ':') {
            ptr = buf + i + 2;
            break;
        }
    }
    strcpy(info.speed, ptr);

    while (cnt < CPUINFO_SIBLINGS) {	
		memset(buf, '\0', BUFFER_SIZE);
		fgets(buf, BUFFER_SIZE, fp);            
		cnt++;
	}
    ptr = buf;
    for (int i = 0; i < strlen(buf); i++) {
        if (buf[i] == ':') {
            ptr = buf + i + 2;
            break;
        }
    }
    sscanf(ptr, "%lu", &info.siblings);

    while (cnt < CPUINFO_CPU_CORES) {	
		memset(buf, '\0', BUFFER_SIZE);
		fgets(buf, BUFFER_SIZE, fp);            
		cnt++;
	}
    ptr = buf;
    for (int i = 0; i < strlen(buf); i++) {
        if (buf[i] == ':') {
            ptr = buf + i + 2;
            break;
        }
    }
    sscanf(ptr, "%lu", &info.cores);

    while (cnt < CPUINFO_FLAGS) {	
		memset(buf, '\0', BUFFER_SIZE);
		fgets(buf, BUFFER_SIZE, fp);            
		cnt++;
	}
    ptr = buf;
    for (int i = 0; i < strlen(buf); i++) {
        if (buf[i] == ':') {
            ptr = buf + i + 2;
            break;
        }
    }
    strcpy(info.flags, ptr);

    while (cnt < CPUINFO_CPU_BOGOMIPS) {	
		memset(buf, '\0', BUFFER_SIZE);
		fgets(buf, BUFFER_SIZE, fp);            
		cnt++;
	}
    ptr = buf;
    for (int i = 0; i < strlen(buf); i++) {
        if (buf[i] == ':') {
            ptr = buf + i + 2;
            break;
        }
    }
    strcpy(info.bogomips, ptr);

    while (cnt < CPUINFO_ADDR_SIZE) {	
		memset(buf, '\0', BUFFER_SIZE);
		fgets(buf, BUFFER_SIZE, fp);            
		cnt++;
	}
    ptr = buf;
    for (int i = 0; i < strlen(buf); i++) {
        if (buf[i] == ':') {
            ptr = buf + i + 2;
            break;
        }
    }
    strcpy(info.addrsize, ptr);

    info.threads = info.siblings / info.cores;
    info.sockets = (info.numofcpu / info.threads) / info.cores;

    fclose(fp);
}

void ReadFileSystemCPU(void) {
    FILE *fp;
    DIR *dp;
	struct dirent *dentry;
    char buf[BUFFER_SIZE];
    
    if ((fp = fopen(ONLINE, "r")) == NULL) {     // online cpu 값 추출
        fprintf(stderr, "open error for %s\n", ONLINE);
        exit(1);
    }
    fgets(info.online, INFO_LEN, fp);
    fclose(fp);

    if ((fp = fopen(IDX0, "r")) == NULL) {      // 코어 하나 당 L1d Cache 사이즈
        fprintf(stderr, "open error for %s\n", IDX0);
        exit(1);
    }
    memset(buf, 0, BUFFER_SIZE);
    fgets(buf, BUFFER_SIZE, fp);
    fclose(fp);
    for (int i = 0; i < strlen(buf); i++) {
        if (!isdigit(buf[i])) {
            buf[i] = '\0';
            break;
        }
    }
    cacheVal[L1d_CACHE] = atoi(buf);            // L1d cache 값
    cacheVal[L1d_CACHE] *= info.numofcpu;

    if ((fp = fopen(IDX1, "r")) == NULL) {      // 코어 하나 당 L1i Cache 사이즈
        fprintf(stderr, "open error for %s\n", IDX1);
        exit(1);
    }
    memset(buf, 0, BUFFER_SIZE);
    fgets(buf, BUFFER_SIZE, fp);
    fclose(fp);
    for (int i = 0; i < strlen(buf); i++) {
        if (!isdigit(buf[i])) {
            buf[i] = '\0';
            break;
        }
    }
    cacheVal[L1i_CACHE] = atoi(buf);            // L1i cache 값
    cacheVal[L1i_CACHE] *= info.numofcpu;

    if ((fp = fopen(IDX2, "r")) == NULL) {      // 코어 하나 당 L2 Cache 사이즈
        fprintf(stderr, "open error for %s\n", IDX2);
        exit(1);
    }
    memset(buf, 0, BUFFER_SIZE);
    fgets(buf, BUFFER_SIZE, fp);
    fclose(fp);
    for (int i = 0; i < strlen(buf); i++) {
        if (!isdigit(buf[i])) {
            buf[i] = '\0';
            break;
        }
    }
    cacheVal[L2_CACHE] = atoi(buf);             // L2 cache 값
    cacheVal[L2_CACHE] *= info.numofcpu;

    if ((fp = fopen(IDX3, "r")) == NULL) {      // 코어 하나 당 L3 Cache 사이즈
        fprintf(stderr, "open error for %s\n", IDX3);
        exit(1);
    }
    memset(buf, 0, BUFFER_SIZE);
    fgets(buf, BUFFER_SIZE, fp);
    fclose(fp);
    for (int i = 0; i < strlen(buf); i++) {
        if (!isdigit(buf[i])) {
            buf[i] = '\0';
            break;
        }
    }
    cacheVal[L3_CACHE] = atoi(buf);             // L3 cache 값
    cacheVal[L3_CACHE] *= info.sockets;

    for (int i = 0; i < 4; i++) {
        if (cacheVal[i] >= 1024) {
            cacheVal[i] /= 1024;
            strcpy(cacheStr[i], "MiB");
        }
        else {
            strcpy(cacheStr[i], "KiB");
        }
    }
}

void ReadFileSystemNode(void) {
    DIR *dp;
	struct dirent *dentry;

	if ((dp = opendir(NODE)) == NULL) {		    // /sys/devices/system/node 디렉터리 open
		fprintf(stderr, "opendir error for %s\n", NODE);
		exit(1);
	}
	
    char nowPath[PATH_SIZE];
	while ((dentry = readdir(dp)) != NULL) {    // /node 디렉터리 탐색
		memset(nowPath, '\0', PATH_SIZE);	    // 현재 탐색 중인 파일 절대 경로
		strcpy(nowPath, NODE);
		strcat(nowPath, "/");
		strcat(nowPath, dentry->d_name);

		struct stat statbuf;
		if (stat(nowPath, &statbuf) < 0) {	    // stat 획득
			fprintf(stderr, "stat error for %s\n", nowPath);
			exit(1);
		}
		if (!S_ISDIR(statbuf.st_mode))		    // 디렉터리가 아닐 경우 continue
			continue;
		
        if (strncmp(dentry->d_name, "node", 4) == 0)
            nodeCnt++;
    }
	closedir(dp);  
}

void ReadFileSystemVul(void) {
    DIR *dp;
	struct dirent *dentry;

	if ((dp = opendir(VUL)) == NULL) {		    // /sys/devices/system/cpu/vulnerabilities open
		fprintf(stderr, "opendir error for %s\n", VUL);
		exit(1);
	}
	
    char nowPath[PATH_SIZE];
    char buf[BUFFER_SIZE];
	while ((dentry = readdir(dp)) != NULL) {    // /node 디렉터리 탐색
		memset(nowPath, '\0', PATH_SIZE);	    // 현재 탐색 중인 파일 절대 경로
		strcpy(nowPath, VUL);
		strcat(nowPath, "/");
		strcat(nowPath, dentry->d_name);

        FILE *fp;
        if ((fp = fopen(nowPath, "r")) == NULL) {
            break;
        }

        if (!strcmp(dentry->d_name, "itlb_multihit")) {
            memset(buf, '\0', BUFFER_SIZE);
            fgets(buf, BUFFER_SIZE, fp);
            strcpy(info.vulnerability[0], buf);
        }

        if (!strcmp(dentry->d_name, "l1tf")) {
            memset(buf, '\0', BUFFER_SIZE);
            fgets(buf, BUFFER_SIZE, fp);
            strcpy(info.vulnerability[1], buf);
        }

        if (!strcmp(dentry->d_name, "mds")) {
            memset(buf, '\0', BUFFER_SIZE);
            fgets(buf, BUFFER_SIZE, fp);
            strcpy(info.vulnerability[2], buf);
        }

        if (!strcmp(dentry->d_name, "meltdown")) {
            memset(buf, '\0', BUFFER_SIZE);
            fgets(buf, BUFFER_SIZE, fp);
            strcpy(info.vulnerability[3], buf);
        }

        if (!strcmp(dentry->d_name, "spec_store_bypass")) {
            memset(buf, '\0', BUFFER_SIZE);
            fgets(buf, BUFFER_SIZE, fp);
            strcpy(info.vulnerability[4], buf);
        }

        if (!strcmp(dentry->d_name, "spectre_v1")) {
            memset(buf, '\0', BUFFER_SIZE);
            fgets(buf, BUFFER_SIZE, fp);
            strcpy(info.vulnerability[5], buf);
        }

        if (!strcmp(dentry->d_name, "spectre_v2")) {
            memset(buf, '\0', BUFFER_SIZE);
            fgets(buf, BUFFER_SIZE, fp);
            strcpy(info.vulnerability[6], buf);
        }

        if (!strcmp(dentry->d_name, "srbds")) {
            memset(buf, '\0', BUFFER_SIZE);
            fgets(buf, BUFFER_SIZE, fp);
            strcpy(info.vulnerability[7], buf);
        }

        if (!strcmp(dentry->d_name, "tsx_async_abort")) {
            memset(buf, '\0', BUFFER_SIZE);
            fgets(buf, BUFFER_SIZE, fp);
            strcpy(info.vulnerability[8], buf);
        }
        fclose(fp);
    }
	closedir(dp);  
}