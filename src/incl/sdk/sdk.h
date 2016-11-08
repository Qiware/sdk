#if !defined(__SDK_PROXY_H__)
#define __SDK_PROXY_H__

#include "log.h"
#include "spinlock.h"
#include "sdk_ssvr.h"

#define SDK_DEV_ID_LEN      (64)
#define SDK_CLIENT_ID_LEN   (64)
#define SDK_APP_KEY_LEN     (128)

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

    int send_thd_num;                   /* 发送线程数 */
    int work_thd_num;                   /* 工作线程数 */

    size_t recv_buff_size;              /* 接收缓存大小 */

    queue_conf_t sendq;                 /* 发送队列配置 */
    queue_conf_t recvq;                 /* 接收队列配置 */
} sdk_conf_t;

/* 全局信息 */
typedef struct
{
    uint64_t sid;                       /* 会话SID */
    sdk_conf_t conf;                    /* 配置信息 */
    log_cycle_t *log;                   /* 日志对象 */

    int cmd_sck_id;                     /* 命令套接字 */
    spinlock_t cmd_sck_lck;             /* 命令套接字锁 */

    thread_pool_t *sendtp;              /* 发送线程池 */
    thread_pool_t *worktp;              /* 工作线程池 */

    avl_tree_t *reg;                    /* 回调注册对象(注: 存储sdk_reg_t数据) */
    queue_t **recvq;                    /* 接收队列(数组长度与send_thd_num一致) */
    queue_t **sendq;                    /* 发送缓存(数组长度与send_thd_num一致) */
} sdk_cntx_t;

/* 内部接口 */
int sdk_ssvr_init(sdk_cntx_t *ctx, sdk_ssvr_t *ssvr, int tidx);
void *sdk_ssvr_routine(void *_ctx);

int sdk_worker_init(sdk_cntx_t *ctx, sdk_worker_t *worker, int tidx);
void *sdk_worker_routine(void *_ctx);

sdk_worker_t *sdk_worker_get_by_idx(sdk_cntx_t *ctx, int idx);

int sdk_mesg_send_ping_req(sdk_cntx_t *ctx, sdk_ssvr_t *ssvr);
int sdk_mesg_send_online_req(sdk_cntx_t *ctx, sdk_ssvr_t *ssvr);

int sdk_mesg_pong_handler(sdk_cntx_t *ctx, sdk_ssvr_t *ssvr, sdk_sct_t *sck);
int sdk_mesg_ping_handler(sdk_cntx_t *ctx, sdk_ssvr_t *ssvr, sdk_sct_t *sck);
int sdk_mesg_online_ack_handler(sdk_cntx_t *ctx, sdk_ssvr_t *ssvr, sdk_sct_t *sck, void *addr);

/* 对外接口 */
sdk_cntx_t *sdk_init(const sdk_conf_t *conf);
int sdk_launch(sdk_cntx_t *ctx);
int sdk_register(sdk_cntx_t *ctx, int cmd, sdk_reg_cb_t proc, void *args);
int sdk_async_send(sdk_cntx_t *ctx, int cmd, uint64_t to, const void *data, size_t size);
int sdk_network_switch(sdk_cntx_t *ctx, int status);

#endif /*__SDK_PROXY_H__*/
