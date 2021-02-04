/*
 *  Communication Module - TCP
 *  Author: Ediz Agarer
 *  github: https://github.com/agarere
 */
#ifndef COMM_MODULE_H_
#define COMM_MODULE_H_

#include <stdbool.h>
#include <netinet/in.h>

#define SERVERIP "127.0.0.1"
#define SERVER_LISTEN_CNT	3

typedef struct TTCPServer TTCPServer;

typedef struct TTCPClient TTCPClient;

/*~
 * Construct Function: COMMUNICATION_newServer()
 * ~*/
struct TTCPServer {

	/*~ Call this function, After the construct
	 *  This will initialize your "server" object and
	 *  your server switch listen mode, if this function return OK.
	 * ~*/
	int (*init)(TTCPServer *server, int port, void (*callback)(char *req, char *res));

	/*~ Any client sending data to this server,
	 *  call this function,
	 *  with params
	 *  - req: receiving data from client
	 *  - res: answer data to client
	 * ~*/
	void (*callback)(char *req, char *res);

	int port;	/*~ Server Port ~*/
	int socket;	/*~ Server Socket ~*/

	bool isClientAvailable; /*~ If any client connect the server, this will be TRUE ~*/
	char error[128]; 			/*~ If any function return with  ERR, There is information of error in this item. ~*/
	bool initialized; 		/*~ If "client" object initialized, this will be TRUE ~*/
};

/*~
 * Construct Function: COMMUNICATION_newClient()
 * ~*/
struct TTCPClient {

	/*~ Public ~*/

	/*~ Call this function, After the construct
	 *  This will initialize your "client" object and
	 *  your "client" object will connect to server.
	 *  - client: your client object
	 *  - port: server port
	 *  - timeout: to waiting response from server
	 *  - try_count: If not connect to server, How many times try to connect
	 *  - Return: OK-> Initialized successfully
	 *  		 ERR-> Not initialized
	 * ~*/
	int (*init)(TTCPClient *client, int port, int timeout, int try_count);

	/*~ Call this function to check your connection to server
	 * - client: your client object
	 * - Return: If the server is alive, return file descriptor of socket
	 * ~*/
	int (*isServerAlive)(TTCPClient *client);

	/*~ Call this function to send data to server
	 * - client: your client object
	 * - req: sending data
	 * - res: response of server
	 * ~*/
	int (*send)(TTCPClient *client, char *req, char *res);

	int timeout;	/*~ Waiting response timeout ~*/
	int serverPort;	/*~ Server port ~*/

	bool isConnected;   /*~ If Connected to server, this will be TRUE ~*/
	char error[128];		/*~ If any function return with  ERR, There is information of error in this item. ~*/
	bool initialized;   /*~ If "client" object initialized, this will be TRUE ~*/

	/*~ Private ~*/
	int sd;
	int structlength;
	struct sockaddr_in local_sock_addr;
	struct sockaddr_in remote_sock_addr;
};

extern void COMMUNICATION_newClient(TTCPClient *client);
extern void COMMUNICATION_newServer(TTCPServer *server);


/*~ Define your objects in here with "extern" ~*/
extern TTCPServer Server;



#endif /* COMM_MODULE_H_ */
