/*
 *  Communication Module - TCP
 *  Author: Ediz Agarer
 *  github: https://github.com/agarere
 */
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>
#include <netinet/tcp.h>

#include <glib-2.0/glib.h>

#include "comm_module.h"

#ifndef OK
#define OK		0
#endif

#ifndef ERR
#define ERR		1
#endif

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif


#define MAX_RESPONSE_SIZE 	1024
#define BUFFER_LENGTH 		1024

/*~ Client Local Functions ~*/
static int tcp_client_init(TTCPClient *client, int port, int timeout, int try_count);
static int tcp_client_init_and_connect(TTCPClient *client, int try_count);
static int tcp_client_init_and_connect_process(TTCPClient *client);
static int tcp_client_send(TTCPClient *client, char *request, char *response);
static int tcp_client_send_to_server(TTCPClient *client, int sd , char *data);
static int tcp_client_read_with_timeout(TTCPClient *client, int client_fd, char *response);
static int tcp_client_is_server_alive(TTCPClient *client);
static int tcp_client_check_connection(TTCPClient *client, unsigned char send_ping);
static int tcp_client_connect_to_server(TTCPClient *client);
static void tcp_client_close_connection(TTCPClient *client);

/*~ Server Local Functions ~*/
static int server_socket;
static void (*server_callback_function)(char *req, char *res);
static int tcp_server_init_2(TTCPServer *server);
static int tcp_server_accept(int server_socket);
static bool tcp_client_handle_request(GIOChannel *source, GIOCondition condition, gpointer data);
static bool tcp_server_accept_connection(GIOChannel *source, GIOCondition condition, gpointer data);
static int tcp_server_init(TTCPServer *server, int port, void (*callback)(char *req, char *res));


static int tcp_server_init_2(TTCPServer *server) {

	int optval = 1;
	int optlen = sizeof(optval);
	int keep_cnt = 3;
	int keep_idle = 5;
	int keep_intvl = 1;
	int no_delay = 1;
	struct sockaddr_in address;

	if ((server->socket = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
		sprintf(server->error, "Error: creating listening socket");
		return ERR;
	}

	fcntl(server->socket, F_SETFD, fcntl(server->socket, F_GETFD,0) | FD_CLOEXEC);

	printf("[ TCP ] [ Socket Created ]\n");

	bzero((char *) &address, sizeof(address));

	address.sin_family      = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	address.sin_port        = htons(server->port);
	memset(address.sin_zero, 0, 8);

	printf("[ TCP ] [ Server Port: %d ]\n", server->port);

	setsockopt(server->socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof (optval));

	if (bind(server->socket, (const struct sockaddr *) &address, sizeof(address)) < 0) {
		sprintf(server->error, "Error: calling bind()");
		close(server->socket);
		return ERR;
	}

	if (listen(server->socket, SERVER_LISTEN_CNT) < 0) {
		close(server->socket);
		sprintf(server->error, "Error: calling listen()");
		return ERR;
	}

	if (setsockopt(server->socket, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) < 0) {
		sprintf(server->error, "Error: calling setsockopt(optval)");
		close(server->socket);
		return ERR;
	}

	if (setsockopt(server->socket, SOL_TCP, TCP_KEEPINTVL, &keep_intvl, optlen) < 0) {
		sprintf(server->error, "Error: calling setsockopt(keep_intvl)");
		close(server->socket);
		return ERR;
	}

	if (setsockopt(server->socket, SOL_TCP, TCP_KEEPIDLE, &keep_idle, optlen) < 0) {
		sprintf(server->error, "Error: calling setsockopt(keep_idle)");
		close(server->socket);
		return ERR;
	}

	if (setsockopt(server->socket, SOL_TCP, TCP_KEEPCNT, &keep_cnt, optlen) < 0) {
		sprintf(server->error, "Error: calling setsockopt(keep_cnt)");
		close(server->socket);
		return ERR;
	}

	if (setsockopt(server->socket, SOL_TCP, TCP_NODELAY, &no_delay, optlen) < 0) {
		sprintf(server->error, "Error: calling setsockopt(no_delay)");
		close(server->socket);
		return ERR;
	}

	printf("[ TCP ] [ Server: %d ]\n", server->socket);

	return server->socket;
}


static int tcp_server_accept(int server_socket) {

	struct sockaddr_in cliAddr;
	int alen = sizeof(cliAddr);
	int client_sockfd;

	client_sockfd = accept(server_socket, (struct sockaddr *) &cliAddr, &alen);

	return client_sockfd;
}

static bool tcp_client_handle_request(GIOChannel *source, GIOCondition condition, gpointer data) {

	int ret = 0;
	int retval = TRUE;
	int client_sockfd = (int) data;
	unsigned char in_buffer[BUFFER_LENGTH] = {0};
	char response_buffer[MAX_RESPONSE_SIZE];

	switch(condition) {
	case G_IO_IN:
		printf("[ TCP ] [ received data from client socket: %d ]\n", client_sockfd);
		ret = read(client_sockfd, in_buffer, BUFFER_LENGTH);
		if (ret <= 0) {
			printf("[ TCP ] [ read failed %d ]\n", ret);
			retval = FALSE;
			close(client_sockfd);
			break;
		}

		printf("[ TCP ] [ Receive buffer: %s ]\n", in_buffer);
		server_callback_function(in_buffer, response_buffer);
		printf("[ TCP ] [ Sending buffer: %s ]\n", response_buffer);
		send(client_sockfd,response_buffer, strlen(response_buffer), 0);

		break;
	case G_IO_ERR:
	case G_IO_NVAL:
	case G_IO_HUP:
		printf("[ TCP ] [ Socket error, removing function callback ]\n");
		retval = FALSE;
		close(client_sockfd);
		break;
	default:
		printf("[ TCP ] [ Unknown error ]\n");
		retval = FALSE;
		close(client_sockfd);
		break;
	}
	return retval;
}

static bool tcp_server_accept_connection(GIOChannel *source, GIOCondition condition, gpointer data) {

	int client_sockfd = -1;
	int ret = TRUE;

	switch(condition) {
	case G_IO_IN:
		printf("[ TCP ] [ Received data, probably a connection request ]\n");
		client_sockfd = tcp_server_accept(server_socket);
		if(client_sockfd < 0) {
			printf("[ TCP ] [ Accept error ]\n");
			break;
		}
		printf("[ TCP ] [ Connection accepted from: %d ]\n", client_sockfd);

		g_io_add_watch (g_io_channel_unix_new (client_sockfd), G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL, tcp_client_handle_request, (void *)client_sockfd);

		break;
	default:
		printf("[ TCP ] [ Unknown error ]\n");
		ret = FALSE;
		break;
	case G_IO_ERR:
	case G_IO_NVAL:
	case G_IO_HUP:
		printf("[ TCP ] [ Socket error, removing function callback ]\n");
		ret = FALSE;
		break;
	}

	return ret;
}

static int tcp_server_init(TTCPServer *server, int port, void (*callback)(char *req, char *res)) {

	server->initialized = FALSE;
	server->port = port;

	/*~ not must ~*/
	server->callback = callback;

	/*~ must ~*/
	server_callback_function = callback; /*~ Local variable for server callback ~*/

    server->socket = tcp_server_init_2(server);
    if (server->socket < 0) {
    	return ERR;
    }

    /*~ must ~*/
    server_socket = server->socket;

    GIOChannel *server_listen_channel = g_io_channel_unix_new (server->socket);
    g_io_add_watch (server_listen_channel, G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL, tcp_server_accept_connection, NULL);

    server->initialized = TRUE;
    return OK;
}


static void tcp_client_close_connection(TTCPClient *client) {

	if (client->sd != -1) {
		close(client->sd);
	}

	client->sd = -1;

	client->isConnected = FALSE;
}

static int tcp_client_connect_to_server(TTCPClient *client) {

	int ret;

	if (client->sd < 0) {
		ret = tcp_client_init_and_connect_process(client);
		if (ret < 0 ) {
			return ERR;
		}
	}

	client->isConnected = TRUE;

	return client->sd;
}

static int tcp_client_check_connection(TTCPClient *client, unsigned char send_ping) {

	struct stat sb;

	if (client->sd != -1) {
		if (fstat(client->sd, &sb) == -1) {
			tcp_client_close_connection(client);
		} else {
			if ((sb.st_mode & S_IFMT) != S_IFSOCK) {
				printf("[ TCP ] [ Not socket, force to initialize connection ]\n");
				client->sd = -1;
			} else {
				if (send_ping) {
					/*~ todo: add ping
					 * ~*/
				}
			}
		}
	}

	if (client->sd < 0) {
		return tcp_client_connect_to_server(client);
	}

	return client->sd;
}

static int tcp_client_is_server_alive(TTCPClient *client) {
	return tcp_client_check_connection(client, 0);
}

static int tcp_client_read_with_timeout(TTCPClient *client, int client_fd, char *response) {

	struct timeval tv;
	int ret;
	fd_set fd;
	tv.tv_sec = client->timeout;
	tv.tv_usec = 0;

	char buffer[MAX_RESPONSE_SIZE] = { 0 };

	FD_ZERO(&fd);
	FD_SET(client_fd, &fd);

	ret = select(client_fd + 1, &fd, NULL, NULL, &tv);
	printf("[ TCP ] [ select()  return: %d ]\n",ret);
	if (! ret ) {
		sprintf(client->error, "select() time out !");
		return ERR;
	}

	ret = read(client_fd, buffer, MAX_RESPONSE_SIZE);
	printf("[ TCP ] [ read() return: %d ]\n",ret);

	memcpy(response, buffer, ret);
	printf("[ TCP ] [ read(): %s ]\n", response);

	return ret;
}

static int tcp_client_send_to_server(TTCPClient *client, int sd , char *data) {

	int len,ret;

	len = strlen(data);

	if (len <= 0) {
		sprintf(client->error, "Sending data size must be > 0, But size: %d", len);
		return ERR;
	}

	if ((ret = write(sd, data, len)) < 0) {
		sprintf(client->error, "Sending error");
		free(data);
		return ERR;
	}

	printf("[ TCP ] [ write() return: %d ]\n", ret);

	return OK;
}

static int tcp_client_send(TTCPClient *client, char *request, char *response) {
	int sequence;
	int fd,ret;

	fd = tcp_client_check_connection(client, 0);
	if (fd  < 0) {

		printf("[ TCP ] [ No Client Connection ip: %s  port: %d ]\n", SERVERIP, client->serverPort);
		return ERR;
	}

	if ((ret = tcp_client_send_to_server(client, fd , request) ) != OK){
		if( ret < 0 ){
			tcp_client_close_connection(client);
		}
		printf("[ TCP ] [ Send Error ]\n");
		return ERR;
	}

	ret = tcp_client_read_with_timeout(client, fd, response);
	if (ret > 0) {
		printf("[ TCP ] [ Read data[%d]: %s ]\n", ret, response);
	} else {
		tcp_client_close_connection(client);
		return ERR;
	}

	return OK;
}

static int tcp_client_init_and_connect_process(TTCPClient *client) {

	struct hostent *server;

	server = gethostbyname(SERVERIP);
	if (server == NULL) {
		sprintf(client->error, "Error: no such host '%s'", SERVERIP);
		return ERR;
	}

	bzero((char *) &client->local_sock_addr, sizeof(client->local_sock_addr));
	bzero((char *) &client->remote_sock_addr, sizeof(client->remote_sock_addr));

	client->remote_sock_addr.sin_family = server->h_addrtype;
	memcpy((char *) &client->remote_sock_addr.sin_addr.s_addr, server->h_addr_list[0], server->h_length);
	client->remote_sock_addr.sin_port = htons(client->serverPort);
	memset(client->remote_sock_addr.sin_zero, 0, 8);

	if ((client->sd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
		sprintf(client->error, "Error: creating listening socket()");
		client->sd = -1;
		return client->sd;
	}

	printf("[ TCP ] [ Socket Created: %d ]\n", client->sd);

	client->local_sock_addr.sin_family = AF_INET;
	client->local_sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	client->local_sock_addr.sin_port = htons(0);
	memset(client->local_sock_addr.sin_zero, 0, 8);

	if (bind(client->sd, (struct sockaddr *) &client->local_sock_addr, sizeof(client->local_sock_addr)) < 0) {
		sprintf(client->error, "Error: cannot bind port: %d", client->serverPort);
		close(client->sd);
		client->sd = -1;
		return ERR;
	}

	if (connect(client->sd, (const struct sockaddr *) &client->remote_sock_addr, sizeof(client->remote_sock_addr) ) < 0 ) {
		sprintf(client->error, "Error: calling connect()");
		close(client->sd);
		client->sd = -1;
		return ERR;
	}

	printf("[ TCP ] [ connect() performed ]\n");

	return OK;
}

static int tcp_client_init_and_connect(TTCPClient *client, int try_count) {

	int ret, tryCount = 0;

	tryCount = 0;
	while (1) {

		ret = tcp_client_init_and_connect_process(client);
		if (ret >= 0) {
			break;
		}

		tryCount++;

		if (tryCount >= try_count)
			break;

		/*~ todo
		 * - make a timer
		 * ~*/
		sleep(1);

		printf("[ TCP ] [ Try to connect navit plugin, try count: %d ]\n", tryCount);
	}

	if (ret < 0 ) {
		return ERR;
	}

	return OK;
}

static int tcp_client_init(TTCPClient *client, int port, int timeout, int try_count) {

	int ret = 0;
	if (client->initialized) {
		sprintf(client->error, "Error: Already initialized");
		return ERR;
	}
	if (port <= 0) {
		sprintf(client->error, "Error: Port must be > 0, But Now: %d", port);
		return ERR;
	}
	client->serverPort = port;
	client->timeout = timeout;
	ret = tcp_client_init_and_connect(client, try_count);
    if (ret != OK) {
    	return ERR;
    }
    client->initialized = TRUE;
    client->isConnected = TRUE;
    return OK;
}

/*~ [ COMMUNICATION_newClient ]
 * - Call this function to construct a new client object.
 * - But not initialize !
 * - You must call .init() method to initialize.
 * ~*/
void COMMUNICATION_newClient(TTCPClient *client) {

	client->initialized = FALSE;
	client->isConnected = FALSE;
	client->serverPort = 0;
	client->timeout = 0;

	client->init = &tcp_client_init;
	client->send = &tcp_client_send;
	client->isServerAlive = &tcp_client_is_server_alive;
}

/*~ [ COMMUNICATION_newServer ]
 * - Call this function to construct a new server object.
 * - But not initialize !
 * - You must call .init() method to initialize.
 * ~*/
void COMMUNICATION_newServer(TTCPServer *server) {

	server->initialized = FALSE;
	server->port = 0;
	server->socket = 0;

	/*~ TODO
	 * Yeni bir bağlantı geldiğinde burayı true yap ??
	 * ~*/
	server->isClientAvailable = FALSE;

	server->init = &tcp_server_init;
}

