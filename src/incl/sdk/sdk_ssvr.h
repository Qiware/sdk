#if !defined(__SDK_SSVR_H__)
#define __SDK_SSVR_H__

#include "log.h"
#include "list.h"
#include "avl_tree.h"
#include "sdk_comm.h"
#include "thread_pool.h"

/* COMM的UNIX-UDP路径 */
#define sdk_comm_usck_path(conf, _path) \
    snprintf(_path, sizeof(_path), "%s/rtmq/%d_comm.usck", (conf)->path, (conf)->nid)
/* SSVR线程的UNIX-UDP路径 */
#define sdk_ssvr_usck_path(conf, _path, id) \
    snprintf(_path, sizeof(_path), "%s/rtmq/%d_ssvr_%d.usck", (conf)->path, (conf)->nid, id+1)
/* WORKER线程的UNIX-UDP路径 */
#define sdk_worker_usck_path(conf, _path, id) \
    snprintf(_path, sizeof(_path), "%s/rtmq/%d_swrk_%d.usck", (conf)->path, (conf)->nid, id+1)
/* 加锁路径 */
#define sdk_lock_path(conf, _path) \
    snprintf(_path, sizeof(_path), "%s/rtmq/%d.lock", (conf)->path, (conf)->nid)

/* 套接字信息 */
typedef struct
{
    int fd;                             /* 套接字ID */
    time_t wrtm;                        /* 最近写入操作时间 */
    time_t rdtm;                        /* 最近读取操作时间 */

#define SDK_KPALIVE_STAT_UNKNOWN   (0) /* 未知状态 */
#define SDK_KPALIVE_STAT_SENT      (1) /* 已发送保活 */
#define SDK_KPALIVE_STAT_SUCC      (2) /* 保活成功 */
    int kpalive;                        /* 保活状态(0:未知 1:已发送 2:保活成功) */
    int kpalive_times;                  /* 保活次数 */
    list_t *mesg_list;                  /* 发送链表 */

    sdk_snap_t recv;                   /* 接收快照 */
    wiov_t send;                       /* 发送信息 */
} sdk_sct_t;

#define sdk_set_kpalive_stat(sck, _stat) (sck)->kpalive = (_stat)

/* SND线程上下文 */
typedef struct
{
    int id;                             /* 对象ID */
    void *ctx;                          /* 存储sdk_t对象 */
    log_cycle_t *log;                   /* 日志对象 */
    queue_t *sendq;                     /* 发送缓存 */

    int cmd_sck_id;                     /* 命令通信套接字ID */
    sdk_sct_t sck;               /* 发送套接字 */

    int max;                            /* 套接字最大值 */
    fd_set rset;                        /* 读集合 */
    fd_set wset;                        /* 写集合 */

    /* 统计信息 */
    uint64_t recv_total;                /* 获取的数据总条数 */
    uint64_t err_total;                 /* 错误的数据条数 */
    uint64_t drop_total;                /* 丢弃的数据条数 */
} sdk_ssvr_t;

#endif /*__SDK_SSVR_H__*/
