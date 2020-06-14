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

/****************************************************************************
 * \project IOTA Access
 * \file demo_02_plugin.h
 * \brief
 * Resolver plugin for CANOpen demo using relay board connected directly to
 * rpi3.
 *
 * @Author Djordje Golubovic
 *
 * \notes
 *
 * \history
 * 04.03.2020. Initial version.
 ****************************************************************************/

#ifndef __DEMO_02_PLUGIN_H__
#define __DEMO_02_PLUGIN_H__

#include "resolver.h"
#include "wallet.h"

/**
 * @fn void Demo02Plugin_initializer(resolver_plugin_t* action_set)
 * @brief resolver plugin initializer callback
 */
void Demo02Plugin_initializer(resolver_plugin_t* action_set, wallet_ctx_t* wallet_ctx);

/**
 * @fn void Demo02Plugin_terminizer()
 * @brief resolver plugin terminizer callback
 */
void Demo02Plugin_terminizer();

#endif
