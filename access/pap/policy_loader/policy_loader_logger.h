/*
 * This file is part of the Frost distribution
 * (https://github.com/xainag/frost)
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
 * \file policy_loader_logger.h
 * \brief
 * Logger for PAP policy loader
 *
 * @Author Bernardo Araujo
 *
 * \notes
 *
 * \history
 * 17.07.2020. Initial version.
 ****************************************************************************/


#ifndef POLICY_LOADER_LOGGER_H
#define POLICY_LOADER_LOGGER_H

#include "utils/logger_helper.h"

/**
 * @brief logger ID
 *
 */
extern logger_id_t pap_logger_id;

/**
 * @brief init Policy Loader logger
 *
 * @param[in] level A level of the logger
 *
 */
void logger_init_policy_loader(logger_level_t level);

/**
 * @brief cleanup Policy Loader logger
 *
 */
void logger_destroy_policy_loader();

#endif  // POLICY_LOADER_LOGGER_H
