//-----------------------------------------------------------
// 파일명 : projserv.c
// 기  능 : 채팅 참가자 관리, 채팅 메시지 수신 및 방송
// 컴파일 : cc -o projserv projserv.c
// 사용법 : projserv 8008
// 모  드 : poling 방식으로 구현
//-----------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <socket.h>

#define MAXLINE 511
#define MAX_SOCK 1024

char *EXIT_STRIGN = "exit";
//char *NOTICE
char *START_STRING = "Connected to chat_server \n";
int num_chat = 0; 		// 참가자 수
int clisock_list[MAX_SOCK];	// 참가자 소켓 번호 목록
int listen_sock;		// 연결 요청 전용 소켓

void addClient(int s, struct sockaddr_in *newcliaddr); // 새로운 참가자 추가
void removeClient(int s);	// 탈퇴 처리 함수
int set_nonblock(int sockfd);	// 소켓을 넌블록으로 설정
int is_nonblock(int sockfd);	// 소켓이 넌블록 모드인지 확인
int ready_to_listen(int host, int port, int backlog);  // 소켓 생성과 bind 및 listen 상태
void errquit(char *mesg) {	// 에러 발생시 종료 함수
	perror(mesg);
	exit(1);
}

int main(int argc, char *argv[]) {
	struct sockaddr_in cliaddr;
	char buf[MAXLINE+1];
	int i, j, nbyte, count;
	int accp_sock, clilen, addrlen;

	if (argc != 2) {
		printf("사용법 : %s port\n", argv[0]);
		exit(0);
	}

	// 소켓 생성, bind, listen 대기 함수 호출
	printf("%s", INADDR_ANY);

//	listen_sock = ready_to_listen(INADDR_ANY, atoi()






	return 0;
}
