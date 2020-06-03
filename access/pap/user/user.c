/*
 * This file is part of the IOTA Access Distribution
 * (https://github.com/iotaledger/access)
 *
 * Copyright (c) 2020 IOTA Stiftung
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

#include "user.h"

#include <string.h>

#include <sqlite3.h>

#include <libfastjson/json.h>
#include <curl/curl.h>

#include "jsmn.h"
#include "config_manager.h"

#define QUERY_STR_LEN 1024
#define QUERY_STR_MAX 2048
#define BC_HOSTNAME_LEN 256
#define DEVICE_ID_LEN 67
#define MSG_LEN 256
#define RESPONSE_LEN 1024*1024
#define STR_SIZE 128
#define PUBLIC_ID_SIZE 256
#define URL_LEN 1024

static sqlite3* users_db;

static CURL* curl = 0;
static char bc_hostname[BC_HOSTNAME_LEN] = "";
static int bc_port = 0;
static char device_id[DEVICE_ID_LEN] = "";


#define USER_MANAGEMENT_INSERT_QUERY "INSERT INTO users (username, firstName, lastName, walletId, publicId, userId) VALUES ('%s', '%s', '%s', '%s', '%s', '%s');"

static int count_cb(void *NotUsed, int argc, char **argv, char **azColName)
{
    int i = argc;

    if (NotUsed != NULL)
    {
        *((int*)NotUsed) = i;
    }
    return 0;
}

static int get_all_cb(void *data, int argc, char **argv, char **azColName)
{
    if (data == NULL)
    {
        return -1;
    }

    fjson_object* fjobj = (fjson_object*)data;
    fjson_object* fj_new_el = fjson_object_new_object();
    fjson_object_array_add(fjobj, fj_new_el);

    for (int i = 0; i<argc; i++)
    {
        if (argv[i] == NULL)
        {
            continue;
        }
        fjson_object_object_add(fj_new_el, azColName[i], fjson_object_new_string(argv[i]));
    }
    return 0;
}

static int get_cb(void *data, int argc, char **argv, char **azColName)
{
    if (data == NULL)
    {
        return -1;
    }
    fjson_object* fjobj = (fjson_object*)data;
    for (int i = 0; i<argc; i++)
    {
        fjson_object_object_add(fjobj, azColName[i], fjson_object_new_string(argv[i]));
    }
    fjson_object_object_add(fjobj, "deviceId", fjson_object_new_string(device_id));
    return 0;
}

static int table_exists()
{
    int check = 0;
    char* errMsg;
    int rc = sqlite3_exec(users_db, "SELECT name FROM sqlite_master WHERE type='table' AND name='users';", count_cb, &check, &errMsg);

    if(rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", errMsg);
        sqlite3_free(errMsg);
        return -1;
    }

    return check;
}

static int query_helper(const char* query, int (*cb)(void*,int,char**,char**), void* data)
{
    char* errMsg;
    int rc;
    rc = sqlite3_exec(users_db, query, cb, data, &errMsg);

    if(rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", errMsg);
        sqlite3_free(errMsg);
        return -1;
    }

    return 0;
}

static void create_json_response(fjson_object* response, fjson_object* data, int err, char* msg)
{
    if (data != NULL)
    {
        fjson_object_object_add(response, "data", data);
    }

    if (msg != NULL)
    {
        fjson_object_object_add(response, "message", fjson_object_new_string(msg));
    }
    else
    {
        fjson_object_object_add(response, "message", fjson_object_new_string(""));
    }

    fjson_object_object_add(response, "error", fjson_object_new_int(err));
}

static char query_string[QUERY_STR_MAX];

static int insert_user(const char* username, const char* firstName, const char* lastName, const char* walletId, const char* publicId, const char* userId)
{
    snprintf(query_string, QUERY_STR_MAX, USER_MANAGEMENT_INSERT_QUERY, username, firstName, lastName, walletId, publicId, userId);

    return query_helper(query_string, count_cb, NULL);
}

static int init_table()
{
    int rc;
    rc = query_helper("CREATE TABLE users (username TEXT PRIMARY KEY, firstName TEXT, lastName TEXT, walletId TEXT, publicId TEXT, userId TEXT);", count_cb, NULL);

    if(rc != 0)
    {
        return -1;
    }

    return 0;
}

int User_init()
{
    CURLcode curl_status = curl_global_init(CURL_GLOBAL_ALL);

    ConfigManager_get_option_string("bc_daemon", "bc_hostname", bc_hostname, BC_HOSTNAME_LEN);
    ConfigManager_get_option_int("bc_daemon", "bc_port", &bc_port);
    ConfigManager_get_option_string("config", "device_id", device_id, sizeof(device_id));

    if (curl_status != 0)
    {
        fprintf(stderr, "curl_global_init failed!");
        exit(-1);
    }

    curl = curl_easy_init();
    if (curl == NULL)
    {
        fprintf(stderr, "curl_easy_init failed!");
        exit(-1);
    }

    int rc = sqlite3_open("users.db", &users_db);

    if (rc)
    {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(users_db));
        return -1;
    }

    char* errMsg;

    int check = table_exists();
    // does table 'users' exist?
    if (check < 0)
    {
        return rc-1;
    }
    else if (check == 0)
    {
        rc = init_table();
        if (rc < 0)
        {
            return rc-2;
        }
    }
    return 0;
}

void User_deinit()
{
    sqlite3_close(users_db);

    // deinit curl
    curl_easy_cleanup(curl);
    curl_global_cleanup();
}

void User_get_all(char* response)
{
    fjson_object* fjobj_response = fjson_object_new_array();
    int rc = query_helper("SELECT username, publicId FROM users", get_all_cb, fjobj_response);

    fjson_object* fjobj_resp = fjson_object_new_object();
    if (rc == 0)
    {
        create_json_response(fjobj_resp, fjobj_response, 0, "");
    }
    else
    {
        create_json_response(fjobj_resp, NULL, 1, "Couldn't get all users!");
    }

    strncpy(response, fjson_object_to_json_string(fjobj_resp), QUERY_STR_MAX);
}

void User_get_obj(const char* id, char* response)
{
    fjson_object* fjobj_response = fjson_object_new_object();
    snprintf(query_string, QUERY_STR_MAX, "SELECT * FROM users WHERE username = '%s'", id);
    int rc = query_helper(query_string, get_cb, fjobj_response);

    fjson_object* fjobj_resp = fjson_object_new_object();
    if (rc == 0)
    {
        create_json_response(fjobj_resp, fjobj_response, 0, "");
    }
    else
    {
        char msg[MSG_LEN] = "";
        snprintf(msg, MSG_LEN, "Couldn't get '%s'!", id);
        create_json_response(fjobj_resp, NULL, 1, msg);
    }

    strncpy(response, fjson_object_to_json_string(fjobj_resp), QUERY_STR_MAX);
}

#define JSMN_TOK_MAX 128
static jsmn_parser p;
static jsmntok_t t[JSMN_TOK_MAX];

static char response[RESPONSE_LEN];

static size_t wallet_id_callback(void *buffer, size_t size, size_t nmemb, void *userp)
{
    char* in_data = (char*)buffer;
    char* out_data = (char*)userp;
    jsmn_init(&p);
    size_t r = jsmn_parse(&p, in_data, nmemb, t, JSMN_TOK_MAX);
    for (int i = 0; i < r; i++)
    {
        if (t[i].size == 0)
        {
            continue;
        }

        if (strncmp(&in_data[t[i].start], "address", strlen("address")) == 0)
        {
            strncpy(out_data, &in_data[t[i+1].start], t[i+1].end - t[i+1].start);
            break;
        }
    }
    return nmemb;
}

void User_put_obj(const char* json_string, char* json_response)
{
    jsmn_init(&p);
    size_t r = jsmn_parse(&p, json_string, strlen(json_string), t, JSMN_TOK_MAX);
    char username[STR_SIZE] = "";
    char firstName[STR_SIZE] = "";
    char lastName[STR_SIZE] = "";
    char publicId[PUBLIC_ID_SIZE] = "";
    char walletId[STR_SIZE] = "";
    char userId[STR_SIZE] = "";
    char dummy = '\0';
    for (int i = 0; i < r; i++)
    {
        if (t[i].size == 0)
        {
            continue;
        }
        if (strncmp(&json_string[t[i].start], "username", strlen("username")) == 0)
        {
            strncpy(username, &json_string[t[i+1].start], t[i+1].end - t[i+1].start);
        }
        else if (strncmp(&json_string[t[i].start], "firstName", strlen("firstName")) == 0)
        {
            strncpy(firstName, &json_string[t[i+1].start], t[i+1].end - t[i+1].start);
        }
        else if (strncmp(&json_string[t[i].start], "lastName", strlen("lastName")) == 0)
        {
            strncpy(lastName, &json_string[t[i+1].start], t[i+1].end - t[i+1].start);
        }
        else if (strncmp(&json_string[t[i].start], "userId", strlen("userId")) == 0)
        {
            strncpy(userId, &json_string[t[i+1].start], t[i+1].end - t[i+1].start);
        }
    }

    // walletId
    char url[URL_LEN];
    CURLcode res;
    snprintf(url, URL_LEN, "%s:%d/account", bc_hostname, bc_port);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, wallet_id_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &walletId);
    curl_easy_setopt(curl, CURLOPT_READDATA, &dummy);
    curl_easy_setopt(curl, CURLOPT_INFILESIZE, 0);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);
    curl_easy_setopt(curl, CURLOPT_PUT, 1L);
    res = curl_easy_perform(curl);

    if(res != CURLE_OK)
    {
        fprintf(stderr, "walletId retrieval failed: %s\n",
                curl_easy_strerror(res));
    }

    // publicId
    int i = 0;
    for (i=0; i<strlen(userId); i++)
    {
        if ((i == 0 && userId[i] == '0' && userId[i+1] == 'x') &&
            (i == 1 && userId[i] == 'x' && userId[i-1] == '0'))
        {
            continue;
        }
        int idx_o = i*2;

        snprintf(&publicId[idx_o], 3, "%02x", userId[i]);
    }

    int rc = insert_user(username, firstName, lastName, walletId, publicId, userId);
    // use url for no particular reason
    snprintf(url, URL_LEN, "{\"username\": \"%s\", \"firstName\": \"%s\", \"lastName\": \"%s\", \"walletId\": \"%s\", \"publicId\": \"%s\", \"userId\": \"%s\", \"deviceId\": \"%s\"}", username, firstName, lastName, walletId, publicId, userId, device_id);
    fjson_object* fjobj_retdata = fjson_tokener_parse(url);
    fjson_object* fjobj_resp = fjson_object_new_object();
    if (rc == 0)
    {
        create_json_response(fjobj_resp, fjobj_retdata, 0, "");
    }
    else
    {
        char msg[MSG_LEN] = "";
        snprintf(msg, MSG_LEN, "Couldn't insert user '%s'!", username);
        create_json_response(fjobj_resp, NULL, 1, msg);
    }
    strncpy(json_response, fjson_object_to_json_string(fjobj_resp), QUERY_STR_MAX);
    fjson_object_put(fjobj_resp);
}

void User_get_user_id(const char* username, char* json_string)
{
    fjson_object* fjobj_response = fjson_object_new_object();
    snprintf(query_string, QUERY_STR_MAX, "SELECT userId FROM users WHERE username = '%s'", username);
    int rc = query_helper(query_string, get_cb, fjobj_response);

    struct fjson_object_iterator iter = fjson_object_iter_begin(fjobj_response);
    struct fjson_object_iterator iter_end = fjson_object_iter_end(fjobj_response);
    int found_user = 0;
    while (!fjson_object_iter_equal(&iter, &iter_end))
    {
        if (strncmp(fjson_object_iter_peek_name(&iter), "userId", strlen("userId")) == 0)
        {
            found_user = 1;
            break;
        }
    }

    fjson_object* fjobj_resp = fjson_object_new_object();
    if (rc == 0 && found_user == 1)
    {
        create_json_response(fjobj_resp, fjobj_response, 0, "");
    }
    else
    {
        char msg[MSG_LEN] = "";
        snprintf(msg, MSG_LEN, "Couldn't find user '%s'!", username);
        create_json_response(fjobj_resp, NULL, 1, msg);
    }

    strncpy(json_string, fjson_object_to_json_string(fjobj_resp), QUERY_STR_MAX);
}

void User_clear_all(char* response)
{
    int count = 0;
    int rc = query_helper("DROP TABLE users", count_cb, &count);
    User_deinit();
    User_init(NULL, 0, NULL);
    fjson_object* fjobj_resp = fjson_object_new_object();
    create_json_response(fjobj_resp, NULL, 0, "");
    strncpy(response, fjson_object_to_json_string(fjobj_resp), QUERY_STR_MAX);
}
