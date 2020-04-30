#include "server.h"

void usage(char *file) {
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}

int main(int argc, char *argv[]) {
    ClientList **clientlist = (ClientList**) malloc(sizeof(ClientList*));
    init(clientlist);

    int sockfd_udp, sockfd_tcp;
	int newsockfd, portno;
	char buffer[BUFLEN];
	char msg[BUFLEN];
    int bytes_read;
	struct sockaddr_in serv_addr, cli_addr;
    struct sockaddr_in from_station;
    unsigned int addrlen;
    int n, sockId, ret;
	socklen_t clilen;

	fd_set read_fds;	// Reading multitude used in select().
	fd_set tmp_fds;		// Temporary used multitude.
	int fdmax;			// Max value from read_fds multitude.

	if (argc < 2) {
		usage(argv[0]);
	}

    // Empty file descriptor reading multitudes (read_fds/tmp_fds)
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);


    // Filling UDP socket necessary data.
    sockfd_udp = socket(PF_INET, SOCK_DGRAM, 0);
	DIE(sockfd_udp < 0, "socket udp");

	portno = atoi(argv[1]);
	DIE(portno == 0, "atoi udp");

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	ret = bind(sockfd_udp, (struct sockaddr *) &serv_addr,
			sizeof(struct sockaddr));
	DIE(ret < 0, "bind udp");

    /* Add the new file descriptor (the one in charge of udp) in read_fds
    multitude. */
	FD_SET(sockfd_udp, &read_fds);
    fdmax = sockfd_udp;

    // Filling TCP socket necessary data.
	sockfd_tcp = socket(PF_INET, SOCK_STREAM, 0);
	DIE(sockfd_tcp < 0, "socket tcp");

	portno = atoi(argv[1]);
	DIE(portno == 0, "atoi tcp");

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

    // Deactivating Nagle's algorithm.
    int flag = 1;
    ret = setsockopt(sockfd_tcp, IPPROTO_TCP, TCP_NODELAY, (char*) &flag,
					sizeof(int));
    DIE(ret < 0, "neagle");

	ret = bind(sockfd_tcp, (struct sockaddr *) &serv_addr,
			sizeof(struct sockaddr));
	DIE(ret < 0, "bind tcp");

	ret = listen(sockfd_tcp, MAX_CLIENTS);
	DIE(ret < 0, "listen tcp");

    /* Add the new file descriptor (the one listening tcp conexions) in
    read_fds multitude. */
	FD_SET(sockfd_tcp, &read_fds);
    FD_SET(0, &read_fds);
    fdmax = sockfd_tcp;

    int stop = 0;   // For exit situation.
	while (1) {
		tmp_fds = read_fds;

		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		for (sockId = 0; sockId <= fdmax; sockId++) {
			if (FD_ISSET(sockId, &tmp_fds)) {
                if (sockId == 0) {
                    // Reading from keyboard.
                    memset(buffer, 0, BUFLEN);
                    fgets(buffer, BUFLEN - 1, stdin);

                    if (strncmp(buffer, "exit", 4) == 0) {
                        stop = 1;
                        for (int sockj = 1; sockj <= fdmax; sockj++) {
                            if (FD_ISSET(sockj, &read_fds)
								&& sockj != sockfd_tcp
								&& sockj != sockfd_udp) {
                                memset(buffer, 0, BUFLEN);
                                strcpy(buffer, "exit");
                                send(sockj, buffer, strlen(buffer), 0);
                                /* Removing from socket multitude the closed
								socket. */
                                FD_CLR(sockj, &read_fds);
                            }
                        }
						// This is keyboard reading socket.
                        FD_CLR(0, &read_fds);
                        break;
                    }

                } else if (sockId == sockfd_udp) {
                    // Received message from an UDP client.
					memset(buffer, 0, BUFLEN);
                    addrlen = sizeof(from_station);
                    bytes_read = recvfrom(sockId, buffer, BUFLEN, 0,
										(struct sockaddr *)&from_station,
										&addrlen);
                    DIE(bytes_read == -1, "recvfrom");

                    /* Add IP, PORT and topic to msg. (Same steps regardless
                    of type) */
        			memset(msg, 0, BUFLEN);
                    strcpy(msg, inet_ntoa(from_station.sin_addr));
                    char str_helper[40];

                    // First 50 bytes are for topic name.
                    char topic[TYPE_OFF + 1];
                    strncpy(topic, buffer, TYPE_OFF);
                    topic[TYPE_OFF] = '\0';

                    sprintf(str_helper, ":%d - %s - ",
                                                ntohs(from_station.sin_port),
                                                topic);
                    strcat(msg, str_helper);
                    // --------------------------------------------------------

                    // Extract the type of payload.
                    __uint8_t type = buffer[TYPE_OFF];

                    // Used for accessing certain parts of the msg.
                    char *offset;
                    if (type == INT_ID) {
                        // Add type to msg.
                        strcat(msg, "INT - ");

                        // Setting offset to point value. Note: it is signed.
                        offset = &buffer[VAL_OFF];

                        // Calculating the coded value.
                        int value = 0;
                        for (int i = 0; i < sizeof(value); i++) {
                            value += (unsigned char)offset[i]
									<< (8 * ((sizeof(value) - 1) - i));
                        }

                        // Setting offset to point sign.
                        offset = &buffer[SIGN_OFF];
                        __uint8_t isSigned = *offset;

                        // Add value to msg.
                        if (isSigned) {
                            sprintf(str_helper, "-%d", value);
                        } else {
                            sprintf(str_helper, "%d", value);
                        }
                        strcat(msg, str_helper);

                    } else if (type == SHORT_REAL_ID) {
                        // Add type to msg.
                        strcat(msg, "SHORT_REAL - ");

                        // Setting offset to point value. Note: it is unsigned.
                        offset = &buffer[VAL_OFF - 1];
                        __uint16_t value = 0;
                        for (int i = 0; i < sizeof(value); i++) {
                            value += (unsigned char)offset[i]
									<< (8 * ((sizeof(value) - 1) - i));
                        }

                        int toDivideBy = 100;
                        int part1 = value / toDivideBy;
                        int part2 = value % toDivideBy;
                        // Add value to msg.
                        sprintf(str_helper, "%d.%.2d", part1, part2);
                        strcat(msg, str_helper);

                    } else if (type == FLOAT_ID) {
                        // Add type to msg.
                        strcat(msg, "FLOAT - ");

                        // Setting offset to point value. Note: it is signed.
                        offset = &buffer[VAL_OFF];
                        __uint32_t value = 0;
                        for (int i = 0; i < sizeof(value); i++) {
                            value += (unsigned char)offset[i]
									<< (8 * ((sizeof(value) - 1) - i));
                        }

                        // Getting power to divide by.
                        offset = &buffer[FLOAT_POW_OFF];
                        __uint8_t pow = *offset;

                        // Setting offset to point sign.
                        offset = &buffer[SIGN_OFF];
                        __uint8_t isSigned = *offset;

                        int toDivideBy = ten_to_pow(pow);
                        int part1 = value / toDivideBy;
                        int part2 = value % toDivideBy;
                        // Add value to msg.
                        if (isSigned) {
                            if (pow == 0) {
                                sprintf(str_helper, "-%d", part1);

                            } else {
                                sprintf(str_helper, "-%d.%.*d", part1, pow,
																part2);
                            }
                        } else {
                            if (pow == 0) {
                                sprintf(str_helper, "%d", part1);
                            } else {
                                sprintf(str_helper, "%d.%.*d", part1,
																pow, part2);
                            }
                        }
                        strcat(msg, str_helper);

                    } else if (type == STRING_ID) {
                        // Add type to msg.
                        strcat(msg, "STRING - ");
                        // Add string to msg.
                        strcat(msg, &buffer[MSG_OFF]);
                    } else {
                        // Unidentified type error handling. Discard message.
                        continue;
                    }

                    ClientList *clients = *(clientlist);
                    /* Iterate through clients so we can attempt to send the
                    msg to the clients that are subscribed to specific topic.*/
                    while (clients != NULL) {
                        for (int i = 0; i < clients->data->num_topics; i++) {
                            if (strcmp(topic,
                                    clients->data->topics[i].name) == 0) {
                                /* He is subscribed, check whether to send him
                                the msg. */
                                if (connected(clients, clients->data->ID)) {
                                    /* He is connected, send him the update
                                    straight away. */
                                    send(clients->data->sockId, msg,
                                        sizeof(msg), 0);

                                } else if (clients->data->topics[i].sf == 1) {
                                    /* Not connected but he want's to know what
                                    he missed when he reconnects. */
                                    storeMsg(clients->data->ID, msg);
                                    clients->data->mustBeForwarded = 1;
                                }

                                // He can't be subscribed twice... break.
                                break;
                            }
                        }
                        clients = clients->next;
                    }

                } else if (sockId == sockfd_tcp) {
                    /* It came a conexion request on inactive socket (listen)
                    which the server accepts. */
					clilen = sizeof(cli_addr);
					newsockfd = accept(sockfd_tcp,
										(struct sockaddr *) &cli_addr,
										&clilen);
					DIE(newsockfd < 0, "accept");

                    ret = setsockopt(newsockfd, IPPROTO_TCP, TCP_NODELAY,
									(char *)&flag, sizeof(int));
                    DIE(ret < 0, "neagle");

                    // For receiving client id.
					memset(buffer, 0, BUFLEN);
					n = recv(newsockfd, buffer, sizeof(buffer), 0);
					DIE(n < 0, "recvID");

                    __uint8_t notNewClient = 0;  // new client flag.
                    int tmp_id = convert_to_ID(buffer);
                    if (containsID(*clientlist, tmp_id)) {
                        if (connected(*clientlist, tmp_id)) {
                            /* We send him connected signal because ID already
                            exists and he is connected. */
					        memset(buffer, 0, BUFLEN);
                            strcpy(buffer, "connected");
                            send(newsockfd, buffer, strlen(buffer), 0);
                            close(newsockfd);
                            continue;
                        } else {
                            SubscriberData *tmp_sub = findID(*clientlist,
															tmp_id);
                            tmp_sub->sockId = newsockfd;
                            notNewClient = 1;
                        }
                    } else {
                        /* Add this client id to the list. */
                        SubscriberData *newSubscriber = createSubscriber(
														newsockfd, tmp_id);
                        insert(clientlist, newSubscriber);
                    }

                    /* Add new socket to read descriptors multitude. */
					FD_SET(newsockfd, &read_fds);
					if (newsockfd > fdmax) {
						fdmax = newsockfd;
					}

                    if (notNewClient) {
                        // printf("Client (%d) reconnected from %s:%d.\n",
                        //                         tmp_id,
                        //                         inet_ntoa(cli_addr.sin_addr),
                        //                         ntohs(cli_addr.sin_port));
                        printf("New client (%d) connected from %s:%d.\n",
                                                tmp_id,
                                                inet_ntoa(cli_addr.sin_addr),
                                                ntohs(cli_addr.sin_port));

                        ClientList *clients = *clientlist;
                        while (clients != NULL) {
                            if (mustBeForwarded(clients)) {
                                char file_name[50];
                                sprintf(file_name, "sub%d.sf",
										clients->data->ID);

                                FILE *fp = fopen(file_name, "r");
                                DIE(fp == NULL, "fopen");

                                size_t msg_len = BUFLEN;
                                char *msg = (char*) malloc(
													msg_len * sizeof(char));
                                memset(msg, 0, msg_len);
                                ssize_t read;
                                while ((read = getline(&msg, &msg_len, fp))
												!= -1) {
                                    /* Will be reading a '\n' as well which we
                                    want to get rid of when sending. */
                                    msg[strlen(msg) - 1] = 0;
                                    send(clients->data->sockId,
										msg, msg_len, 0);
                                }

                                fclose(fp);
                                remove(file_name);
                                free(msg);
                                clients->data->mustBeForwarded = 0;
                            }
                            clients = clients->next;
                        }

                    } else {
                        printf("New client (%d) connected from %s:%d.\n",
                                                tmp_id,
                                                inet_ntoa(cli_addr.sin_addr),
                                                ntohs(cli_addr.sin_port));
                    }

                } else {
                    /* Data received from one of the subscriber sockets so
                    server must receive. */
					memset(buffer, 0, BUFLEN);
					n = recv(sockId, buffer, sizeof(buffer), 0);
					DIE(n < 0, "recv");

					if (n == 0) {
                        // Conexion closed.
                        SubscriberData *tmp_sub = findSocket(*clientlist,
															sockId);
                        tmp_sub->sockId = -1;
                        printf("Client (%d) disconnected.\n", tmp_sub->ID);
						close(sockId);


                        // Removing from socket multitude the closed socket.
						FD_CLR(sockId, &read_fds);
					} else {
                        /* Received a message(request) from one of the
						subscribers. */
                        char topic[MSG_OFF];
                        memset(topic, 0, MSG_OFF);
                        if (strncmp(buffer, "subscribe", 9) == 0) {
                            __uint8_t sf = 0;  // Default value.
                            sscanf(&buffer[10], "%s %hhd", topic, &sf);
                            subscribe(clientlist, sockId, topic, sf);

                        } else if (strncmp(buffer, "unsubscribe", 11) == 0) {
                            sscanf(&buffer[12], "%s", topic);
                            unsubscribe(clientlist, sockId, topic);
                        }
                    }
				}
			}
		}
        if (stop) {
            break;
        }
	}

    // Check to delete all stored messages.
    ClientList *client = *clientlist;
    while (client != NULL) {
        if (mustBeForwarded(client)) {
            char file_name[50];
            sprintf(file_name, "sub%d.sf", client->data->ID);
            remove(file_name);
        }
        client = client->next;
    }

    FD_ZERO(&read_fds);  // Just in case we forget. Better safe than sorry.
	close(sockfd_tcp);
    freeList(clientlist);
	return 0;
}
