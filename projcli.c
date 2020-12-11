//----------------------------------------------------------------------
// 파일명 : projcli.c
// 기  능 : 1. 서버에 접속해 키보드 입력을 서버로 전달함
// 	    2. 서버로부터 오는 메시지를 화면에 출력함
// 	    3. 현재 접속자 목록을 요청할 수 있음
// 	    4. 특정 아이디에 메시지를 전달할 수 있음
// 	    5. 방장인 경우
// 	    	5-1. 특정 아이디를 강퇴시키라는 메시지를 전달할 수 있음
// 	    	5-2. 방장인 경우, 공지사항을 추가할 수 있음
// 컴파일 : cc -o projcli projcli.c
// 사용법 : ./projcli 127.0.0.1 8008 name
// 모  드 : selecting 방식
//----------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
//#include <time.h>
//#include <fcntl.h>

#define MAXLINE 1000
#define NAME_LEN 20

char *EXIT_STRING = "exit";
int connect_to_server(int af, char *servip, unsigned short port); // 소켓생성, 서버연결, 생성된 소켓리턴
void errquit(char *mesg) {
       perror(mesg);
       exit(1);
}

int main(int argc, char *argv[]) {
	char allMessage[MAXLINE + NAME_LEN],	// name + massege를 위한 버퍼
	     *message;				// allMessage에서 메시지 부분의 포인터
	int maxfdp1,	// 소켓 디스크립터 중 최대 크기
	    s,		// 서버와 소통할 소켓
	    namelen;	// name의 길이
	fd_set read_fds;// I/O 변화 발생 감지 구조체
	
	if(argc != 4) {
		printf("사용법 : %s 서버ip port번호 이름\n", argv[0]);
		exit(0);
	}

	sprintf(allMessage, "[%s] : ", argv[3]);// allMessage의 앞부분에 name을 저장
	namelen = strlen(allMessage);
	message = allMessage + namelen; // 메시지 시작부분 지정

	s = connect_to_server(AF_INET, argv[1], atoi(argv[2]));
	if (s == -1)
		errquit("connect_to_server fail");

	puts("연결 되었습니다.");
	maxfdp1 = s+1;
	FD_ZERO(&read_fds);

	while(1) {
		FD_SET(0, &read_fds); // 키보드 입력에 대한 fd 초기화
		FD_SET(s, &read_fds); // 수신에 대한 fd 초기화

		if (select(maxfdp1, &read_fds, NULL, NULL, NULL) < 0) { // read에 대한  fds만
			errquit("select fail");
		}

		// 서버와 소통하는 소켓의 I/O 변화이면 (서버 수신 메시지)
		if (FD_ISSET(s, &read_fds)) {
			int nbyte;
			if ((nbyte = recv(s, message, MAXLINE, 0)) > 0) {
				message[nbyte] = 0;	// null 문자 추가
				puts(message);		// 화면 출력
			}
		}

		// 키보드 입력에 대한 fd의 I/O 변화이면
		if (FD_ISSET(0, &read_fds)) {
			if (fgets(message, MAXLINE, stdin)) {
				if (send(s, allMessage, namelen + strlen(message), 0) < 0) {
					puts("Error : Write error on socket.");
				}
				if (strstr(message, EXIT_STRING) != NULL) {
					puts("회의방을 종료합니다.");
					close(s);
					exit(0);
				}
			}
		}
	}
	return 0;
}


// 소켓생성, 서버연결, 생성된 소켓리턴
int connect_to_server(int af, char *servip, unsigned short port) {
	struct sockaddr_in servaddr;
	int s;

	// 소켓 생성
	if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		return -1;
	}

	// 서버 소켓 주소 구조체 초기화
	bzero((char *)&servaddr, sizeof(servaddr));
	servaddr.sin_family = af;
	servaddr.sin_addr.s_addr = inet_addr(servip);
	servaddr.sin_port = htons(port);

	// connect call
	if (connect(s, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
		return -1;
	}
	return s;
}
