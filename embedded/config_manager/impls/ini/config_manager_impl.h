/*
 * This file is part of the IOTA Access distribution
 * (https://github.com/iotaledger/access)
 *
 * Copyright (c) 2020 IOTA Foundation
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

#ifndef _CFG_INI_H_
#define _CFG_INI_H_

#include <stddef.h>

#define CONFIG_MANAGER_DATA_SIZE 2048
#define CONFIG_MANAGER_MAX_TOKENS 1024

typedef enum {
    CONFIG_MANAGER_TOKEN_GROUP,
    CONFIG_MANAGER_TOKEN_OPTION
} ConfigManager_token_type_t;

typedef struct {
    int start;
    int eq_sign_idx;
    int end;
    int size;
    ConfigManager_token_type_t level;
} ConfigManager_token_t;

int ConfigManagerImpl_init_cb(void* in_parameter);
int ConfigManagerImpl_get_string_cb(const char* module_name, const char* option_name, char* option_value, size_t option_value_size);
int ConfigManagerImpl_get_int_cb(const char* module_name, const char* option_name, int* option_value);
int ConfigManagerImpl_get_float_cb(const char* module_name, const char* option_name, float* option_value);

#endif
