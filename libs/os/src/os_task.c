/**
 * Copyright (c) 2015 Runtime Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include "os/os.h"

#include <string.h>

uint8_t g_task_id;

STAILQ_HEAD(, os_task) g_os_task_list = STAILQ_HEAD_INITIALIZER(g_os_task_list);

static void
_clear_stack(os_stack_t *stack_bottom, int size) 
{
    int i; 

    for (i = 0; i < size; i++) {
        stack_bottom[i] = OS_STACK_PATTERN;
    }
}

static inline uint8_t 
os_task_next_id(void)
{
    uint8_t rc;
    os_sr_t sr;

    OS_ENTER_CRITICAL(sr);
    rc = g_task_id;
    g_task_id++;
    OS_EXIT_CRITICAL(sr);

    return (rc);
}

uint8_t 
os_task_count(void)
{
    return (g_task_id);
}

int 
os_task_sanity_checkin(struct os_task *t)
{
    int rc; 

    if (t == NULL) {
        t = os_sched_get_current_task();
    }

    rc = os_sanity_check_reset(&t->t_sanity_check);
    if (rc != OS_OK) {
        goto err;
    }

    return (0);
err:
    return (rc);
}



int 
os_task_init(struct os_task *t, char *name, os_task_func_t func, void *arg, 
        uint8_t prio, os_time_t sanity_itvl, os_stack_t *stack_bottom, 
        uint16_t stack_size)
{
    struct os_sanity_check *sc; 
    int rc; 

    memset(t, 0, sizeof(*t));

    t->t_func = func;
    t->t_arg = arg;
    
    t->t_taskid = os_task_next_id(); 
    t->t_prio = prio;

    t->t_state = OS_TASK_READY;
    t->t_name = name;
    t->t_next_wakeup = 0; 

    rc = os_sanity_check_init(&t->t_sanity_check); 
    if (rc != OS_OK) {
        goto err;
    }

    if (sanity_itvl != OS_WAIT_FOREVER) {
        sc = (struct os_sanity_check *) &t->t_sanity_check; 
        sc->sc_checkin_itvl = sanity_itvl;
        
        rc = os_sanity_check_register(sc);
        if (rc != OS_OK) {
            goto err;
        }
    }

    _clear_stack(stack_bottom, stack_size);
    t->t_stackptr = os_arch_task_stack_init(t, &stack_bottom[stack_size], 
            stack_size);
    t->t_stackbase = stack_bottom;
    t->t_stacksize = stack_size;

    /* insert this task into the task list */
    STAILQ_INSERT_TAIL(&g_os_task_list, t, t_os_task_list);

    /* insert this task into the scheduler list */
    rc = os_sched_insert(t);
    if (rc != OS_OK) {
        goto err;
    }


    return (0);
err:
    return (rc);
}

struct os_task *
os_task_info_get_next(const struct os_task *prev, struct os_task_info *oti)
{
    struct os_task *next;
    os_sr_t sr;

    if (prev != NULL) {
        next = STAILQ_NEXT(prev, t_os_task_list);
    } else {
        next = STAILQ_FIRST(&g_os_task_list);
    }

    if (next == NULL) {
        return (next);
    }

    /* Otherwise, copy OS task information into the OTI structure, and 
     * return 1, which means continue
     */
    OS_ENTER_CRITICAL(sr);
    oti->oti_prio = next->t_prio;
    oti->oti_taskid = next->t_taskid;
    oti->oti_state = next->t_state;
    oti->oti_stkusage = (uint16_t) (next->t_stackptr - next->t_stackbase);
    oti->oti_stksize = next->t_stacksize;
    oti->oti_cswcnt = next->t_ctx_sw_cnt;
    oti->oti_runtime = next->t_run_time;
    oti->oti_last_checkin = next->t_sanity_check.sc_checkin_last;
    oti->oti_next_checkin = next->t_sanity_check.sc_checkin_last + 
        next->t_sanity_check.sc_checkin_itvl;
    OS_EXIT_CRITICAL(sr);
    strncpy(oti->oti_name, next->t_name, sizeof(oti->oti_name));

    return (next);
}

