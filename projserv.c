//-----------------------------------------------------------
// 파일명 : projserv.c
// 기  능 : 1. 참가자 접속 및 목록 관리 o
// 		* 접속 시 모두에게 접속 알림 o
// 		* 접속 시 참가자에게 공지사항 전달 o
// 		* 탈퇴 시 모두에게 탈퇴 알림 o
// 	    2. 메시지 수신 및 전달 o
// 	        * 명령어인지 검사 o
// 	    	    2-1. 특정 인원에게 전달 o
// 	    	    	* 존재하는 사용자인지 검사 o
// 	    	    	    1) 전달 o
// 	    	    	    2) 거부 메시지 전달o
// 	    	    2-2. 특정 인원 강퇴 o 
// 	    	    	* 방장인지 검사 o
// 	    	    	    1) 방장이면 수행후 모두에게 메시지 전달 o
// 	    	    	    2) 아니면 거부메시지 전달 o
// 	    	    2-3. 공지사항 등록 o
// 	    	    	* 방장인지 검사 o
// 	    	    	    1) 방장이면 수행후 모두에게 메시지 전달 o
// 	    	    	    2) 아니면 거부 메시지 전달 o
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
#define MAX_NOTICE 4

typedef struct member_information {
	int s;
	char name[NAME_LEN];
} cli;

char* EXIT_STRING = "exit";
char* ADD_NOTICE = "/n+# ";
char* REMOVE_NOTICE = "/n-# ";
char* KICK_MEMBER = "/k# ";
char* ADD_MEMBER_NAME = "/a# ";
char* PRIVATE_SENDING = "/p# ";
char* REQUEST_NOTICE = "/?#";
char* REQUEST_MEMBER = "/m#";

int num_chat = 0; 		// 참가자 수
cli member_info_list[MAX_SOCK];	// 참가자 정보 목록
int num_notice = 1;
char* notice[MAX_NOTICE] = {"채팅창 이용 방법 ******************************************\n/n+# 내용 : (방장 권한) 공지사항 추가\n/n-# 숫자 : (방장 권한) n번째 공지사항 삭제\n/k# 참가자이름 공백 : (방장 권한) 특정 인원 강퇴\n/p# 참가자이름 메시지 : 특정 인원에게 비공개 메시지 보내기\n/?# : 공지사항 목록 요청\n/m# : 참가자 목록 요청\n********************************************************\n"}; // 공지 목록
int listen_sock;		// 연결 요청 전용 소켓

char whichCommend(char *msg);	// 어떤 명령어인지 판단
int isManager(int i);		// 방장인지 확인
int addNotice(char* content);	// 공지사항 추가
int getNoticeNum(char *msg);	// 공지 번호 얻기
int removeNotice(int num);	// 공지사항 삭제
void sendNoticeList(int s);	// 공지사항 목록 전송
void sendMemberList(int s);	// 참가자 정보 목록 전송
int findMemberSock(char *msg);	// 특정 참가자의 소켓번호 찾기
char* tokenMessage(char *msg);	// 메시지부분 분리하기
void sendPrivateMessage(char *msg, int s); // 특정 참가자에게 비공개 메시지 전송

void addClient(int s); 		// 새로운 참가자 추가
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
	listen_sock = ready_to_listen(INADDR_ANY, atoi(argv[1]), 5);

	if (listen_sock == -1)
		errquit("ready_to_listen fail");
	if (set_nonblock(listen_sock) == -1)
		errquit("set_nonblock fail");

	puts("서버 개설");
	for (count=0; ; count++) { // 무한루프
		if (count == 100000) {
			fflush(stdout);
			count = 0;
		}
		accp_sock = accept(listen_sock, (struct sockaddr*)&cliaddr, &clilen);
		if (accp_sock == -1 && errno != EWOULDBLOCK) {
			errquit("accept fail");
		} else if (accp_sock > 0) {
			// 통신용 소켓은 기본적으로 block 모드
			if (set_nonblock(accp_sock) < 0) {
				errquit("set_nonblock fail");
			}
			addClient(accp_sock);
			sendNoticeList(accp_sock); // 참가자에게 공지사항 전달
			printf("%d번째 참가자 추가.\n", num_chat);
		}

		// 클라이언트의 메시지를 수신
		for (i=0; i<num_chat; i++) {
			errno = 0;
			nbyte = recv(member_info_list[i].s, buf, MAXLINE, 0);
			if (nbyte == 0) { // 메시지 문제 발생시
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
			if ((commend = whichCommend(buf)) != 0) { // 명령어인지 검사
				switch(commend) { // 어떤 명령어인지 확인하기
					case 'a': // 참가자 이름 더하기 및 입장 알림
						sprintf(member_info_list[num_chat-1].name, "%s", buf+4);
						printf("%s님이 입장하셨습니다.\n", member_info_list[num_chat-1].name);
						
						for (j=0; j<num_chat; j++) {
							char add_msg[MAXLINE] = {0};
							sprintf(add_msg, "%s님이 입장하셨습니다.\n", buf+4);
							send(member_info_list[j].s, add_msg, strlen(add_msg), 0);
						}
						break;
					case 'p': // 개별 메시지 전달
						printf("");
						int privateSock = findMemberSock(buf); // 특정 참가자 소켓 찾기
						if (privateSock < 0) {
							sendPrivateMessage("존재하지 않는 참가자 입니다.", member_info_list[i].s);
						} else { // 해당 참가자 소켓으로만 메시지 전달
							char msg[MAXLINE];
							sprintf(msg, "[%s](비공개) : %s", member_info_list[i].name, tokenMessage(buf));
							sendPrivateMessage(msg, member_info_list[privateSock].s);
						}
						break;
					case '?': // 공지사항 목록 요청에 대한 전송
						sendNoticeList(member_info_list[i].s);
						break;
					case 'm': // 참가자 목록 요청에 대한 전송
						sendMemberList(member_info_list[i].s);
						break;
					case '+': // (방장 권한) 공지사항 추가 및 새 공지 알림
						if (isManager(i) != -1) { // 방장 확인
							if (addNotice(buf) < 0) { // 추가 실패 알림
								sendPrivateMessage("공지사항이 꽉차서 추가할 수 없습니다.", member_info_list[i].s);
							} else { // 추가 성공 및 참가자에게 공지사항 전송
								sendPrivateMessage("공지가 추가되었습니다.\n", member_info_list[i].s);
								for (j=0; j<num_chat; j++) {
									sendNoticeList(member_info_list[j].s); // 모두에게 공지사항 전송
								}
							}
						} else { // 방장 외 접근 금지 알림 전송
							sendPrivateMessage("방장만 접근할 수 있는 명령입니다.", member_info_list[i].s);
						}
						break;
					case '-': // (방장 권한) 공지사항 삭제
						if (isManager(i) != -1) { // 방장 확인
							char num = getNoticeNum(buf); // 메시지에서 삭제할 공지사항 번호 추출
							if (num != -1) {
								if (removeNotice(num) < 0) { // 삭제 실패시
									sendPrivateMessage("삭제할 수 없는 공지입니다.", member_info_list[i].s);
								} else { // 삭제 성공
									sendPrivateMessage("공지가 삭제되었습니다.", member_info_list[i].s);
								}
							} else { // 올바른 숫자가 아닌 경우 오류 메시지 전송
								sendPrivateMessage("옳은 명령이 아닙니다.", member_info_list[i].s);
							}
						} else { // 방장 외 접근 금지 알림 전송
							sendPrivateMessage("방장만 접근할 수 있는 명령입니다.", member_info_list[i].s);
						}
						break;
					case 'k':
						if (isManager(i) != -1) { // 방장 확인
							int privateSock = findMemberSock(buf); // 특정 참가자 소켓 찾기
							if (privateSock < 0) { // 존재하지 않는 참가자 알림
								sendPrivateMessage("존재하지 않는 참가자 입니다.", member_info_list[i].s);
							} else { // 강퇴
								sendPrivateMessage("방장에 의해 강퇴 되었습니다.", member_info_list[privateSock].s);
								removeClient(privateSock);
							}
						} else { // 방장 외 접근 금지 알림 전송
							sendPrivateMessage("방장만 접근할 수 있는 명령입니다.", member_info_list[i].s);
						}
				}

			} else { // 명령어가 아니라면 모든 참가자에게 메시지 방송
				if (strstr(buf, EXIT_STRING) != NULL) { // 종료 문자 처리
					removeClient(i); // 클라이언트 종료
					continue;
				}

				for (j=0; j<num_chat; j++) {
					if (j == i) // 메시지를 전송한 참가자에게는 방송하지 않음
						continue;
					send(member_info_list[j].s, buf, nbyte, 0);
				}
				printf("%s\n", buf);
			}

			// 종료 문자 처리
			if (strstr(buf, EXIT_STRING) != NULL) {
				removeClient(i); // 클라이언트 종료
				continue;
			}
		}
	} // while
	return 0;
}

// 명령어인지 판단
char whichCommend(char* msg) {
	if (msg[0] == '/' && msg[2] == '#') {
		return msg[1];
	} else if (strstr(msg, PRIVATE_SENDING) != NULL) {
		return 'p';
	} else if (strstr(msg, REQUEST_NOTICE) != NULL) {
		return '?';
	} else if (strstr(msg, REQUEST_MEMBER) != NULL) {
		return 'm';
	} else if (strstr(msg, ADD_NOTICE) != NULL) {
		return '+';
	} else if (strstr(msg, REMOVE_NOTICE) != NULL) {
		return '-';
	} else if (strstr(msg, KICK_MEMBER) != NULL) {
		return 'k';
	} else {
		return 0;
	}
}

// 방장인지 확인
int isManager(int i) {
	if (strstr(member_info_list[i].name, "manager") != NULL) {
		puts("방장 권한 확인 완료");
		return 1;
	} else {
		return -1;
	}
}

// 공지사항 추가
int addNotice(char* content) {
	if (num_notice < MAX_NOTICE) {
		notice[num_notice] = (char*)malloc(strlen(content)+1);
	        sprintf(notice[num_notice++], "%s", strstr(content, ADD_NOTICE)+5);
		puts("공지사항 추가 완료");
		return 0;
	} else {
		return -1;
	}
}

// 공지 번호 얻기
int getNoticeNum(char* msg) {
	char num = msg[strlen(msg)-2];
	if (isdigit(num)) {
		return num-'0'; // 정수 반환
	} else {
		return -1; // 숫자가 아닌 경우
	}
}

// 공지사항 삭제
int removeNotice(int num) {
	int i;
	if (num > 0 && num < num_notice) {
		free(notice[num]);
		for (i=num; i<num_notice-1; i++) {
			notice[i] = notice[i+1];
		}
		num_notice--;
		puts("공지사항 삭제 완료");
		return 0;
	} else {
		return -1;
	}
}

// 공지사항 목록 전송
void sendNoticeList(int s) {
	char noticeList[MAXLINE*2] = {0}; 
	int i;
		
	for (i=0; i<num_notice; i++) {
		sprintf(&noticeList[strlen(noticeList)], "%d. %s", i, notice[i]);
	}
	send(s, noticeList, strlen(noticeList), 0);
	puts("공지사항 목록 전송 완료");
}

// 참가자 정보 목록 전송
void sendMemberList(int s) {
	char memberList[MAXLINE] = "\n** 참가자 목록 **\n";
	int i;
	
	for (i=0; i<num_chat; i++) {
		sprintf(&memberList[strlen(memberList)], "%d. %s\n", i+1, member_info_list[i].name);
	}
	send(s, memberList, strlen(memberList), 0);
	puts("참가자 목록 전송 완료");
}
	
// 특정 참가자의 소켓 번호 찾기
int findMemberSock(char *msg) {
	int i;
	char *temp = (char*)malloc(strlen(msg)); // 임시 변수할당
	sprintf(temp, "%s", strstr(msg, "/")+4); // 명령어 뒤에오는 첫 글자의 주소를 복사
	temp = strtok(temp, " "); // 토큰으로 분리하여 참가자 이름을 추출

	for (i=0; i<num_chat; i++) {
		if (strstr(temp, member_info_list[i].name) != NULL) {
			free(temp);
			puts("존재하는 참가자");
			return i;
		}
	}
	free(temp);
	puts("존재하지 않는 참가자");
	return -1;
}

// 메시지부분 분리하기
char* tokenMessage(char *msg) {
	char *temp = strstr(msg, PRIVATE_SENDING)+4;
	while (*temp != ' ')
		temp++;
	puts(temp+1);
	return temp+1;
}

// 특정 참가자에게 비공개 메시지 전송
void sendPrivateMessage(char *msg, int s) {
	puts("개별 메시지 전송");
	send(s, msg, strlen(msg), 0);
}

// 새로운 참가자 추가
void addClient(int s) {
	member_info_list[num_chat].s = s; // 참가자 목록에 추가
	num_chat++;
}

// 참가자 탈퇴 처리
void removeClient(int i) {
	char rm_msg[MAXLINE] = {0};
	int j;
	close(member_info_list[i].s);
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
