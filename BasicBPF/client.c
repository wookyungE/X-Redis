#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char **argv) {
    int sockfd;
    struct sockaddr_in server_addr;
    char message[] = "*3\r\n$3\r\nSET\r\n$3\r\nkey\r\n$5\r\nvalue\r\n";

    // 소켓 생성
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket");
        return 1;
    }

    // 서버 주소 설정
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(3000);
    //server_addr.sin_addr.s_addr = inet_addr("203.253.13.73");
    server_addr.sin_addr.s_addr = inet_addr("203.253.13.93");

    // 서버에 연결
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        close(sockfd);
        return 1;
    }
    printf("Connected to server\n");

    // 메시지 전송
    if (send(sockfd, message, strlen(message), 0) == -1) {
        perror("send");
        close(sockfd);
        return 1;
    }
    printf("Sent message: %s\n", message);

    // 소켓 닫기
    close(sockfd);

    return 0;
}
