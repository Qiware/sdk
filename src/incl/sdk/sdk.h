#if !defined(__SDK_PROXY_H__)
#define __SDK_PROXY_H__

#include "log.h"
#include "spinlock.h"
#include "sdk_ssvr.h"

#define SDK_DEV_ID_LEN      (64)
#define SDK_CLIENT_ID_LEN   (64)
#define SDK_APP_KEY_LEN     (128)
#define SDK_SSVR_NUM        (1)         /* 发送协程个数 */

/* 发送单元 */
typedef struct
{
    int cmd;                            /* 命令类型 */
    time_t ttl;                         /* 超时时间 */
    void *data;                         /* 发送的数据 */
} sdk_send_item_t;

/* 配置信息 */
typedef struct
{
    int nid;                            /* 设备ID: 唯一值 */
    char path[FILE_LINE_MAX_LEN];       /* 工作路径 */

    uint64_t sessionid;                 /* 会话ID(备选) */
    char devid[SDK_DEV_ID_LEN];         /* 设备ID */
    char clientid[SDK_CLIENT_ID_LEN];   /* 客户端ID(必须提供) */
    char appkey[SDK_APP_KEY_LEN];       /* APP KEY */
    char version[SDK_CLIENT_ID_LEN];    /* 客户端自身版本号(留做统计用) */

    int log_level;                      /* 日志级别 */
    char log_path[FILE_LINE_MAX_LEN];   /* 日志路径(路径+文件名) */

    char httpsvr[IP_ADDR_MAX_LEN];      /* HTTP端(IP+端口/域名) */

    int work_thd_num;                   /* 工作线程数 */

    size_t recv_buff_size;              /* 接收缓存大小 */

    int sendq_len;                      /* 发送队列配置 */
    int recvq_len;                      /* 接收队列配置 */
} sdk_conf_t;

/* 全局信息 */
typedef struct
{
    uint64_t sid;                       /* 会话SID */
    uint64_t data_seq;                  /* 发送数据的序列号 */
    sdk_conf_t conf;                    /* 配置信息 */
    log_cycle_t *log;                   /* 日志对象 */

    int cmd_sck_id;                     /* 命令套接字 */
    spinlock_t cmd_sck_lck;             /* 命令套接字锁 */

    thread_pool_t *sendtp;              /* 发送线程池 */
    thread_pool_t *worktp;              /* 工作线程池 */

    avl_tree_t *reg;                    /* 回调注册对象(注: 存储sdk_reg_t数据) */

    sdk_queue_t recvq;                  /* 接收队列 */
    sdk_queue_t sendq;                  /* 发送队列 */
} sdk_cntx_t;

/* 内部接口 */
int sdk_ssvr_init(sdk_cntx_t *ctx, sdk_ssvr_t *ssvr, int tidx);
void *sdk_ssvr_routine(void *_ctx);

int sdk_worker_init(sdk_cntx_t *ctx, sdk_worker_t *worker, int tidx);
void *sdk_worker_routine(void *_ctx);

sdk_worker_t *sdk_worker_get_by_idx(sdk_cntx_t *ctx, int idx);

int sdk_mesg_send_ping_req(sdk_cntx_t *ctx, sdk_ssvr_t *ssvr);
int sdk_mesg_send_online_req(sdk_cntx_t *ctx, sdk_ssvr_t *ssvr);
int sdk_mesg_send_sync_req(sdk_cntx_t *ctx, sdk_ssvr_t *ssvr, sdk_sct_t *sck);

int sdk_mesg_pong_handler(sdk_cntx_t *ctx, sdk_ssvr_t *ssvr, sdk_sct_t *sck);
int sdk_mesg_ping_handler(sdk_cntx_t *ctx, sdk_ssvr_t *ssvr, sdk_sct_t *sck);
int sdk_mesg_online_ack_handler(sdk_cntx_t *ctx, sdk_ssvr_t *ssvr, sdk_sct_t *sck, void *addr);

int sdk_queue_init(sdk_queue_t *q);
int sdk_queue_length(sdk_queue_t *q);
int sdk_queue_rpush(sdk_queue_t *q, void *addr);
void *sdk_queue_lpop(sdk_queue_t *q);
bool sdk_queue_empty(sdk_queue_t *q);

/* 发送结果回调
 *  data: 被发送的数据
 *  len: 数据长度
 *  reason: 回调原因(0:发送成功 -1:发送失败 -2:超时未发送 -3:发送后超时未应答)
 * 作用: 发送成功还是失败都会调用此回调 */
typedef int (*sdk_send_cb_t)(void *data, size_t size, int reason);

/* 对外接口 */
sdk_cntx_t *sdk_init(const sdk_conf_t *conf);
int sdk_launch(sdk_cntx_t *ctx);
int sdk_register(sdk_cntx_t *ctx, int cmd, sdk_reg_cb_t proc, void *args);
int sdk_async_send(sdk_cntx_t *ctx, int cmd, uint64_t to, const void *data, size_t size, int timeout, sdk_send_cb_t cb);
int sdk_network_switch(sdk_cntx_t *ctx, int status);

#endif /*__SDK_PROXY_H__*/
