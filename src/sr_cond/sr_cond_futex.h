/**
 * @file sr_cond_futex.h
 * @author Michal Vasko <mvasko@cesnet.cz>
 * @brief Conditional variable futex implementation header.
 *
 * @copyright
 * Copyright (c) 2022 Deutsche Telekom AG.
 * Copyright (c) 2022 CESNET, z.s.p.o.
 *
 * This source code is licensed under BSD 3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef _SR_COND_FUTEX_H
#define _SR_COND_FUTEX_H

#include <pthread.h>
#include <stdint.h>

#include "compat.h"
#include "sysrepo_types.h"

/**
 * @brief Condition variable futex implementation.
 */
typedef struct {
    uint32_t futex;             /**< futex used for waiting and signalling idle/ready */
    uint32_t waiters;           /**< number of waiters for the futex */
    pthread_mutex_t wait_lock;  /**< lock held while the futex is being waited on */
} sr_cond_t;

/**
 * @brief Wrapper for pthread_cond_init().
 *
 * @param[out] cond Condition variable to initialize.
 * @param[in] shared Whether the condition will be shared among processes.
 * @param[in] robust Whether the condition must be robust or not.
 * @return err_info, NULL on successq.
 */
sr_error_info_t *sr_cond_init(sr_cond_t *cond, int shared, int robust);

/**
 * @brief Wrapper for pthread_cond_destroy().
 *
 * @param[in] cond Conditional variable to destroy.
 */
void sr_cond_destroy(sr_cond_t *cond);

/**
 * @brief Wrapper for pthread_cond_wait().
 *
 * @param[in] cond Condition variable to wait on.
 * @param[in] mutex Conditional variable mutex.
 * @return errno
 */
int sr_cond_wait(sr_cond_t *cond, pthread_mutex_t *mutex);

/**
 * @brief Wrapper for pthread_cond_timedwait().
 *
 * @param[in] cond Condition variable to wait on.
 * @param[in] mutex Conditional variable mutex.
 * @param[in] timeout_ms Timeout to wait for the condition.
 * @return errno
 */
int sr_cond_timedwait(sr_cond_t *cond, pthread_mutex_t *mutex, uint32_t timeout_ms);

/**
 * @brief Wrapper for pthread_cond_broadcast().
 *
 * @param[in] cond Condition variable to broadcast on.
 */
void sr_cond_broadcast(sr_cond_t *cond);

/**
 * @brief Make conditional variable consistent after it was being used by a crashed process.
 *
 * @param[in] cond Conditional variable to make consistent.
 */
void sr_cond_consistent(sr_cond_t *cond);

#endif /* _SR_COND_FUTEX_H */
