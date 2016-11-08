#include "str.h"
#include "sdk.h"
#include "redo.h"
#include "queue.h"
#include "sdk_mesg.h"
#include "sdk_comm.h"
#include "mesg.pb-c.h"

#include <curl/curl.h>
#include <cjson/cJSON.h>

#define URL_MAX_LEN (1024)
#define SDK_HOST_MAX_LEN    (256)
#define SDK_CONN_INFO_MAX_LEN    (1024)

/* 静态函数 */
static sdk_ssvr_t *sdk_ssvr_get_curr(sdk_cntx_t *ctx);

static int sdk_ssvr_creat_sendq(sdk_ssvr_t *ssvr, const sdk_conf_t *conf);
static int sdk_ssvr_creat_usck(sdk_ssvr_t *ssvr, const sdk_conf_t *conf);

static int sdk_ssvr_recv_cmd(sdk_cntx_t *ctx, sdk_ssvr_t *ssvr);
static int sdk_ssvr_recv_proc(sdk_cntx_t *ctx, sdk_ssvr_t *ssvr);

static int sdk_ssvr_data_proc(sdk_cntx_t *ctx, sdk_ssvr_t *ssvr, sdk_sct_t *sck);
static bool sdk_is_sys_mesg(uint16_t cmd);
static int sdk_sys_mesg_proc(sdk_cntx_t *ctx, sdk_ssvr_t *ssvr, sdk_sct_t *sck, void *addr);
static int sdk_exp_mesg_proc(sdk_cntx_t *ctx, sdk_ssvr_t *ssvr, sdk_sct_t *sck, void *addr);

static int sdk_ssvr_timeout_hdl(sdk_cntx_t *ctx, sdk_ssvr_t *ssvr);
static int sdk_ssvr_proc_cmd(sdk_cntx_t *ctx, sdk_ssvr_t *ssvr, const sdk_cmd_t *cmd);
static int sdk_ssvr_send_data(sdk_cntx_t *ctx, sdk_ssvr_t *ssvr);

static int sdk_ssvr_clear_mesg(sdk_ssvr_t *ssvr);

static int sdk_ssvr_kpalive_req(sdk_cntx_t *ctx, sdk_ssvr_t *ssvr);
static int sdk_ssvr_online_req(sdk_cntx_t *ctx, sdk_ssvr_t *ssvr);
static int sdk_ssvr_update_conn_info(sdk_cntx_t *ctx, sdk_ssvr_t *ssvr);

static int sdk_ssvr_cmd_proc_req(sdk_cntx_t *ctx, sdk_ssvr_t *ssvr, int rqid);
static int sdk_ssvr_cmd_proc_all_req(sdk_cntx_t *ctx, sdk_ssvr_t *ssvr);

/******************************************************************************
 **函数名称: sdk_ssvr_init
 **功    能: 初始化发送线程
 **输入参数:
 **     ctx: 全局信息
 **     ssvr: 发送服务对象
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述:
 **注意事项:
 **作    者: # Qifeng.zou # 2015.01.14 #
 ******************************************************************************/
int sdk_ssvr_init(sdk_cntx_t *ctx, sdk_ssvr_t *ssvr, int idx)
{
    void *addr;
    sdk_conf_t *conf = &ctx->conf;
    sdk_sct_t *sck = &ssvr->sck;
    sdk_snap_t *recv = &sck->recv;
    sdk_conn_info_t *info = &ssvr->conn_info;

    ssvr->id = idx;
    ssvr->log = ctx->log;
    ssvr->ctx = (void *)ctx;
    ssvr->sck.fd = INVALID_FD;
    ssvr->sleep_sec = 1;

    /* > 创建发送队列 */
    ssvr->sendq = ctx->sendq[idx];

    /* > 连接信息 */
    memset(info, 0, sizeof(sdk_conn_info_t));

    info->iplist = list_creat(NULL);
    if (NULL == info->iplist) {
        log_error(ssvr->log, "Create ip list failed!");
        return SDK_ERR;
    }

    /* > 创建unix套接字 */
    if (sdk_ssvr_creat_usck(ssvr, conf)) {
        log_error(ssvr->log, "Initialize send queue failed!");
        return SDK_ERR;
    }

    /* > 创建发送链表 */
    sck->mesg_list = list_creat(NULL);
    if (NULL == sck->mesg_list) {
        log_error(ssvr->log, "Create list failed!");
        return SDK_ERR;
    }

    /* > 初始化发送缓存(注: 程序退出时才可释放此空间，其他任何情况下均不释放) */
    if (wiov_init(&sck->send, 2 * conf->sendq.max)) {
        log_error(ssvr->log, "Initialize send iov failed!");
        return SDK_ERR;
    }

    /* 5. 初始化接收缓存(注: 程序退出时才可释放此空间，其他任何情况下均不释放) */
    addr = calloc(1, conf->recv_buff_size);
    if (NULL == addr) {
        log_error(ssvr->log, "errmsg:[%d] %s!", errno, strerror(errno));
        return SDK_ERR;
    }

    sdk_snap_setup(recv, addr, conf->recv_buff_size);

    return SDK_OK;
}

/******************************************************************************
 **函数名称: sdk_ssvr_creat_usck
 **功    能: 创建发送线程的命令接收套接字
 **输入参数:
 **     ssvr: 发送服务对象
 **     conf: 配置信息
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述:
 **注意事项:
 **作    者: # Qifeng.zou # 2015.01.14 #
 ******************************************************************************/
static int sdk_ssvr_creat_usck(sdk_ssvr_t *ssvr, const sdk_conf_t *conf)
{
    char path[FILE_PATH_MAX_LEN];

    sdk_ssvr_usck_path(conf, path, ssvr->id);

    ssvr->cmd_sck_id = unix_udp_creat(path);
    if (ssvr->cmd_sck_id < 0) {
        log_error(ssvr->log, "errmsg:[%d] %s! path:%s", errno, strerror(errno), path);
        return SDK_ERR;
    }

    log_trace(ssvr->log, "cmd_sck_id:[%d] path:%s", ssvr->cmd_sck_id, path);
    return SDK_OK;
}

/******************************************************************************
 **函数名称: sdk_ssvr_set_rwset
 **功    能: 设置读写集
 **输入参数:
 **     ssvr: 发送服务对象
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述:
 **注意事项:
 **作    者: # Qifeng.zou # 2015.01.16 #
 ******************************************************************************/
void sdk_ssvr_set_rwset(sdk_ssvr_t *ssvr)
{
    FD_ZERO(&ssvr->rset);
    FD_ZERO(&ssvr->wset);

    FD_SET(ssvr->cmd_sck_id, &ssvr->rset);

    ssvr->max = ssvr->cmd_sck_id;

    if (ssvr->sck.fd > 0) {
        ssvr->max = MAX(ssvr->cmd_sck_id, ssvr->sck.fd);

        /* 1 设置读集合 */
        FD_SET(ssvr->sck.fd, &ssvr->rset);

        /* 2 设置写集合: 发送至接收端 */
        if (!list_empty(ssvr->sck.mesg_list)
            || !queue_empty(ssvr->sendq)) {
            FD_SET(ssvr->sck.fd, &ssvr->wset);
            return;
        }
        else if (!wiov_isempty(&ssvr->sck.send)) {
            FD_SET(ssvr->sck.fd, &ssvr->wset);
            return;
        }
    }

    return;
}

/******************************************************************************
 **函数名称: sdk_ssvr_routine
 **功    能: 发送线程入口函数
 **输入参数:
 **     _ctx: 全局信息
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述:
 **注意事项:
 **作    者: # Qifeng.zou # 2015.01.16 #
 ******************************************************************************/
void *sdk_ssvr_routine(void *_ctx)
{
    int ret;
    sdk_sct_t *sck;
    sdk_ssvr_t *ssvr;
    struct timeval timeout;
    sdk_cntx_t *ctx = (sdk_cntx_t *)_ctx;

    nice(-20);

    /* 1. 获取发送线程 */
    ssvr = sdk_ssvr_get_curr(ctx);
    if (NULL == ssvr) {
        log_fatal(ssvr->log, "Get current thread failed!");
        abort();
        return (void *)-1;
    }

    sck = &ssvr->sck;

    /* 3. 进行事件处理 */
    for (;;) {
        /* 3.2 等待事件通知 */
        sdk_ssvr_set_rwset(ssvr);

        timeout.tv_sec = ssvr->sleep_sec;
        timeout.tv_usec = 0;
        ret = select(ssvr->max+1, &ssvr->rset, &ssvr->wset, NULL, &timeout);
        if (ret < 0) {
            if (EINTR == errno) { continue; }
            log_fatal(ssvr->log, "errmsg:[%d] %s!", errno, strerror(errno));
            abort();
            return (void *)-1;
        }
        else if (0 == ret) {
            sdk_ssvr_timeout_hdl(ctx, ssvr);
            continue;
        }

        /* 发送数据: 发送优先 */
        if (FD_ISSET(sck->fd, &ssvr->wset)) {
            sdk_ssvr_send_data(ctx, ssvr);
        }

        /* 接收命令 */
        if (FD_ISSET(ssvr->cmd_sck_id, &ssvr->rset)) {
            sdk_ssvr_recv_cmd(ctx, ssvr);
        }

        /* 接收Recv服务的数据 */
        if (FD_ISSET(sck->fd, &ssvr->rset)) {
            sdk_ssvr_recv_proc(ctx, ssvr);
        }
    }

    abort();
    return (void *)-1;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/******************************************************************************
 **函数名称: sdk_ssvr_kpalive_req
 **功    能: 发送保活命令
 **输入参数:
 **     ctx: 全局信息
 **     ssvr: Snd线程对象
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述:
 **注意事项:
 **     因发送KeepAlive请求时，说明链路空闲时间较长，
 **     因此发送数据时，不用判断EAGAIN的情况是否存在。
 **作    者: # Qifeng.zou # 2015.01.14 #
 ******************************************************************************/
static int sdk_ssvr_kpalive_req(sdk_cntx_t *ctx, sdk_ssvr_t *ssvr)
{
    void *addr;
    mesg_header_t *head;
    int size = sizeof(mesg_header_t);
    sdk_sct_t *sck = &ssvr->sck;
    wiov_t *send = &ssvr->sck.send;

    /* 1. 上次发送保活请求之后 仍未收到应答 */
    if ((sck->fd < 0)
        || (SDK_KPALIVE_STAT_SENT == sck->kpalive
            && sck->kpalive_times > 3 ))
    {
        CLOSE(sck->fd);
        wiov_clean(send);
        log_error(ssvr->log, "Didn't get keepalive respond for a long time!");
        return SDK_OK;
    }

    addr = (void *)calloc(1, size);
    if (NULL == addr) {
        log_error(ssvr->log, "Alloc memory failed!");
        return SDK_ERR;
    }

    /* 2. 设置心跳数据 */
    head = (mesg_header_t *)addr;

    head->cmd = CMD_PING;
    head->len = 0;
    head->flag = 0;
    head->from = ctx->sid;

    /* 3. 加入发送列表 */
    if (list_rpush(sck->mesg_list, addr)) {
        free(addr);
        log_error(ssvr->log, "Insert list failed!");
        return SDK_ERR;
    }

    log_debug(ssvr->log, "Add keepalive request success! fd:[%d]", sck->fd);

    ++sck->kpalive_times;
    sdk_set_kpalive_stat(sck, SDK_KPALIVE_STAT_SENT);
    return SDK_OK;
}

/******************************************************************************
 **函数名称: sdk_ssvr_online_req
 **功    能: 发送ONLINE命令
 **输入参数:
 **     ctx: 全局信息
 **     ssvr: Snd线程对象
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述:
 **注意事项: 连接建立后, 则发送ONLINE请求.
 **作    者: # Qifeng.zou # 2016.11.08 17:52:19 #
 ******************************************************************************/
static int sdk_ssvr_online_req(sdk_cntx_t *ctx, sdk_ssvr_t *ssvr)
{
    void *addr;
    size_t size;
    mesg_header_t *head;
    sdk_sct_t *sck = &ssvr->sck;
    sdk_conf_t *conf = &ctx->conf;
    sdk_conn_info_t *info = &ssvr->conn_info;
    Mesg__Online online = MESG__ONLINE__INIT;

    /* > 设置ONLINE字段 */
    online.cid = conf->clientid;
    online.appkey = conf->appkey;
    online.version = conf->version;
    online.token = info->token;

    /* > 申请内存空间 */
    size = sizeof(mesg_header_t) + mesg__online__get_packed_size(&online);

    addr = (void *)calloc(1, size);
    if (NULL == addr) {
        log_error(ssvr->log, "Alloc memory failed! errmsg:[%d] %s!", errno, strerror(errno));
        return SDK_ERR;
    }

    /* 2. 设置心跳数据 */
    head = (mesg_header_t *)addr;

    head->cmd = CMD_ONLINE;
    head->len = size - sizeof(mesg_header_t);
    head->flag = 0;
    head->from = ctx->sid;

    /* 3. 加入发送列表 */
    if (list_rpush(sck->mesg_list, addr)) {
        free(addr);
        log_error(ssvr->log, "Insert list failed!");
        return SDK_ERR;
    }

    log_debug(ssvr->log, "Add keepalive request success! fd:[%d]", sck->fd);

    ++sck->kpalive_times;
    sdk_set_kpalive_stat(sck, SDK_KPALIVE_STAT_SENT);
    return SDK_OK;
}



////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/******************************************************************************
 **函数名称: sdk_ssvr_get_curr
 **功    能: 获取当前发送线程的上下文
 **输入参数:
 **     ssvr: 发送服务对象
 **     conf: 配置信息
 **输出参数: NONE
 **返    回: Address of sndsvr
 **实现描述:
 **注意事项:
 **作    者: # Qifeng.zou # 2015.01.14 #
 ******************************************************************************/
static sdk_ssvr_t *sdk_ssvr_get_curr(sdk_cntx_t *ctx)
{
    int id;

    /* 1. 获取线程索引 */
    id = thread_pool_get_tidx(ctx->sendtp);
    if (id < 0) {
        log_error(ctx->log, "Get current thread index failed!");
        return NULL;
    }

    /* 2. 返回线程对象 */
    return (sdk_ssvr_t *)(ctx->sendtp->data + id * sizeof(sdk_ssvr_t));
}

/* 重连 */
static int sdk_ssvr_reconn(ip_port_t *item, sdk_ssvr_t *ssvr)
{
    sdk_sct_t *sck = &ssvr->sck;

    /* > 重连接入层 */
    if ((sck->fd = tcp_connect(AF_INET, item->ipaddr, item->port)) < 0) {
        log_error(ssvr->log, "Conncet [%s:%d] failed! errmsg:[%d] %s!",
                item->ipaddr, item->port, errno, strerror(errno));
        return false;
    }

    ssvr->sleep_sec = SDK_RECONN_INTV;

    return true;
}

/******************************************************************************
 **函数名称: sdk_ssvr_timeout_hdl
 **功    能: 超时处理
 **输入参数:
 **     ctx: 全局信息
 **     ssvr: 发送服务全局信息
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述:
 **     1. 判断是否长时间无数据通信
 **     2. 发送保活数据
 **注意事项:
 **作    者: # Qifeng.zou # 2015.01.14 #
 ******************************************************************************/
static int sdk_ssvr_timeout_hdl(sdk_cntx_t *ctx, sdk_ssvr_t *ssvr)
{
    time_t curr_tm = time(NULL);
    sdk_sct_t *sck = &ssvr->sck;
    sdk_conn_info_t *info = &ssvr->conn_info;

    /* 如果网路已断开, 则进行重连 */
    if (sck->fd < 0) {
        if (curr_tm > info->expire) { // 更新
            if (sdk_ssvr_update_conn_info(ctx, ssvr)) {
                log_error(ssvr->log, "Update conn information failed!");
                return -1;
            }
        }

        /* 3.1 连接合法性判断 */
        sdk_ssvr_clear_mesg(ssvr);

        if (NULL == list_find(info->iplist, (find_cb_t)sdk_ssvr_reconn, (void *)ssvr)) {
            if (ssvr->sleep_sec < 300) {
                ssvr->sleep_sec = (!ssvr->sleep_sec)? SDK_RECONN_INTV : 2*ssvr->sleep_sec;
            }
            return SDK_ERR;
        }

        sdk_ssvr_online_req(ctx, ssvr);

        ssvr->sleep_sec = SDK_RECONN_INTV;
        sdk_set_kpalive_stat(sck, SDK_KPALIVE_STAT_UNKNOWN);
        return SDK_OK;
    }

    /* 1. 判断是否长时无数据 */
    if ((curr_tm - sck->wrtm) < SDK_KPALIVE_INTV) {
        return SDK_OK;
    }

    /* 2. 发送保活请求 */
    if (sdk_ssvr_kpalive_req(ctx, ssvr)) {
        log_error(ssvr->log, "Connection keepalive failed!");
        return SDK_ERR;
    }

    sck->wrtm = curr_tm;

    return SDK_OK;
}

/******************************************************************************
 **函数名称: sdk_ssvr_recv_proc
 **功    能: 接收网络数据
 **输入参数:
 **     ctx: 全局信息
 **     ssvr: 发送服务
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述:
 **     1. 接收网络数据
 **     2. 进行数据处理
 **注意事项:
 **       ------------------------------------------------
 **      | 已处理 |     未处理     |       剩余空间       |
 **       ------------------------------------------------
 **      |XXXXXXXX|////////////////|                      |
 **      |XXXXXXXX|////////////////|         left         |
 **      |XXXXXXXX|////////////////|                      |
 **       ------------------------------------------------
 **      ^        ^                ^                      ^
 **      |        |                |                      |
 **     addr     optr             iptr                   end
 **作    者: # Qifeng.zou # 2015.01.14 #
 ******************************************************************************/
static int sdk_ssvr_recv_proc(sdk_cntx_t *ctx, sdk_ssvr_t *ssvr)
{
    int n, left;
    sdk_sct_t *sck = &ssvr->sck;
    sdk_snap_t *recv = &sck->recv;

    sck->rdtm = time(NULL);

    while (1) {
        /* 1. 接收网络数据 */
        left = (int)(recv->end - recv->iptr);

        n = read(sck->fd, recv->iptr, left);
        if (n > 0) {
            recv->iptr += n;

            /* 2. 进行数据处理 */
            if (sdk_ssvr_data_proc(ctx, ssvr, sck)) {
                log_error(ssvr->log, "Proc data failed! fd:%d", sck->fd);

                CLOSE(sck->fd);
                sdk_snap_reset(recv);
                return SDK_ERR;
            }
            continue;
        }
        else if (0 == n) {
            log_info(ssvr->log, "Server disconnected. fd:%d n:%d/%d", sck->fd, n, left);
            CLOSE(sck->fd);
            sdk_snap_reset(recv);
            return SDK_SCK_DISCONN;
        }
        else if ((n < 0) && (EAGAIN == errno)) {
            return SDK_OK; /* Again */
        }
        else if (EINTR == errno) {
            continue;
        }

        log_error(ssvr->log, "errmsg:[%d] %s. fd:%d", errno, strerror(errno), sck->fd);

        CLOSE(sck->fd);
        sdk_snap_reset(recv);
        return SDK_ERR;
    }

    return SDK_OK;
}

/******************************************************************************
 **函数名称: sdk_ssvr_data_proc
 **功    能: 进行数据处理
 **输入参数:
 **     ctx: 全局信息
 **     ssvr: 发送服务
 **     sck: 连接对象
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述:
 **     1. 是否含有完整数据
 **     2. 校验数据合法性
 **     3. 进行数据处理
 **注意事项:
 **       ------------------------------------------------
 **      | 已处理 |     未处理     |       剩余空间       |
 **       ------------------------------------------------
 **      |XXXXXXXX|////////////////|                      |
 **      |XXXXXXXX|////////////////|         left         |
 **      |XXXXXXXX|////////////////|                      |
 **       ------------------------------------------------
 **      ^        ^                ^                      ^
 **      |        |                |                      |
 **     addr     optr             iptr                   end
 **作    者: # Qifeng.zou # 2015.01.14 #
 ******************************************************************************/
static int sdk_ssvr_data_proc(sdk_cntx_t *ctx, sdk_ssvr_t *ssvr, sdk_sct_t *sck)
{
    mesg_header_t *head;
    uint32_t len, mesg_len;
    sdk_snap_t *recv = &sck->recv;

    while (1) {
        head = (mesg_header_t *)recv->optr;
        len = (int)(recv->iptr - recv->optr);
        if (len < sizeof(mesg_header_t)) {
            goto LEN_NOT_ENOUGH; /* 不足一条数据时 */
        }

        /* 1. 是否不足一条数据 */
        mesg_len = sizeof(mesg_header_t) + ntohl(head->len);
        if (len < mesg_len) {
        LEN_NOT_ENOUGH:
            if (recv->iptr == recv->end) {
                /* 防止OverWrite的情况发生 */
                if ((recv->optr - recv->base) < (recv->end - recv->iptr)) {
                    log_fatal(ssvr->log, "Data length is invalid!");
                    return SDK_ERR;
                }

                memcpy(recv->base, recv->optr, len);
                recv->optr = recv->base;
                recv->iptr = recv->optr + len;
                return SDK_OK;
            }
            return SDK_OK;
        }

        /* 2. 至少一条数据时 */
        /* 2.1 转化字节序 */
        SDK_HEAD_NTOH(head, head);

        log_trace(ssvr->log, "cmd:%d len:%d flag:%d", head->cmd, head->len, head->flag);

        /* 2.2 校验合法性 */
        if (!SDK_HEAD_ISVALID(head)) {
            ++ssvr->err_total;
            log_error(ssvr->log, "Header is invalid! cmd:%d len:%d flag:%d",
                  head->cmd, head->len, head->flag);
            return SDK_ERR;
        }

        /* 2.3 如果是系统消息 */
        if (sdk_is_sys_mesg(head->cmd)) {
            sdk_sys_mesg_proc(ctx, ssvr, sck, recv->optr);
        }
        else {
            sdk_exp_mesg_proc(ctx, ssvr, sck, recv->optr);
        }

        recv->optr += mesg_len;
    }

    return SDK_OK;
}

/******************************************************************************
 **函数名称: sdk_ssvr_recv_cmd
 **功    能: 接收命令数据
 **输入参数:
 **     ctx: 全局信息
 **     ssvr: 发送服务对象
 **输出参数:
 **返    回: 0:成功 !0:失败
 **实现描述:
 **     1. 接收命令
 **     2. 处理命令
 **注意事项:
 **作    者: # Qifeng.zou # 2015.01.14 #
 ******************************************************************************/
static int sdk_ssvr_recv_cmd(sdk_cntx_t *ctx, sdk_ssvr_t *ssvr)
{
    sdk_cmd_t cmd;

    memset(&cmd, 0, sizeof(cmd));

    /* 1. 接收命令 */
    if (unix_udp_recv(ssvr->cmd_sck_id, &cmd, sizeof(cmd)) < 0) {
        log_error(ssvr->log, "Recv command failed! errmsg:[%d] %s!", errno, strerror(errno));
        return SDK_ERR;
    }

    /* 2. 处理命令 */
    return sdk_ssvr_proc_cmd(ctx, ssvr, &cmd);
}

/******************************************************************************
 **函数名称: sdk_ssvr_proc_cmd
 **功    能: 命令处理
 **输入参数:
 **     ssvr: 发送服务对象
 **     cmd: 接收到的命令信息
 **输出参数:
 **返    回: 0:成功 !0:失败
 **实现描述:
 **注意事项:
 **作    者: # Qifeng.zou # 2015.01.14 #
 ******************************************************************************/
static int sdk_ssvr_proc_cmd(sdk_cntx_t *ctx, sdk_ssvr_t *ssvr, const sdk_cmd_t *cmd)
{
    sdk_sct_t *sck = &ssvr->sck;
    wiov_t *send = &sck->send;

    switch (cmd->type) {
        case SDK_CMD_SEND:
        case SDK_CMD_SEND_ALL:
            log_debug(ssvr->log, "Recv command! type:[%d]", cmd->type);
            if (fd_is_writable(sck->fd)) {
                return sdk_ssvr_send_data(ctx, ssvr);
            }
            return SDK_OK;
        case SDK_CMD_NETWORK_ON:
            CLOSE(sck->fd);
            wiov_clean(send);
            ssvr->sleep_sec = 0;
            return SDK_OK;
        case SDK_CMD_NETWORK_OFF:
            CLOSE(sck->fd);
            wiov_clean(send);
            ssvr->sleep_sec = SDK_RECONN_INTV;
            return SDK_OK;
        default:
            log_error(ssvr->log, "Unknown command! type:[%d]", cmd->type);
            return SDK_OK;
    }
    return SDK_OK;
}

/******************************************************************************
 **函数名称: sdk_ssvr_wiov_add
 **功    能: 添加发送数据(零拷贝)
 **输入参数:
 **     ssvr: 发送服务
 **     sck: 连接对象
 **输出参数:
 **返    回: 需要发送的数据长度
 **实现描述:
 **     1. 从消息链表取数据
 **     2. 从发送队列取数据
 **注意事项: WARNNING: 千万勿将共享变量参与MIN()三目运算, 否则可能出现严重错误!!!!且很难找出原因!
 **          原因: MIN()不是原子运算, 使用共享变量可能导致判断成立后, 而返回时共
 **                享变量的值可能被其他进程或线程修改, 导致出现严重错误!
 **作    者: # Qifeng.zou # 2015.12.26 08:23:22 #
 ******************************************************************************/
static int sdk_ssvr_wiov_add(sdk_ssvr_t *ssvr, sdk_sct_t *sck)
{
#define RTSD_POP_NUM    (1024)
    size_t len;
    int num, idx;
    mesg_header_t *head;
    void *data[RTSD_POP_NUM];
    wiov_t *send = &sck->send;

    /* > 从消息链表取数据 */
    while(!wiov_isfull(send)) {
        /* > 是否有数据 */
        head = (mesg_header_t *)list_lpop(sck->mesg_list);
        if (NULL == head) {
            break; /* 无数据 */
        }

        len = sizeof(mesg_header_t) + head->len;

        /* > 取发送的数据 */
        SDK_HEAD_HTON(head, head);

        /* > 设置发送数据 */
        wiov_item_add(send, head, len, NULL, mem_dealloc);
    }

    /* > 从发送队列取数据 */
    for (;;) {
        /* > 判断剩余空间(WARNNING: 勿将共享变量参与三目运算, 否则可能出现严重错误!!!) */
        num = MIN(wiov_left_space(send), RTSD_POP_NUM);
        num = MIN(num, queue_used(ssvr->sendq));
        if (0 == num) {
            break; /* 空间不足 */
        }

        /* > 弹出发送数据 */
        num = queue_mpop(ssvr->sendq, data, num);
        if (0 == num) {
            continue;
        }

        log_trace(ssvr->log, "Multi-pop num:%d!", num);

        for (idx=0; idx<num; ++idx) {
            /* > 是否有数据 */
            head = (mesg_header_t *)data[idx];

            len = sizeof(mesg_header_t) + head->len;

            /* > 设置发送数据 */
            SDK_HEAD_HTON(head, head);

            /* > 设置发送数据 */
            wiov_item_add(send, head, len, ssvr->sendq, queue_dealloc);
        }
    }

    return 0;
}

/******************************************************************************
 **函数名称: sdk_ssvr_send_data
 **功    能: 发送系统消息
 **输入参数:
 **     ctx: 全局信息
 **     ssvr: 发送服务
 **输出参数:
 **返    回: 0:成功 !0:失败
 **实现描述:
 **     1. 填充发送缓存
 **     2. 发送缓存数据
 **     3. 重置标识量
 **注意事项:
 **       ------------------------------------------------
 **      | 已发送 |     待发送     |       剩余空间       |
 **       ------------------------------------------------
 **      |XXXXXXXX|////////////////|                      |
 **      |XXXXXXXX|////////////////|         left         |
 **      |XXXXXXXX|////////////////|                      |
 **       ------------------------------------------------
 **      ^        ^                ^                      ^
 **      |        |                |                      |
 **     addr     optr             iptr                   end
 **作    者: # Qifeng.zou # 2015.01.14 #
 ******************************************************************************/
static int sdk_ssvr_send_data(sdk_cntx_t *ctx, sdk_ssvr_t *ssvr)
{
    ssize_t n;
    sdk_sct_t *sck = &ssvr->sck;
    wiov_t *send = &sck->send;

    sck->wrtm = time(NULL);

    for (;;) {
        /* 1. 填充发送缓存 */
        if (!wiov_isfull(send)) {
            sdk_ssvr_wiov_add(ssvr, sck);
        }

        if (wiov_isempty(send)) {
            break;
        }

        /* 2. 发送缓存数据 */
        n = writev(sck->fd, wiov_item_begin(send), wiov_item_num(send));
        if (n < 0) {
            log_error(ssvr->log, "errmsg:[%d] %s! fd:%u",
                  errno, strerror(errno), sck->fd);
            CLOSE(sck->fd);
            wiov_clean(send);
            return SDK_ERR;
        }
        /* 只发送了部分数据 */
        else {
            wiov_item_adjust(send, n);
            return SDK_OK;
        }
    }

    return SDK_OK;
}

/******************************************************************************
 **函数名称: sdk_ssvr_clear_mesg
 **功    能: 清空发送消息
 **输入参数:
 **     ssvr: 发送服务
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 依次取出每条消息, 并释放所占有的空间
 **注意事项:
 **作    者: # Qifeng.zou # 2015.01.16 #
 ******************************************************************************/
static int sdk_ssvr_clear_mesg(sdk_ssvr_t *ssvr)
{
    void *data;

    while (1) {
        data = list_lpop(ssvr->sck.mesg_list);
        if (NULL == data) {
            return SDK_OK;
        }
        free(data);
    }

    return SDK_OK;
}

/******************************************************************************
 **函数名称: sdk_is_sys_mesg
 **功    能: 是否为系统消息
 **输入参数:
 **     cmd: 消息类型
 **输出参数: NONE
 **返    回: true:成功 false:失败
 **实现描述: 
 **注意事项:
 **作    者: # Qifeng.zou # 2016.11.03 14:25:41 #
 ******************************************************************************/
static bool sdk_is_sys_mesg(uint16_t cmd)
{
    switch (cmd) {
        case CMD_PING:
        case CMD_PONG:
            return true;
    }
    return false;
}

/******************************************************************************
 **函数名称: sdk_sys_mesg_proc
 **功    能: 系统消息的处理
 **输入参数:
 **     ctx: 全局信息
 **     ssvr: 发送服务
 **     sck: 连接对象
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 根据消息类型调用对应的处理接口
 **注意事项:
 **作    者: # Qifeng.zou # 2015.01.16 #
 ******************************************************************************/
static int sdk_sys_mesg_proc(sdk_cntx_t *ctx, sdk_ssvr_t *ssvr, sdk_sct_t *sck, void *addr)
{
    mesg_header_t *head = (mesg_header_t *)addr;

    switch (head->cmd) {
        case CMD_PONG:      /* 保活应答 */
            sck->kpalive_times = 0;
            sdk_set_kpalive_stat(sck, SDK_KPALIVE_STAT_SUCC);
            return SDK_OK;
        case CMD_PING:      /* 保活请求 */
            sck->kpalive_times = 0;
            sdk_set_kpalive_stat(sck, SDK_KPALIVE_STAT_SUCC);
            sdk_ssvr_kpalive_req(ctx, ssvr);
            return SDK_OK;
    }

    log_error(ssvr->log, "Unknown type [%d]!", head->cmd);
    return SDK_ERR;
}

/******************************************************************************
 **函数名称: sdk_exp_mesg_proc
 **功    能: 自定义消息的处理
 **输入参数:
 **     ctx: 全局信息
 **     ssvr: 发送服务
 **     sck: 连接对象
 **     addr: 数据地址
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 将自定义消息放入工作队列中, 一次只放入一条数据
 **注意事项:
 **作    者: # Qifeng.zou # 2015.05.19 #
 ******************************************************************************/
static int sdk_exp_mesg_proc(sdk_cntx_t *ctx, sdk_ssvr_t *ssvr, sdk_sct_t *sck, void *addr)
{
    void *data;
    int idx, len;
    mesg_header_t *head = (mesg_header_t *)addr;

    ++ssvr->recv_total;

    /* > 验证长度 */
    len = SDK_DATA_TOTAL_LEN(head);
    if ((int)len > queue_size(ctx->recvq[0])) {
        ++ssvr->drop_total;
        log_error(ctx->log, "Data is too long! len:%d drop:%lu total:%lu",
              len, ssvr->drop_total, ssvr->recv_total);
        return SDK_ERR_TOO_LONG;
    }

   /* > 申请空间 */
    idx = rand() % ctx->conf.work_thd_num;

    data = queue_malloc(ctx->recvq[idx], len);
    if (NULL == data) {
        ++ssvr->drop_total;
        log_error(ctx->log, "Alloc from queue failed! drop:%lu recv:%lu size:%d/%d",
              ssvr->drop_total, ssvr->recv_total, len, queue_size(ctx->recvq[idx]));
        return SDK_ERR;
    }

    /* > 放入队列 */
    memcpy(data, addr, len);

    if (queue_push(ctx->recvq[idx], data)) {
        ++ssvr->drop_total;
        log_error(ctx->log, "Push into queue failed! len:%d drop:%lu total:%lu",
              len, ssvr->drop_total, ssvr->recv_total);
        queue_dealloc(ctx->recvq[idx], data);
        return SDK_ERR;
    }

    sdk_ssvr_cmd_proc_req(ctx, ssvr, idx);    /* 发送处理请求 */

    return SDK_OK;
}

/******************************************************************************
 **函数名称: sdk_ssvr_cmd_proc_req
 **功    能: 发送处理请求
 **输入参数:
 **     ctx: 全局对象
 **     ssvr: 接收服务
 **     rqid: 队列ID(与工作队列ID一致)
 **输出参数: NONE
 **返    回: >0:成功 <=0:失败
 **实现描述:
 **注意事项:
 **作    者: # Qifeng.zou # 2015.06.08 #
 ******************************************************************************/
static int sdk_ssvr_cmd_proc_req(sdk_cntx_t *ctx, sdk_ssvr_t *ssvr, int rqid)
{
    sdk_cmd_t cmd;
    char path[FILE_PATH_MAX_LEN];
    sdk_cmd_proc_req_t *req = (sdk_cmd_proc_req_t *)&cmd.param;

    memset(&cmd, 0, sizeof(cmd));

    cmd.type = SDK_CMD_PROC_REQ;
    req->ori_svr_id = ssvr->id;
    req->num = -1;
    req->rqidx = rqid;

    /* > 获取Worker路径 */
    sdk_worker_usck_path(&ctx->conf, path, rqid);

    /* > 发送处理命令 */
    return unix_udp_send(ssvr->cmd_sck_id, path, &cmd, sizeof(sdk_cmd_t));
}

/******************************************************************************
 **函数名称: sdk_ssvr_cmd_proc_all_req
 **功    能: 发送处理请求
 **输入参数:
 **     ctx: 全局对象
 **     ssvr: 接收服务
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 遍历所有接收队列, 并发送处理请求
 **注意事项:
 **作    者: # Qifeng.zou # 2015.06.08 #
 ******************************************************************************/
static int sdk_ssvr_cmd_proc_all_req(sdk_cntx_t *ctx, sdk_ssvr_t *ssvr)
{
    int idx;

    for (idx=0; idx<ctx->conf.send_thd_num; ++idx) {
        sdk_ssvr_cmd_proc_req(ctx, ssvr, idx);
    }

    return SDK_OK;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/******************************************************************************
 **函数名称: sdk_ssvr_write_conn_info
 **功    能: 接收HTTP返回的数据
 **输入参数:
 **     ptr: 收到的数据
 **     size: 数据长度
 **     nmemb: 内存块数
 **     stream: 存储数据的空间(用户定义)
 **输出参数: NONE
 **返    回: 接收数据的长度
 **实现描述: 返回值必须为size * nmemb, 否则会中断数据的接口.
 **注意事项:
 **作    者: # Qifeng.zou # 2016.11.07 16:52:04 #
 ******************************************************************************/
static size_t sdk_ssvr_write_conn_info(void *ptr, size_t size, size_t nmemb, void *stream)
{
    fprintf(stderr, "Call %s()! size:%lu nmemb:%lu", __func__, size, nmemb);

    if (strlen((char *)stream) + strlen((char *)ptr) > SDK_CONN_INFO_MAX_LEN) {
        return 0;
    }

    strcat(stream, (char *)ptr);

    return size*nmemb;
}

/******************************************************************************
 **函数名称: sdk_ssvr_http_conn_info
 **功    能: 通过HTTPSVR获取连接信息
 **输入参数:
 **     ctx: 全局对象
 **     ssvr: 接收服务
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 通过libcurl访问/client/conninfo接口, 获取连接信息
 **注意事项:
 **作    者: # Qifeng.zou # 2016.11.07 14:38:04 #
 ******************************************************************************/
static int sdk_ssvr_http_conn_info(sdk_cntx_t *ctx, char *conn_info_str)
{
    CURL *curl;
    CURLcode ret = -1;
    sdk_conf_t *conf = &ctx->conf;
    struct curl_slist *chunk = NULL;
    char host[SDK_HOST_MAX_LEN], url[URL_MAX_LEN];

    curl = curl_easy_init();
    if (NULL == curl) {
        log_error(ctx->log, "Initialize curl failed!");
        return -1;
    }

    /* Remove a header curl would otherwise add by itself */
    chunk = curl_slist_append(chunk, "Accept:");

    /* Add a custom header */
    chunk = curl_slist_append(chunk, "Another: yes");

    /* Modify a header curl otherwise adds differently */
    snprintf(host, sizeof(host), "Host: %s", conf->httpsvr);
    chunk = curl_slist_append(chunk, host);

    /* Add a header with "blank" contents to the right of the colon. Note that
     *        we're then using a semicolon in the string we pass to curl! */
    chunk = curl_slist_append(chunk, "X-silly-header;");

    /* set our custom set of headers */
    ret = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, sdk_ssvr_write_conn_info);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, conn_info_str);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3);

    snprintf(url, sizeof(url), "%s/client/conInfo?clientid=%s&app_key=%s&vlink=1",
            conf->httpsvr, conf->clientid, conf->appkey);
    log_debug(ctx->log, "url: %s!", url);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    ret = curl_easy_perform(curl);
    /* Check for errors */
    if(CURLE_OK != ret) {
        log_error(ctx->log, "Exec curl_easy_perform() failed! errmsg:[%d] %s!",
                ret, curl_easy_strerror(ret));
    }

    /* always cleanup */
    curl_easy_cleanup(curl);

    /* free the custom headers */
    curl_slist_free_all(chunk);

    return ret;
}

/******************************************************************************
 **函数名称: sdk_ssvr_parse_conn_info
 **功    能: 解析HTTPSVR获取连接信息
 **输入参数:
 **     ctx: 全局对象
 **     ssvr: 接收服务
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 通过libcurl访问/client/conninfo接口, 获取连接信息
 **注意事项:
 **作    者: # Qifeng.zou # 2016.11.07 14:38:04 #
 ******************************************************************************/
static int sdk_ssvr_parse_conn_info(
        sdk_cntx_t *ctx, sdk_ssvr_t *ssvr, const char *conn_info_str)
{
    int ip_list_len;
    ip_port_t *item, ip_item;
    sdk_conn_info_t *conn_info = &ssvr->conn_info;
    cJSON *info, *code, *data, *expire, *token, *iplist, *ip, *sessionid;

    /* > 解析JSON数据 */
    info = cJSON_Parse(conn_info_str);
    if (NULL == info) {
        log_error(ctx->log, "Parse conn info failed! info:%s", conn_info_str);
        return -1;
    }

    do {
        /* > 解析错误码 */
        code = cJSON_GetObjectItem(info, "code");
        if (NULL == code || 0 != atoi(code->valuestring)) {
            log_error(ctx->log, "Get conn info failed! info:%s", conn_info_str);
            break;
        }

        /* > 解析数据信息 */
        data = cJSON_GetObjectItem(info, "data");
        if (NULL == data) {
            log_error(ctx->log, "Get data failed! info:%s", conn_info_str);
            break;
        }

        /* > 解析数据信息-TOKEN超时时间 */
        expire = cJSON_GetObjectItem(data, "expire");
        if (NULL == expire || 0 == expire->valueint) {
            log_error(ctx->log, "Get expire failed! info:%s", conn_info_str);
            break;
        }

        conn_info->expire = time(NULL) + expire->valueint;

        /* > 解析数据信息-TOKEN */
        token = cJSON_GetObjectItem(data, "token");
        if (NULL == token || 0 == strlen(token->valuestring)) {
            log_error(ctx->log, "Get token failed! info:%s", conn_info_str);
            break;
        }

        snprintf(conn_info->token, sizeof(conn_info->token), "%s", token->valuestring);

        /* > 解析数据信息-IPLIST */
        iplist = cJSON_GetObjectItem(data, "ipList");
        if (NULL == iplist) {
            log_error(ctx->log, "Get expire info failed! info:%s", conn_info_str);
            break;
        }

        ip_list_len = cJSON_GetArraySize(iplist);
        if (0 == ip_list_len) {
            log_error(ctx->log, "Get ip list failed! info:%s", conn_info_str);
            break;
        }

        ip = iplist->child;
        while (NULL != ip) {
            if (str_to_ip_port(ip->valuestring, &ip_item)) {
                log_error(ctx->log, "Convert string to ip and port failed! info:%s", conn_info_str);
                continue;
            }

            item = (ip_port_t *)calloc(1, sizeof(ip_port_t));
            if (NULL == item) {
                log_error(ctx->log, "errmsg:[%d] %s!", errno, strerror(errno));
                continue;
            }

            snprintf(item->ipaddr, sizeof(item->ipaddr), "%s", ip_item.ipaddr);
            item->port = ip_item.port;

            list_lpush(conn_info->iplist, item);

            ip = ip->next;
        }

        /* > 解析数据信息-SESSIONID */
        sessionid = cJSON_GetObjectItem(data, "sessionid");
        if (NULL == sessionid || 0 == sessionid->valueint) {
            log_error(ctx->log, "Get sessionid failed! info:%s", conn_info_str);
            break;
        }
        conn_info->sessionid = (uint64_t)sessionid->valueint;
    } while(0);

    cJSON_Delete(info);

    return 0;
}

/******************************************************************************
 **函数名称: sdk_ssvr_update_conn_info
 **功    能: 更新连接信息
 **输入参数:
 **     ctx: 全局对象
 **     ssvr: 发送服务
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **     1. 通过HTTP接口获取连接信息
 **     2. 解析连接信息
 **注意事项:
 **作    者: # Qifeng.zou # 2016.11.08 11:14:27 #
 ******************************************************************************/
static int sdk_ssvr_update_conn_info(sdk_cntx_t *ctx, sdk_ssvr_t *ssvr)
{
    ip_port_t *item;
    char info_str[SDK_CONN_INFO_MAX_LEN];
    sdk_conn_info_t *info = &ssvr->conn_info;

    memset(info_str, 0, sizeof(info_str));

    /* > 重置连接信息 */
    info->expire = 0;
    memset(info->token, 0, sizeof(info->token));
    info->sessionid = 0;

    do {
        item = list_rpop(info->iplist);
        if (NULL == item) {
            break;
        }
        free(item);
    } while (NULL != item);

    /* > 获取连接信息 */
    if (sdk_ssvr_http_conn_info(ctx, info_str)) {
        log_error(ctx->log, "Get conn info by http failed!");
        return -1;
    }

    /* > 解析连接信息 */
    if (sdk_ssvr_parse_conn_info(ctx, ssvr, info_str)) {
        log_error(ctx->log, "Parse conn info failed!");
        return -1;
    }

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
