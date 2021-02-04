/*
 *  Communication Module - TCP
 *  Author: Ediz Agarer
 *  github: https://github.com/agarere
 */
#include <stdio.h>
#include <stdlib.h>
#include "comm_module.h"

#define PORT 8080
TTCPClient Client = { 0 };

int main(void) {

    printf("\n[ CommApp-Client ]\n");

    COMMUNICATION_newClient(&Client);
    Client.init(&Client, PORT, 100, 10);

    char req[256] = { 0 };
    char res[256] = { 0 };

    sprintf(req, "GET");

    int i = 0;

    for (i = 0; i < 10; i++) {

    	printf("\n\n\nSending: '%s'\n\n\n", req);
    	Client.send(&Client, req, res);

    }

    return 0;
}

