# Linux 운영체제 가상환경 구축
1. 가상머신 VMWare Workstation 설치
2. Ubuntu 설치
3. Linux 커널 설치   
    ```
    wget https://www.kernel.org/pub/linux/kernel/v5.x/linux-5.11.22.tar.xz
    ```

4. Linux 커널 컴파일   
* Linux 커널 컴파일 하기 위한 라이브러리 설치
    ```c
    apt-get install vim make gcc kernel-package libncurses5-dev bison flex libssl-dev
    ```
* Linux 커널 컴파일하여 이미지 파일을 생성
    ```c
    make-kpkg --initrd --revision=1.0 kernel_image
    ```
    * 컴파일 도중 에러가 발생하였을 때
        ```c
        scripts/config disable SYSTEM_TRUSTED_KEYS
        ```
        ```c
        scripts/config set str SYSTEM_TRUSTED_KEYS
        ```   
* 상위 디렉토리에 deb 파일 생성 -> 새로운 커널 이미지로 부팅
    ```c
    dpkg -l linux-image 5.11.22_1.0_amd64.deb
    ```
    
    
