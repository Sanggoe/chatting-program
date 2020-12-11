//--------------------------------------------------------
// 파일명 : tcp_chatserv_nonb.c
// 기  능 : 채팅 참가자 관리, 채팅 메시지 수신 및 방송
// 컴파일 : cc -o tcp_chatserv_nonb tcp_chatserv_nonb.c
// 사용법 : tcp_chatserv_nonb 4001
//--------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <netdb.h>
#include <errno.h>
//#include <unistd.h>

#define MAXLINE 511
#define MAX_SOCK 1024	// 솔라리스의 경우 64

char *EXIT_STRING = "exit"; // 클라이언트의 종료 요청 문자열
char *START_STRING = "Connected to chat_server \n"; // 클라이언트 환영 메시지
int num_chat = 0;		// 채팅 참가자 수
int clisock_list[MAX_SOCK];	// 채팅 참가자 소켓 번호 목록
int listen_sock;		// 서버의 리슨 소켓

void addClient(int s, struct sockaddr_in *newcliaddr); // 새로운 채팅 참가자 처리
void removeClient(int s);	// 채팅 탈퇴 처리 함수
int set_nonblock(int sockfd);	// 소켓을 넌블록으로 설정
int is_nonblock(int sockfd);	// 소켓이 넌블록 모드인지 확인
int tcp_listen(int host, int port, int backlog); // 소켓 생성 및 listen
void errquit(char *mesg) {	// 에러시 종료 함수
	perror(mesg);
	exit(1);
}

int main(int argc, char* argv[]) {
	struct sockaddr_in cliaddr;
	char buf[MAXLINE+1];
	int i, j, nbyte, count;
	int accp_sock, clilen, addrlen; // 이거 추가 해줘야 컴파일 에러 안나나?

	if(argc != 2) {
		printf("사용법 : %s port\n", argv[0]);
		exit(0);
	}

	// tcp_listen(host, port, backlog) 함수 호출
	listen_sock = tcp_listen(INADDR_ANY, atoi(argv[1]), 5);
	
	if (listen_sock == -1)
		errquit("tcp_listen fail");
	if (set_nonblock(listen_sock) == -1)
		errquit("set_nonblock fail");
	
	for(count=0; ; count++) { // 무한 루프
		if (count == 100000) {
			putchar('.');
			fflush(stdout);
			count = 0;
		}
		addrlen = sizeof(cliaddr);//////// 안쓰이는중?
		accp_sock = accept(listen_sock, (struct sockaddr*)&cliaddr, &clilen);
		if(accp_sock == -1 && errno != EWOULDBLOCK) {
			errquit("accept fail");
		} else if (accp_sock > 0) {
			// 통신용 소켓은 넌블록 모드가 아님
			if (set_nonblock(accp_sock) < 0) {
				errquit("set_nonblock fail");
			}
			addClient(accp_sock, &cliaddr);
			send(accp_sock, START_STRING, strlen(START_STRING), 0);
			printf("%d번째 사용자 추가.\n", num_chat);
		}
		
		// 클라이언트가 보낸 메시지를 모든 클라이언트에게 방송
		for(i=0; i<num_chat; i++) {
			errno = 0; 
			nbyte = recv(clisock_list[i], buf, MAXLINE, 0);
			if (nbyte == 0) { // 메시지 문제 발생시
				removeClient(i); // 클라이언트 중료
				continue;
			}
			else if (nbyte == -1 && errno == EWOULDBLOCK)
				continue;
			// 종료문자 처리
			if(strstr(buf, EXIT_STRING) != NULL) {
				removeClient(i); // 클라이언트 종료
				continue;
			}
			// 모든 채팅 참가자에게 메시지 방송
			buf[nbyte] = 0;
			for(j=0; j<num_chat; j++) {
				if(j == i) // 내가 보낸 메시지는 수신하면 안되져!
					continue;
				send(clisock_list[j], buf, nbyte, 0);
			}
			printf("%s\n", buf);
		}
	} // end of while
	return 0;
}


// 새로운 채팅 참가자 처리
void addClient(int s, struct sockaddr_in *newcliaddr) {
	char buf[20];
	inet_ntop(AF_INET, &newcliaddr->sin_addr, buf, sizeof(buf));
	printf("new client: %s\n", buf); //, %d\n", buf, ntohs(newcliaddr->sin_port));
	// 채팅 클라이언트 목록에 추가
	clisock_list[num_chat] = s;
	num_chat++;
}

// 채팅 탈퇴 처리 함수
void removeClient(int s) {
	close(clisock_list[s]);
	if(s != num_chat-1) {
		clisock_list[s] = clisock_list[num_chat-1];
	}
	num_chat--;
	printf("채팅 참가자 1명 탈퇴. 현재 참가자 수 = %d\n", num_chat);
}

// 소켓이 넌블록 모드인지 확인
int is_nonblock(int sockfd) {
	int val;
	// 기존 플래그값 얻어오기
	val = fcntl(sockfd, F_GETFL, 0);
	// 넌블록 모드인지 확인
	if (val & O_NONBLOCK)
		return 0;
	return -1;
}

// 소켓을 넌블록으로 설정
int set_nonblock(int sockfd) {
	int val;
	// 기존의 플래그 값을 얻어온다
	val = fcntl(sockfd, F_GETFL, 0);
	if (fcntl(sockfd, F_SETFL, val | O_NONBLOCK) == -1)
		return -1;
	return 0;
}

// 소켓 생성 및 listen
int tcp_listen(int host, int port, int backlog){
	int sd;
	struct sockaddr_in servaddr;

	sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd == -1) {
		errquit("socket fail");
	}
	// servaddr 구조체의 내용 셋팅
	bzero((char *)&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(host);
	servaddr.sin_port = htons(port);

	if (bind(sd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
		errquit("bind fail");
	}

	// 클라이언트로부터 연결 요청을 기다림
	listen(sd, backlog);
	return sd;
}
