/*
 * This file is part of the Frost distribution
 * (https://github.com/xainag/frost)
 *
 * Copyright (c) 2019 XAIN AG.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/****************************************************************************
 * \project Decentralized Access Control
 * \file tcp_server.c
 * \brief
 * Implementation of tcp_server module
 *
 * @Author Djordje Golubovic
 *
 * \notes
 *
 * \history
 * 07.11.2019. Initial version.
 ****************************************************************************/

#include "tcp_server.h"

#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

#include "json_parser.h"
#include "authDacHelper.h"
#include "pep.h"
#include "policystore.h"
#include "utils_string.h"
#include "user_management.h"
#include "globals_declarations.h"

#define Dlog_printf printf

#define SEND_BUFF_LEN 4096
#define READ_BUFF_LEN 1025
#define BUF_LEN 80
#define CONNECTION_BACKLOG_LEN 10
#define POL_ID_HEX_LEN 32
#define POL_ID_STR_LEN 64
#define USERNAME_LEN 128
#define USER_DATA_LEN 4096
#define TIME_50MS 50000
#define MAX_TRY_NUM 350

#define NO_ERROR 0
#define ERROR_BIND_FAILED 1
#define ERROR_LISTEN_FAILED 2
#define ERROR_CREATE_THREAD_FAILED 3

#define COMMAND_RESOLVE 0
#define COMMAND_GET_POL_LIST 1
#define COMMAND_ENABLE_POLICY 2
#define COMMAND_SET_DATASET 3
#define COMMAND_GET_DATASET 4
#define COMMAND_GET_USERNAME 5
#define COMMAND_GET_USERID 6
#define COMMAND_REDISTER_USER 7
#define COMMAND_GET_ALL_USER 8
#define COMMAND_CLEAR_ALL_USER 9

static pthread_t thread;
static dacSession_t session;
static int state = 0;
static int DAC_AUTH = 1;
static char send_buffer[SEND_BUFF_LEN];

static unsigned short port = 9998;
static int end = 0;

static int listenfd = 0;
static int connfd = 0;

static VehicleDataset_state_t* vdstate;

static void *server_thread(void *ptr);

int TCPServer_start(int portname, VehicleDataset_state_t *_vdstate)
{
    port = portname;
    vdstate = _vdstate;

    struct sockaddr_in serv_addr;
    char read_buffer[READ_BUFF_LEN];

    listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    memset(&serv_addr, '0', sizeof(serv_addr));
    memset(read_buffer, '0', sizeof(read_buffer));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);

    int retstat = bind(listenfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
    if (retstat != 0)
    {
        perror("bind failed");
        end = 1;
        return ERROR_BIND_FAILED;
    }

    if (end != 1)
    {
        retstat = listen(listenfd, CONNECTION_BACKLOG_LEN);
        if (retstat != 0)
        {
            perror("listen failed");
            end = 1;
            return ERROR_LISTEN_FAILED;
        }
    }

    if (pthread_create(&thread, NULL, server_thread, NULL))
    {
        fprintf(stderr, "Error creating thread\n");
        return ERROR_CREATE_THREAD_FAILED;
    }

    return NO_ERROR;
}

void TCPServer_stop()
{
    end = 1;
}

static ssize_t read_socket(void *ext, void *data, unsigned short len)
{
    int *sockfd = (int *)ext;
    return read(*sockfd, data, len);
}

static ssize_t write_socket(void *ext, void *data, unsigned short len)
{
    int *sockfd = (int *)ext;
    return write(*sockfd, data, len);
}

static int verify(unsigned char *key, int len)
{
    return 0;
}

static int get_server_state()
{
    return state;
}

static int append_action_item_to_str(char *str, int pos, list_t *action_item)
{
    if(action_item == NULL)
    {
        return 0;
    }
    else if(action_item->policyID == NULL)
    {
        return 0;
    }

    int buffer_position = pos;

    if(buffer_position != 1)
    {
        str[buffer_position++] = ',';
    }

    str[buffer_position++] = '{';

    // add "policy_id"
    memcpy(str + buffer_position, "\"policy_id\":\"", strlen("\"policy_id\":\""));
    buffer_position += strlen("\"policy_id\":\"");

    // add "policy_id" value
    hex_to_str(action_item->policyID, str + buffer_position, POL_ID_HEX_LEN);
    buffer_position += POL_ID_STR_LEN;
    str[buffer_position++] = '\"';

    // add "action"
    memcpy(str + buffer_position, ",\"action\":\"", strlen(",\"action\":\""));
    buffer_position += strlen(",\"action\":\"");

    // add "action" value
    memcpy(str + buffer_position, action_item->action, action_item->action_length);
    buffer_position += action_item->action_length;
    str[buffer_position++] = '\"';

    int is_paid = PolicyStore_is_policy_paid(action_item->policyID, POL_ID_HEX_LEN);

    // check if "cost" field should be added (add it if policy is not paid)
    if (is_paid == 0)
    {
        // add "cost"
        memcpy(str + buffer_position, ",\"cost\":\"", strlen(",\"cost\":\""));
        buffer_position += strlen(",\"cost\":\"");

        // add "cost" value
        memcpy(str + buffer_position, action_item->policy_cost, action_item->policy_cost_size);
        buffer_position += action_item->policy_cost_size;
        str[buffer_position++] = '\"';
    }

    str[buffer_position++] = '}';

    return buffer_position - pos;
}

static int list_to_string(list_t *action_list, char *output_str)
{
    output_str[0] = '[';
    int buffer_position = 1;
    list_t *action_list_temp = action_list;

    while(action_list_temp != NULL)
    {
        buffer_position += append_action_item_to_str(output_str, buffer_position, action_list_temp);
        action_list_temp = action_list_temp->next;
    }

    output_str[buffer_position++] = ']';
    output_str[buffer_position++] = '\0';

    return buffer_position;
}

static unsigned int doAuthWorkTiny(char **recvData)
{
    int request_code = -1;
    int decision = -1;
    unsigned int buffer_position = 0;

    char grant[] = "{\"response\":\"access granted\"}";
    char deny[] =  "{\"response\":\"access denied \"}";
    char *msg;

    int num_of_tokens = json_parser_init(*recvData);

    request_code = checkMsgFormat_new(*recvData);

    if(request_code == COMMAND_RESOLVE)
    {
        decision = pep_request_access(*recvData);

        if(decision == 1)
        {
            msg = grant;
        }
        else
        {
            msg = deny;
        }

        if(DAC_AUTH == 1)
        {
            free(*recvData);
        }

        memcpy(send_buffer, msg, sizeof(grant));
        *recvData = send_buffer;
        buffer_position = sizeof(grant);
    }
    else if (request_code == COMMAND_GET_POL_LIST)
    {
        list_t *action_list = NULL;

        // index of "user_id" token
        int user_id_index = 0;

        for (int i = 0; i < num_of_tokens; i++)
        {
            if (memcmp(*recvData + get_start_of_token(i), "user_id", strlen("user_id")) == 0)
            {
                user_id_index = i + 1;
                break;
            }
        }

        PolicyStore_get_list_of_actions(*recvData + get_start_of_token(user_id_index), get_size_of_token(user_id_index), &action_list);

        buffer_position = list_to_string(action_list, (char *)send_buffer);
        Dlog_printf("\nResponse: %s\n", send_buffer);

        if(DAC_AUTH == 1)
        {
            free(*recvData);
        }

        *recvData = send_buffer;

        PolicyStore_clear_list_of_actions(action_list);
    }
    else if (request_code == COMMAND_ENABLE_POLICY)
    {
        int policy_id_index = -1;

        for (int i = 0; i < num_of_tokens; i++)
        {
            if (memcmp(*recvData + get_start_of_token(i), "policy_id", strlen("policy_id")) == 0)
            {
                policy_id_index = i + 1;
                break;
            }
        }

        if (policy_id_index == -1)
        {
            return buffer_position;
        }

        char* pol_id_hex = calloc(get_size_of_token(policy_id_index) / 2, 1);
        str_to_hex(*recvData + get_start_of_token(policy_id_index), pol_id_hex, get_size_of_token(policy_id_index));

        int ret = -1;

        if (pol_id_hex != NULL)
        {
            ret = PolicyStore_enable_policy(pol_id_hex, get_size_of_token(policy_id_index) / 2);
        }
        free(pol_id_hex);

        if (ret == 0)
        {
            memcpy(send_buffer, grant, sizeof(grant));
            *recvData = send_buffer;
            buffer_position = sizeof(grant);
        }
        else
        {
            memcpy(send_buffer, deny, sizeof(deny));
            *recvData = send_buffer;
            buffer_position = sizeof(deny);
        }
    }
    else if (request_code == COMMAND_SET_DATASET)
    {
        int dataset_list_index = -1;

        for (int i = 0; i < num_of_tokens; i++)
        {
            if (memcmp(*recvData + get_start_of_token(i), "dataset_list", strlen("dataset_list")) == 0)
            {
                dataset_list_index = i;
                break;
            }
        }

        int arr_start = dataset_list_index + 1;

        if ((dataset_list_index == -1) || (get_token_at(arr_start).type != JSMN_ARRAY))
        {
            memcpy(send_buffer, deny, strlen(deny));
            buffer_position = strlen(deny);
        }
        else
        {
            VehicleDataset_from_json(vdstate, *recvData + get_token_at(arr_start).start, get_token_at(arr_start).end - get_token_at(arr_start).start);
            memcpy(send_buffer, grant, strlen(grant));
            buffer_position = strlen(grant);
        }
        *recvData = send_buffer;
    }
    else if (request_code == COMMAND_GET_DATASET)
    {
        buffer_position = VehicleDataset_to_json(vdstate, (char *)send_buffer);
        *recvData = send_buffer;
    }
    else if (request_code == COMMAND_GET_USERNAME)
    {
        char username[USERNAME_LEN] = "";

        for (int i = 0; i < num_of_tokens; i++)
        {
        if (memcmp(*recvData + get_start_of_token(i), "username", strlen("username")) == 0)
        {
            strncpy(username, *recvData + get_start_of_token(i+1), get_size_of_token(i+1));
            username[get_size_of_token(i+1)] = '\0';
            break;
        }
        }

        printf("get user\n");
        UserManagement_get_string(username, send_buffer);
        *recvData = send_buffer;
        buffer_position = strlen(send_buffer);
    }
    else if (request_code == COMMAND_GET_USERID)
    {
        char username[USERNAME_LEN] = "";

        for (int i = 0; i < num_of_tokens; i++)
        {
        if (memcmp(*recvData + get_start_of_token(i), "username", strlen("username")) == 0)
        {
            strncpy(username, *recvData + get_start_of_token(i+1), get_size_of_token(i+1));
            username[get_size_of_token(i+1)] = '\0';
            break;
        }
        }

        printf("get_auth_id\n");
        UserManagement_get_authenteq_id(username, send_buffer);
        *recvData = send_buffer;
        buffer_position = strlen(send_buffer);
    }
    else if (request_code == COMMAND_REDISTER_USER)
    {
        char user_data[USER_DATA_LEN];
        for (int i = 0; i < num_of_tokens; i++)
        {
        if (memcmp(*recvData + get_start_of_token(i), "user", strlen("user")) == 0)
        {
            strncpy(user_data, *recvData + get_start_of_token(i+1), get_size_of_token(i+1));
            user_data[get_size_of_token(i+1)] = '\0';
            break;
        }
        }

        printf("put user\n");
        UserManagement_put_string(user_data, send_buffer);
        *recvData = send_buffer;
        buffer_position = strlen(send_buffer);
    }
    else if (request_code == COMMAND_GET_ALL_USER)
    {
        printf("get all users\n");
        UserManagement_get_all_users(send_buffer);
        *recvData = send_buffer;
        buffer_position = strlen(send_buffer);
    }
    else if (request_code == COMMAND_CLEAR_ALL_USER)
    {
        printf("clear all users\n");
        UserManagement_clear_all_users(send_buffer);
        *recvData = send_buffer;
        buffer_position = strlen(send_buffer);
    }
    else
    {
        Dlog_printf("\nRequest message format not valid\n > %s\n", *recvData);
        memset(*recvData, '0', sizeof(send_buffer));
        memcpy(send_buffer, deny, sizeof(deny));
        *recvData = send_buffer;
        buffer_position = sizeof(deny);
    }

    return buffer_position;
}

static void *server_thread(void *ptr)
{
    while (!end)
    {
        struct timeval tv = { 0, TIME_50MS };
        int result;
        fd_set rfds;

        FD_ZERO(&rfds);
        FD_SET(listenfd, &rfds);

        result = select(listenfd + 1, &rfds, (fd_set *) 0, (fd_set *) 0, &tv);

        if (0 < result)
        {
            int ret, n = 0;

            connfd = accept(listenfd, (struct sockaddr*) NULL, NULL);

            time_t     now;
            struct tm  ts;
            char       buf[BUF_LEN];

            // Get current time
            time(&now);

            // Format time, "ddd yyyy-mm-dd hh:mm:ss zzz"
            ts = *localtime(&now);
            strftime(buf, sizeof(buf), "%H:%M:%S", &ts);

            printf("\n%s <Network status>\tClient connected", buf);

            //START

            if((state == 0))
            {
                state = 1;
                char *recvData = NULL;
                unsigned short recv_len = 0;
                int auth = -1;
                int decision = -1;

                dacInitServer(&session, &connfd);

                session.f_read = read_socket;
                session.f_write = write_socket;
                session.f_verify = verify;

                auth = dacAuthenticate(&session);

                if(auth == 0)
                {
                    dacReceive(&session, (unsigned char**)&recvData, &recv_len);
                    decision = doAuthWorkTiny(&recvData);
                    sendDecision_new(decision, &session, recvData, decision);
                }
                else
                {
                    time(&now);

                    // Format time, "ddd yyyy-mm-dd hh:mm:ss zzz"
                    ts = *localtime(&now);
                    strftime(buf, sizeof(buf), "%H:%M:%S", &ts);

                    printf("\n%s <Network status>\tError: Authentication failed\n", buf);

                    decision = 0;
                    int size = 34;
                    write_socket(&connfd, "{\"error\":\"authentication failed\"}", size);
                }

                state = 0;

                dacRelease(&session);

                unsigned char try = 0;
                while((get_server_state() != 0) && ( try++ < MAX_TRY_NUM));
            }

            // END

            close(connfd);
        }

        usleep(g_task_sleep_time);
    }
}