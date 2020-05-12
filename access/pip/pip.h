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

/****************************************************************************
 * \project Decentralized Access Control
 * \file pip.h
 * \brief
 * Implementation of Policy Information Point
 *
 * @Author Milivoje Knezevic, Strahinja Golic
 *
 * \notes
 *
 * \history
 * 12.10.2018. Initial version.
 * 07.05.2020. Refactoring
 ****************************************************************************/
#ifndef _PIP_H_
#define _PIP_H_

/****************************************************************************
 * MACROS
 ****************************************************************************/
#ifndef bool
#define bool _Bool
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define PIP_MAX_STR_LEN 256
#define PIP_MAX_AUTH_CALLBACKS 1 //After each new authority is added, this must be incremented

/****************************************************************************
 * ENUMERATIONS
 ****************************************************************************/
typedef enum
{
	PIP_NO_ERROR,
	PIP_ERROR
} PIP_error_e;

typedef enum
{
	PIP_IOTA = 0
} PIP_authorities_e;

/****************************************************************************
 * TYPES
 ****************************************************************************/
typedef struct attribute_object
{
	char type[PIP_MAX_STR_LEN];
	char value[PIP_MAX_STR_LEN];
} PIP_attribute_object_t;

/****************************************************************************
 * CALLBACKS
 ****************************************************************************/
typedef bool (*fetch_fn)(char* uri, PIP_attribute_object_t* attribute_object);

/****************************************************************************
 * API FUNCTIONS
 ****************************************************************************/
/**
 * @fn      PIP_init
 *
 * @brief   Initialize module
 *
 * @param   void
 *
 * @return  PIP_error_e error status
 */
PIP_error_e PIP_init(void);

/**
 * @fn      PIP_term
 *
 * @brief   Terminate module
 *
 * @param   void
 *
 * @return  PIP_error_e error status
 */
PIP_error_e PIP_term(void);

/**
 * @fn      PIP_register_callback
 *
 * @brief   Register callback for authority
 *
 * @param   authority - Authority which needs to register callback
 * @param   fetch - Callback to register
 *
 * @return  PIP_error_e error status
 */
PIP_error_e PIP_register_callback(PIP_authorities_e authority, fetch_fn fetch);

/**
 * @fn      PIP_unregister_callback
 *
 * @brief   Unregister callback for authority
 *
 * @param   authority - Authority which needs to unregister callback
 *
 * @return  PIP_error_e error status
 */
PIP_error_e PIP_unregister_callback(PIP_authorities_e authority);

/**
 * @fn      PIP_unregister_all_callbacks
 *
 * @brief   Unregister callbacka for every authority
 *
 * @param   void
 *
 * @return  PIP_error_e error status
 */
PIP_error_e PIP_unregister_all_callbacks(void);

/**
 * @fn      PIP_get_data
 *
 * @brief   Get attributes from plugins
 *
 * @param   uri - Request to perform
 * @param   attribute - Fetched attribute
 *
 * @return  PIP_error_e error status
 */
PIP_error_e PIP_get_data(char* uri, PIP_attribute_object_t* attribute);

#endif /* _PIP_H_ */