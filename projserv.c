//-----------------------------------------------------------
// 파일명 : projserv.c
// 기  능 : 1. 참가자 접속 및 목록 관리 o
// 		* 접속 시 모두에게 접속 알림 o
// 		* 접속 시 참가자에게 공지사항 전달 o
// 		* 탈퇴 시 모두에게 탈퇴 알림 o
// 	    2. 메시지 수신 및 전달
// 	        * 명령어인지 검사 o
// 	    	    2-1. 특정 인원에게 전달 o
// 	    	    	* 존재하는 사용자인지 검사
// 	    	    	    1) 전달
// 	    	    	    2) 거부 메시지 전달
// 	    	    2-2. 특정 인원 강퇴 
// 	    	    	* 방장인지 검사
// 	    	    	    1) 방장이면 수행후 모두에게 메시지 전달
// 	    	    	    2) 아니면 거부메시지 전달
// 	    	    2-3. 공지사항 등록 
// 	    	    	* 방장인지 검사
// 	    	    	    1) 방장이면 수행후 모두에게 메시지 전달
// 	    	    	    2) 아니면 거부 메시지 전달
// 	    	    2-4. 현재 접속자 목록 o
// 	    	    	* 요청자에게 전달 o
// 	    	    2-5. 공지사항 목록 o
// 	    	    	* 요청자에게 전달 o
// 	    	* 아니면 모두에게 메시지 전달 o
// 컴파일 : cc -o projserv projserv.c
// 사용법 : projserv 8008
// 모  드 : poling 방식으로 구현
//-----------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>

#define MAXLINE 511
#define MAX_SOCK 1024
#define NAME_LEN 20
#define MAX_NOTICE 5

typedef struct member_information {
	int s;
	char name[NAME_LEN];
} cli;

char *EXIT_STRING = "exit";
char *ADD_MEMBER_NAME = "/a# ";
char *PRIVATE_SENDING = "/p# ";
char *REQUEST_NOTICE = "/?#";
char *REQUEST_MEMBER = "/m#";
char *START_STRING = "Connected to chat_server \n";

int num_chat = 0; 		// 참가자 수
cli member_info_list[MAX_SOCK];	// 참가자 정보 목록
int num_notice = 1;
char* notice[MAX_NOTICE] = {"\n** 채팅창 이용 방법 **\n/p# 참가자이름 메시지 : 특정 인원에게 비공개 메시지 보내기\n/?# : 공지사항 목록 요청\n/m# : 참가자 목록 요청\n**********************\n"}; // 공지 목록
int listen_sock;		// 연결 요청 전용 소켓


void addNotice(char* content);	// 공지사항 추가 ///////////// 구현해야해ㅐㅐㅐㅐㅐㅐㅐㅐㅐㅐㅐㅐㅐㅐㅐㅐㅐㅐㅐㅐㅐ
void addClient(int s, struct sockaddr_in *newcliaddr); // 새로운 참가자 추가

char whichCommend(char *buf);	// 어떤 명령어인지 판단
void sendNoticeList(int s);	// 공지사항 목록 전송
void sendMemberList(int s);	// 참가자 정보 목록 전송

void removeClient(int s);	// 참가자 탈퇴 처리
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

	listen_sock = ready_to_listen(INADDR_ANY, atoi(argv[1]), 5);

	if (listen_sock == -1)
		errquit("ready_to_listen fail");
	if (set_nonblock(listen_sock) == -1)
		errquit("set_nonblock fail");

	for (count=0; ; count++) { // 무한루프
		if (count == 100000) {
			putchar('.');
			fflush(stdout);
			count = 0;
		}
		//arrlen = sizeof(cliaddr);
		accp_sock = accept(listen_sock, (struct sockaddr*)&cliaddr, &clilen);
		if (accp_sock == -1 && errno != EWOULDBLOCK) {
			errquit("accept fail");
		} else if (accp_sock > 0) {
			// 통신용 소켓은 기본적으로 block 모드
			if (set_nonblock(accp_sock) < 0) {
				errquit("set_nonblock fail");
			}
			addClient(accp_sock, &cliaddr);
			send(accp_sock, START_STRING, strlen(START_STRING), 0);
			sendNoticeList(accp_sock);
			printf("%d번째 참가자 추가.\n", num_chat);
		}

		// 클라이언트의 메시지를 수신
		for (i=0; i<num_chat; i++) {
			errno = 0;
			nbyte = recv(member_info_list[i].s, buf, MAXLINE, 0);
			if (nbyte == 0) { // 메시지 문제 발생시
				puts("메시지 문제 발생");
				removeClient(i); // 클라이언트 종료
				continue;
			} else if (nbyte == -1 && errno == EWOULDBLOCK) {
				// non-block으로 바로 반환된 경우
				continue;
			}
			
			// 명령어인지 검사
			char commend = 0;
			buf[nbyte] = 0;
			puts(buf);
			puts("명령어인지 검사");
			if ((commend = whichCommend(buf)) != 0) {
				puts("명령어네");
				switch(commend) {
					case 'a': // add member name
						puts("멤버의 이름 더하는 중");
						sprintf(member_info_list[num_chat-1].name, "%s", buf+4);
						printf("%s님이 입장하셨습니다.\n", member_info_list[num_chat-1].name);
						
						for (j=0; j<num_chat; j++) {
							char add_msg[MAXLINE] = {0};
							sprintf(add_msg, "%s님이 입장하셨습니다.\n", buf+4);
							send(member_info_list[j].s, add_msg, strlen(add_msg), 0);
						}
						break;
					case 'p': // private message
						puts("개별 메시지 전달");
						
						break;
					case '?': // request notice list
						puts("공지 리스트 전달");
						sendNoticeList(member_info_list[i].s);
						break;
					case 'm': // request member list
						puts("멤버 리스트 전달");
						sendMemberList(member_info_list[i].s);
						break;
				}
			} else { // 명령어가 아니라면 모든 참가자에게 메시지 방송
				
				if (strstr(buf, EXIT_STRING) != NULL) { // 종료 문자 처리
					puts("종료 문자 처리");
					removeClient(i); // 클라이언트 종료
					continue;
				}

				puts("전송");
				for (j=0; j<num_chat; j++) {
					if (j == i) // 메시지를 전송한 참가자에게는 방송하지 않음
						continue;
					send(member_info_list[j].s, buf, nbyte, 0);
				}
				printf("%s\n", buf);
			}

			// 종료 문자 처리
			if (strstr(buf, EXIT_STRING) != NULL) {
				puts("종료 문자 처리");
				removeClient(i); // 클라이언트 종료
				continue;
			}
		}
	} // while
	return 0;
}

// 공지사항 추가
void addNotice(char* content) {
	if (num_notice < MAX_NOTICE)
	sprintf(notice[num_notice++], "%s\n", content);

}

// 새로운 참가자 추가
void addClient(int s, struct sockaddr_in *newcliaddr) {
	char buf[20];
	inet_ntop(AF_INET, &newcliaddr->sin_addr, buf, sizeof(buf));
	printf("new client: %s\n", buf);

	// 채팅 클라이언트 목록에 추가
	member_info_list[num_chat].s = s;
	num_chat++;
}


// 명령어인지 판단
char whichCommend(char* buf) {
	if (buf[0] == '/' && buf[2] == '#') {
		return buf[1];
	} else if (strstr(buf, PRIVATE_SENDING) != NULL) {
		return 'p';
	} else if (strstr(buf, REQUEST_NOTICE) != NULL) {
		return '?';
	} else if (strstr(buf, REQUEST_MEMBER) != NULL) {
		return 'm';
	} else {
		return 0;
	}
}


// 공지사항 목록 전송
void sendNoticeList(int s) {
	char noticeList[MAXLINE] = {0};
	int i;
	
	for (i=0; i<num_notice; i++) {
		sprintf(&noticeList[strlen(noticeList)], "%s\n", notice[i]);
		puts(notice[i]); /////////////////
	}
	puts(noticeList); ///////////////
	send(s, noticeList, strlen(noticeList), 0);
}

// 참가자 정보 목록 전송
void sendMemberList(int s) {
	char memberList[MAXLINE] = "\n** 참가자 목록 **\n";
	int i;
	
	for (i=0; i<num_chat; i++) {
		sprintf(&memberList[strlen(memberList)], "%d. %s\n", i+1, member_info_list[i].name);
		puts(member_info_list[i].name);
	}
	puts(memberList);
	send(s, memberList, strlen(memberList), 0);
}






// 참가자 탈퇴 처리
void removeClient(int i) {
	char rm_msg[MAXLINE] = {0};
	int j;
	close(member_info_list[i].s);
	printf("removeClient 호출\n");
	printf("%s님이 퇴장하셨습니다. \n", member_info_list[i].name);
	
	// 모두에게 전달
	sprintf(rm_msg, "%s님이 퇴장하셨습니다.\n", member_info_list[i].name);
	for (j=0; j<num_chat; j++) {
		send(member_info_list[j].s, rm_msg, strlen(rm_msg), 0);
	}

	if (i != num_chat-1) {
		member_info_list[i] = member_info_list[num_chat-1];
	}
	num_chat--;
	printf("현재 참가자 수 = %d\n", num_chat);
}

// 소켓을 넌블록으로 설정
int set_nonblock(int sockfd) {
	int val;
	val = fcntl(sockfd, F_GETFL, 0); // 기존 플래그값 얻어오기
	if (fcntl(sockfd, F_SETFL, val | O_NONBLOCK) == -1) // 넌블럭 설정
		return -1;
	return 0;
}

// 소켓이 넌블록 모드인지 확인
int is_nonblock(int sockfd) {
	int val;
	val = fcntl(sockfd, F_GETFL, 0); // 기존 플래그값 얻어오기
	if (val & O_NONBLOCK) // 넌블록 모드인지 확인
		return 0;
	return -1;
}

// 소켓 생성과 bind 및 listen 상태
int ready_to_listen(int host, int port, int backlog) {
	int sd;
	struct sockaddr_in servaddr;

	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		errquit("socket fail");
	}

	// 서버 소켓 주소 구조체 초기화
	bzero((char*)&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(host);
	servaddr.sin_port = htons(port);

	if (bind(sd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
		errquit("bind fail");
	}

	// 소켓 수동형으로 변경
	listen(sd, backlog);
	return sd;
}
