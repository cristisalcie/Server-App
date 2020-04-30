#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>

/*
 * Error macro checker.
 * Example:
 *     int fd = open(file_name, O_RDONLY);
 *     DIE(fd == -1, "open failed");
 */
#define DIE(assertion, call_description)	\
	do {									\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			perror(call_description);		\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)

#define BUFLEN 2000
#define MSG_OFF 51
#define MAX_ID_LEN 10 // Max chars in an client ID.

void usage(char *file) {
	fprintf(stderr, "Usage: %s <ID_Client> <IP_Server> <Port_Server>\n", file);
	exit(0);
}

int main(int argc, char *argv[]) {
    DIE(strlen(argv[1]) + 1 > MAX_ID_LEN, "max id length");

	int sockfd, n, ret;
	struct sockaddr_in serv_addr;
	char buffer[BUFLEN];
    char topic[MSG_OFF];
    char action[20];

	if (argc <= 3) {
		usage(argv[0]);
	}

    fd_set read_fds;
    int fdmax;


    sockfd = socket(PF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton");

    int flag = 1;
    ret = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char*) &flag,
					sizeof(int));
    DIE(ret < 0, "neagle");

	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "connect");


    // Send to server the chosen ID.
    n = send(sockfd, argv[1], strlen(argv[1]) + 1, 0);
    DIE(n < 0, "send");
    int stop = 0;
    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(sockfd, &read_fds);
        FD_SET(0, &read_fds);
        fdmax = sockfd;

        ret = select(fdmax + 1, &read_fds, NULL, NULL, NULL);
        DIE(ret < 0, "select");

        for (int i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) {
                if (i == sockfd) {
                // Receiving message from server.
                memset(buffer, 0, BUFLEN);
                memset(action, 0, 20);
                n = recv(i, buffer, sizeof(buffer), 0);
                DIE(n < 0, "recv");

                __uint8_t isMsgInside = 0;
                int j;
                for (j = 0; j < BUFLEN; j++) {
                    if (buffer[j] != 0) {
                        isMsgInside = 1;
                        break;
                    }
                }

                if (isMsgInside) {
                    sscanf(&buffer[j], "%s", action);
                    if (strcmp(action, "exit") == 0) {
                        stop = 1;
                        break;
                    }

                    DIE(strcmp(action, "connected") == 0, "Already connected");

                    printf("%s\n", &buffer[j]);
                }

                } else {
                    // Reading from keyboard.
                    memset(buffer, 0, BUFLEN);
                    memset(topic, 0, MSG_OFF);
                    memset(action, 0, 20);
                    fgets(buffer, BUFLEN - 1, stdin);

                    sscanf(buffer, "%s", action);

                    if (strcmp(action, "exit") == 0) {
                        stop = 1;
                        break;
                    } else if (strcmp(action, "subscribe") == 0) {
                        sscanf(&buffer[10], "%s", topic);
                        if (topic[0] == 0) {
                            // No topic given.
                            continue;
                        }
                        // Send message to server.
                        n = send(sockfd, buffer, strlen(buffer), 0);
                        DIE(n < 0, "send");
                        printf("subscribed %s\n", topic);

                    } else if (strcmp(action, "unsubscribe") == 0) {
                        sscanf(&buffer[12], "%s", topic);
                        if (topic[0] == 0) {
                            // No topic given.
                            continue;
                        }

                        // Send message to server.
                        n = send(sockfd, buffer, strlen(buffer), 0);
                        DIE(n < 0, "send");
                        printf("unsubscribed %s\n", topic);
                    }
                }
            }
        }
        if (stop) {
            break;
        }
    }

    close(sockfd);
	return 0;
}
