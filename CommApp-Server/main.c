/*
 *  Communication Module - TCP
 *  Author: Ediz Agarer
 *  github: https://github.com/agarere
 */
#include <stdio.h>
#include <stdlib.h>
#include <glib-2.0/glib.h>
#include <glib-2.0/glib/gmain.h>
#include "comm_module.h"

#define PORT 8080
TTCPServer Server = { 0 };

void server_callback(char *req, char *res) {
	printf("\n\n[ SERVER ] [ Req: %s ]\n\n", req);
	strcpy(res, "OK");
}


int main(void) {

	GMainLoop *loop;

	printf("\n[ CommApp-Server ]\n");

	COMMUNICATION_newServer(&Server);
	Server.init(&Server, PORT, &server_callback);

	loop = g_main_loop_new(NULL, 0);
	g_main_loop_run(loop);

	return 0;
}

