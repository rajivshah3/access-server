/*
 * This file is part of the DAC distribution (https://github.com/xainag/frost)
 * Copyright (c) 2019 XAIN AG.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>

#include "libauthdac.h"
#include "authDacHelper.h"


static int state = 0;
int get_server_state()
{
    return state;
}

void print_data(char *name, unsigned char *data, int len);

ssize_t read_socket(void *ext, void *data, unsigned short len)
{
   int *sockfd = (int *)ext;
   return read(*sockfd, data, len);
}

ssize_t write_socket(void *ext, void *data, unsigned short len)
{
   int *sockfd = (int *)ext;
   return write(*sockfd, data, len);
}

int verify(unsigned char *key, int len)
{
   return 0;
}


static dacSession_t session;

int main()
{
    int count = 0;
    int ret = 0;

    int sockfd = 0;
    int port = 9998;

    unsigned char recvBuff[1024];
    struct sockaddr_in serv_addr;
    char* servip = "127.0.0.1";

    memset(recvBuff, '0', sizeof(recvBuff));
    if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        printf("\n Error : Could not create socket \n");
        return 1;
    }

    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, servip, &serv_addr.sin_addr) <= 0)
    {
        printf("\n inet_pton error occured\n");
        return 1;
    }

    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\n Error : Connect Failed \n");
        return 1;
    }

//    write_socket(&sockfd, "Hello World!", 13);


    dacInitClient(&session, &sockfd);

    session.f_read = read_socket;
    session.f_write = write_socket;
    session.f_verify = verify;

    int auth = dacAuthenticate(&session);

    sendDecision_new(1, &session, "Hello World!", 13);

    unsigned short length;
    dacReceive(&session, (unsigned char**)&recvBuff, &length);

    int temp = (int)length;

    print_data("\n\n\nRECEIVED DATA: ", recvBuff, temp);

    close(sockfd);

    return 0;
}

void print_data(char *name, unsigned char *data, int len)
{
   int i;
   printf("\n%s (%d) = {", name, len);
   for (i = 0; i < len-1; i++)
   {
      printf("0x%02X, ", data[i]);
   }
   printf("0x%02X};\n\n", data[len-1]);
}


