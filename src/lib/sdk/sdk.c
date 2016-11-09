#include "comm.h"
#include "lock.h"
#include "redo.h"
#include "syscall.h"
#include "sdk.h"
#include "sdk_mesg.h"

static int sdk_lock_server(const sdk_conf_t *conf);
static int sdk_creat_cmd_usck(sdk_cntx_t *ctx);

/******************************************************************************
 **函数名称: sdk_creat_workers
 **功    能: 创建工作线程线程池
 **输入参数:
 **     ctx: 全局对象
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述:
 **注意事项:
 **作    者: # Qifeng.zou # 2015.08.19 #
 ******************************************************************************/
static int sdk_creat_workers(sdk_cntx_t *ctx)
{
    int idx;
    sdk_worker_t *worker;
    sdk_conf_t *conf = &ctx->conf;

    /* > 创建对象 */
    worker = (sdk_worker_t *)calloc(conf->work_thd_num, sizeof(sdk_worker_t));
    if (NULL == worker) {
        return SDK_ERR;
    }

    /* > 创建线程池 */
    ctx->worktp = thread_pool_init(conf->work_thd_num, NULL, (void *)worker);
    if (NULL == ctx->worktp) {
        free(worker);
        return SDK_ERR;
    }

    /* > 初始化线程 */
    for (idx=0; idx<conf->work_thd_num; ++idx) {
        if (sdk_worker_init(ctx, worker+idx, idx)) {
            free(worker);
            thread_pool_destroy(ctx->worktp);
            return SDK_ERR;
        }
    }

    return SDK_OK;
}

/******************************************************************************
 **函数名称: sdk_creat_sends
 **功    能: 创建发送线程线程池
 **输入参数:
 **     ctx: 全局对象
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述:
 **注意事项:
 **作    者: # Qifeng.zou # 2015.08.19 #
 ******************************************************************************/
static int sdk_creat_sends(sdk_cntx_t *ctx)
{
    int idx;
    sdk_ssvr_t *ssvr;

    /* > 创建对象 */
    ssvr = (sdk_ssvr_t *)calloc(SDK_SSVR_NUM, sizeof(sdk_ssvr_t));
    if (NULL == ssvr) {
        log_error(ctx->log, "errmsg:[%d] %s!", errno, strerror(errno));
        return SDK_ERR;
    }

    /* > 创建线程池 */
    ctx->sendtp = thread_pool_init(SDK_SSVR_NUM, NULL, (void *)ssvr);
    if (NULL == ctx->sendtp) {
        log_error(ctx->log, "Initialize thread pool failed!");
        free(ssvr);
        return SDK_ERR;
    }

    /* > 初始化线程 */
    for (idx=0; idx<SDK_SSVR_NUM; ++idx) {
        if (sdk_ssvr_init(ctx, ssvr+idx, idx)) {
            log_fatal(ctx->log, "Initialize send thread failed!");
            free(ssvr);
            thread_pool_destroy(ctx->sendtp);
            return SDK_ERR;
        }
    }

    return SDK_OK;
}

/******************************************************************************
 **函数名称: sdk_creat_recvq
 **功    能: 创建接收队列
 **输入参数:
 **     ctx: 全局对象
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述:
 **注意事项:
 **作    者: # Qifeng.zou # 2015.06.04 #
 ******************************************************************************/
static int sdk_creat_recvq(sdk_cntx_t *ctx)
{
    int idx;
    sdk_conf_t *conf = &ctx->conf;

    /* > 创建队列对象 */
    ctx->recvq = (queue_t **)calloc(SDK_SSVR_NUM, sizeof(queue_t *));
    if (NULL == ctx->recvq) {
        log_error(ctx->log, "errmsg:[%d] %s!", errno, strerror(errno));
        return SDK_ERR;
    }

    /* > 创建接收队列 */
    for (idx=0; idx<conf->work_thd_num; ++idx) {
        ctx->recvq[idx] = queue_creat(conf->recvq.max, conf->recvq.size);
        if (NULL == ctx->recvq[idx]) {
            log_error(ctx->log, "Create recvq failed!");
            return SDK_ERR;
        }
    }

    return SDK_OK;
}

/******************************************************************************
 **函数名称: sdk_creat_sendq
 **功    能: 创建发送线程的发送队列
 **输入参数:
 **     ctx: 发送对象
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述:
 **注意事项:
 **作    者: # Qifeng.zou # 2016.01.01 22:32:21 #
 ******************************************************************************/
static int sdk_creat_sendq(sdk_cntx_t *ctx)
{
    int idx;
    sdk_conf_t *conf = &ctx->conf;

    /* > 创建队列对象 */
    ctx->sendq = (queue_t **)calloc(SDK_SSVR_NUM, sizeof(queue_t *));
    if (NULL == ctx->sendq) {
        log_error(ctx->log, "errmsg:[%d] %s!", errno, strerror(errno));
        return SDK_ERR;
    }

    /* > 创建发送队列 */
    for (idx=0; idx<SDK_SSVR_NUM; ++idx) {
        ctx->sendq[idx] = queue_creat(conf->sendq.max, conf->sendq.size);
        if (NULL == ctx->sendq[idx]) {
            log_error(ctx->log, "Create send queue failed!");
            return SDK_ERR;
        }
    }

    return SDK_OK;
}

/******************************************************************************
 **函数名称: sdk_init
 **功    能: 初始化发送端
 **输入参数:
 **     conf: 配置信息
 **     log: 日志对象
 **输出参数: NONE
 **返    回: 全局对象
 **实现描述:
 **注意事项:
 **作    者: # Qifeng.zou # 2015.05.19 #
 ******************************************************************************/
sdk_cntx_t *sdk_init(const sdk_conf_t *conf)
{
    sdk_cntx_t *ctx;
    log_cycle_t *log;

    log = log_init(conf->log_level, conf->log_path);
    if (NULL == log) {
        fprintf(stderr, "errmsg:[%d] %s!", errno, strerror(errno));
        return NULL;
    }

    /* > 创建对象 */
    ctx = (sdk_cntx_t *)calloc(1, sizeof(sdk_cntx_t));
    if (NULL == ctx) {
        log_fatal(log, "errmsg:[%d] %s!", errno, strerror(errno));
        return NULL;
    }

    ctx->log = log;

    /* > 加载配置信息 */
    memcpy(&ctx->conf, conf, sizeof(sdk_conf_t));

    do {
        /* > 锁住指定文件 */
        if (sdk_lock_server(conf)) {
            log_fatal(log, "Lock proxy server failed! errmsg:[%d] %s", errno, strerror(errno));
            break;
        }

        /* > 创建处理映射表 */
        ctx->reg = avl_creat(NULL, (cmp_cb_t)sdk_reg_cmp_cb);
        if (NULL == ctx->reg) {
            log_fatal(log, "Create register map failed!");
            break;
        }
        /* > 创建通信套接字 */
        if (sdk_creat_cmd_usck(ctx)) {
            log_fatal(log, "Create cmd socket failed!");
            break;
        }

        /* > 创建接收队列 */
        if (sdk_creat_recvq(ctx)) {
            log_fatal(log, "Create recv-queue failed!");
            break;
        }

        /* > 创建发送队列 */
        if (sdk_creat_sendq(ctx)) {
            log_fatal(log, "Create send queue failed!");
            break;
        }

        /* > 创建工作线程池 */
        if (sdk_creat_workers(ctx)) {
            log_fatal(ctx->log, "Create work thread pool failed!");
            break;
        }

        /* > 创建发送线程池 */
        if (sdk_creat_sends(ctx)) {
            log_fatal(ctx->log, "Create send thread pool failed!");
            break;
        }

        return ctx;
    } while(0);

    free(ctx);
    return NULL;
}

/******************************************************************************
 **函数名称: sdk_launch
 **功    能: 启动发送端
 **输入参数:
 **     ctx: 全局信息
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述:
 **     1. 创建工作线程池
 **     2. 创建发送线程池
 **注意事项:
 **作    者: # Qifeng.zou # 2015.01.14 #
 ******************************************************************************/
int sdk_launch(sdk_cntx_t *ctx)
{
    int idx;
    sdk_conf_t *conf = &ctx->conf;

    /* > 注册Worker线程回调 */
    for (idx=0; idx<conf->work_thd_num; ++idx) {
        thread_pool_add_worker(ctx->worktp, sdk_worker_routine, ctx);
    }

    /* > 注册Send线程回调 */
    for (idx=0; idx<SDK_SSVR_NUM; ++idx) {
        thread_pool_add_worker(ctx->sendtp, sdk_ssvr_routine, ctx);
    }

    return SDK_OK;
}

/******************************************************************************
 **函数名称: sdk_register
 **功    能: 消息处理的注册接口
 **输入参数:
 **     ctx: 全局对象
 **     type: 扩展消息类型 Range:(0 ~ SDK_TYPE_MAX)
 **     proc: 回调函数
 **     param: 附加参数
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述:
 **注意事项:
 **     1. 只能用于注册处理扩展数据类型的处理
 **     2. 不允许重复注册
 **作    者: # Qifeng.zou # 2015.05.19 #
 ******************************************************************************/
int sdk_register(sdk_cntx_t *ctx, int cmd, sdk_reg_cb_t proc, void *param)
{
    sdk_reg_t *item;

    item = (sdk_reg_t *)calloc(1, sizeof(sdk_reg_t));
    if (NULL == item) {
        log_error(ctx->log, "errmsg:[%d] %s!", errno, strerror(errno));
        return -1;
    }

    item->cmd = cmd;
    item->proc = proc;
    item->param = param;

    if (avl_insert(ctx->reg, item)) {
        log_error(ctx->log, "Register maybe repeat! cmd:%d!", cmd);
        free(item);
        return SDK_ERR_REPEAT_REG;
    }

    return SDK_OK;
}

/******************************************************************************
 **函数名称: sdk_creat_cmd_usck
 **功    能: 创建命令套接字
 **输入参数:
 **     ctx: 上下文信息
 **     idx: 目标队列序号
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述:
 **注意事项:
 **作    者: # Qifeng.zou # 2015.01.14 #
 ******************************************************************************/
static int sdk_creat_cmd_usck(sdk_cntx_t *ctx)
{
    char path[FILE_NAME_MAX_LEN];

    sdk_comm_usck_path(&ctx->conf, path);

    spin_lock_init(&ctx->cmd_sck_lck);
    ctx->cmd_sck_id = unix_udp_creat(path);
    if (ctx->cmd_sck_id < 0) {
        log_error(ctx->log, "errmsg:[%d] %s! path:%s", errno, strerror(errno), path);
        return SDK_ERR;
    }

    return SDK_OK;
}

/******************************************************************************
 **函数名称: sdk_lock_server
 **功    能: 锁住指定路径(注: 防止路径和结点ID相同的配置)
 **输入参数:
 **     conf: 配置信息
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述:
 **注意事项: 文件描述符可不用关闭
 **作    者: # Qifeng.zou # 2016.05.02 21:14:39 #
 ******************************************************************************/
static int sdk_lock_server(const sdk_conf_t *conf)
{
    int fd;
    char path[FILE_NAME_MAX_LEN];

    sdk_lock_path(conf, path);

    Mkdir2(path, DIR_MODE);

    fd = Open(path, O_CREAT|O_RDWR, OPEN_MODE);
    if (fd < 0) {
        return -1;
    }

    if (proc_try_wrlock(fd)) {
        close(fd);
        return -1;
    }
    return 0;
}

/******************************************************************************
 **函数名称: sdk_cli_cmd_send_req
 **功    能: 通知Send服务线程
 **输入参数:
 **     ctx: 上下文信息
 **     idx: 发送服务ID
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述:
 **注意事项:
 **作    者: # Qifeng.zou # 2015.01.14 #
 ******************************************************************************/
static int sdk_cli_cmd_send_req(sdk_cntx_t *ctx, int idx)
{
    sdk_cmd_t cmd;
    char path[FILE_NAME_MAX_LEN];
    sdk_conf_t *conf = &ctx->conf;

    memset(&cmd, 0, sizeof(cmd));

    cmd.type = SDK_CMD_SEND_ALL;
    sdk_ssvr_usck_path(conf, path, idx);

    if (spin_trylock(&ctx->cmd_sck_lck)) {
        log_debug(ctx->log, "Try lock failed!");
        return SDK_OK;
    }

    if (unix_udp_send(ctx->cmd_sck_id, path, &cmd, sizeof(cmd)) < 0) {
        spin_unlock(&ctx->cmd_sck_lck);
        if (EAGAIN != errno) {
            log_debug(ctx->log, "errmsg:[%d] %s! path:%s", errno, strerror(errno), path);
        }
        return SDK_ERR;
    }

    spin_unlock(&ctx->cmd_sck_lck);

    return SDK_OK;
}

/******************************************************************************
 **函数名称: sdk_async_send
 **功    能: 发送指定数据(对外接口)
 **输入参数:
 **     ctx: 上下文信息
 **     type: 数据类型
 **     nid: 源结点ID
 **     data: 数据地址
 **     size: 数据长度
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 将数据按照约定格式放入队列中
 **     {
 **         uint16_t cmd;       // 协议类型，命令ID,定义规则：奇数是请求，偶数是应答
 **         uint16_t flag;      // 校验码
 **         uint32_t len;       // 消息body长度
 **         uint64_t from;      // 发送者连接sessionID
 **         uint64_t to;        // 目标者sessionID
 **         uint32_t seq;       // 消息序列号
 **         uint16_t mid;       // 内部通信时的模块ID
 **         uint16_t version;   // 版本
 **         uint32_t rsv1;      // 预留字段
 **         uint32_t rsv2;      // 预留字段
 **     }
 **注意事项:
 **     1. 只能用于发送自定义数据类型, 而不能用于系统数据类型
 **     2. 不用关注变量num在多线程中的值, 因其不影响安全性
 **     3. 只要SSVR未处于未上线成功的状态, 则认为联网失败.
 **作    者: # Qifeng.zou # 2015.01.14 #
 ******************************************************************************/
int sdk_async_send(sdk_cntx_t *ctx, int cmd, uint64_t to, const void *data, size_t size)
{
    int idx;
    void *addr;
    mesg_header_t *head;
    static uint8_t num = 0; // 无需加锁

    /* > 选择发送队列 */
    idx = (num++) % SDK_SSVR_NUM;

    addr = queue_malloc(ctx->sendq[idx], sizeof(mesg_header_t)+size);
    if (NULL == addr) {
        log_error(ctx->log, "Alloc from queue failed! size:%d/%d",
                size+sizeof(mesg_header_t), queue_size(ctx->sendq[idx]));
        return SDK_ERR;
    }

    /* > 设置发送数据 */
    head = (mesg_header_t *)addr;

    head->cmd = cmd;
    head->flag = 0;
    head->len = size;
    head->from = ctx->sid;
    head->to = to;

    memcpy(head+1, data, size);

    log_debug(ctx->log, "rq:%p Head type:%d sid:%d length:%d flag:%d!",
          ctx->sendq[idx]->ring, head->cmd, head->from, head->len, head->flag);

    /* > 放入发送队列 */
    if (queue_push(ctx->sendq[idx], addr)) {
        log_error(ctx->log, "Push into shmq failed!");
        queue_dealloc(ctx->sendq[idx], addr);
        return SDK_ERR;
    }

    /* > 通知发送线程 */
    sdk_cli_cmd_send_req(ctx, idx);

    return SDK_OK;
}

/******************************************************************************
 **函数名称: sdk_network_switch
 **功    能: 网络状态切换(对外接口)
 **输入参数:
 **     ctx: 上下文信息
 **     status: 状态(0:关闭 1:开启)
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 发送命令通知接收线程
 **注意事项:
 **作    者: # Qifeng.zou # 2016.11.07 09:43:21 #
 ******************************************************************************/
int sdk_network_switch(sdk_cntx_t *ctx, int status)
{
    sdk_cmd_t cmd;
    int ret, idx = 0;
    char path[FILE_NAME_MAX_LEN];
    sdk_conf_t *conf = &ctx->conf;

    memset(&cmd, 0, sizeof(cmd));

    cmd.type = status? SDK_CMD_NETWORK_ON : SDK_CMD_NETWORK_OFF;
    sdk_ssvr_usck_path(conf, path, idx);

    if (spin_trylock(&ctx->cmd_sck_lck)) {
        log_debug(ctx->log, "Try lock failed!");
        return SDK_OK;
    }

    ret = unix_udp_send(ctx->cmd_sck_id, path, &cmd, sizeof(cmd));
    spin_unlock(&ctx->cmd_sck_lck);
    if (ret < 0) {
        if (EAGAIN != errno) {
            log_debug(ctx->log, "errmsg:[%d] %s! path:%s", errno, strerror(errno), path);
        }
        return SDK_ERR;
    }

    return SDK_OK;

}
