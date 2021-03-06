/*
 * This file is part of the IOTA Access distribution
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
 * \project IOTA Access
 * \file policy_updater_logger.h
 * \brief
 * Logger for policy_updater module
 *
 * @Author Bernardo Araujo
 *
 * \notes
 *
 * \history
 * 01.08.2020. Initial version.
 ****************************************************************************/

#ifndef POLICY_LOADER_LOGGER_H
#define POLICY_LOADER_LOGGER_H

#include "utils/logger_helper.h"

/**
 * @brief logger ID
 *
 */
extern logger_id_t policy_updater_logger_id;

/**
 * @brief init policy_updater logger
 *
 * @param[in] level A level of the logger
 *
 */
void logger_init_policy_updater(logger_level_t level);

/**
 * @brief cleanup policy_updater logger
 *
 */
void logger_destroy_policy_updater();

#endif  // POLICY_UPDATER_LOGGER_H
