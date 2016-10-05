/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef _NEWTMGR_H_
#define _NEWTMGR_H_

#include <json/json.h>
#include <inttypes.h>
#include <os/os.h>

#ifdef __cplusplus
extern "C" {
#endif

/* MTU for newtmgr responses */
#define NMGR_MAX_MTU 1024

#ifndef STR
/* Stringification of constants */
#define STR(x) #x
#endif

/* First 64 groups are reserved for system level newtmgr commands.
 * Per-user commands are then defined after group 64.
 */
#define NMGR_GROUP_ID_DEFAULT   (0)
#define NMGR_GROUP_ID_IMAGE     (1)
#define NMGR_GROUP_ID_STATS     (2)
#define NMGR_GROUP_ID_CONFIG    (3)
#define NMGR_GROUP_ID_LOGS      (4)
#define NMGR_GROUP_ID_CRASH     (5)
#define NMGR_GROUP_ID_SPLIT     (6)
#define NMGR_GROUP_ID_PERUSER   (64)

#define NMGR_OP_READ            (0)
#define NMGR_OP_READ_RSP        (1)
#define NMGR_OP_WRITE           (2)
#define NMGR_OP_WRITE_RSP       (3)


/**
 * Newtmgr JSON error codes
 */
#define NMGR_ERR_EOK      (0)
#define NMGR_ERR_EUNKNOWN (1)
#define NMGR_ERR_ENOMEM   (2)
#define NMGR_ERR_EINVAL   (3)
#define NMGR_ERR_ETIMEOUT (4)
#define NMGR_ERR_ENOENT   (5)
#define NMGR_ERR_EPERUSER (256)

struct nmgr_hdr {
    uint8_t  nh_op;             /* NMGR_OP_XXX */
    uint8_t  nh_flags;
    uint16_t nh_len;            /* length of the payload */
    uint16_t nh_group;          /* NMGR_GROUP_XXX */
    uint8_t  nh_seq;            /* sequence number */
    uint8_t  nh_id;             /* message ID within group */
};

struct nmgr_jbuf {
    /* json_buffer must be first element in the structure */
    struct json_buffer njb_buf;
    struct json_encoder njb_enc;
    char *njb_in;
    uint16_t njb_in_off;
    uint16_t njb_in_end;
    char *njb_out;
    uint16_t njb_out_off;
    uint16_t njb_out_end;
};
int nmgr_jbuf_setoerr(struct nmgr_jbuf *njb, int errcode);

typedef int (*nmgr_handler_func_t)(struct nmgr_jbuf *);

#define NMGR_HANDLER_FUNC(__name)                                           \
    int __name(struct nmgr_hdr *nmr, struct os_mbuf *req, uint16_t srcoff,  \
            struct os_mbuf *rsp)

struct nmgr_handler {
    nmgr_handler_func_t nh_read;
    nmgr_handler_func_t nh_write;
};

struct nmgr_group {
    struct nmgr_handler *ng_handlers;
    uint16_t ng_handlers_count;
    uint16_t ng_group_id;
    STAILQ_ENTRY(nmgr_group) ng_next;
};

#define NMGR_GROUP_SET_HANDLERS(__group, __handlers)       \
    (__group)->ng_handlers = (__handlers);                 \
    (__group)->ng_handlers_count = (sizeof((__handlers)) / \
            sizeof(struct nmgr_handler));

int nmgr_oic_init(uint8_t, os_stack_t *, uint16_t);
int nmgr_group_register(struct nmgr_group *group);

#ifdef __cplusplus
}
#endif

#endif /* _NETMGR_H */