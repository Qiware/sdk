#include "str.h"
#include "sdk.h"
#include "redo.h"
#include "sdk_mesg.h"
#include "sdk_comm.h"
#include "mesg.pb-c.h"

#include <curl/curl.h>
#include <cjson/cJSON.h>

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/******************************************************************************
 **函数名称: sdk_mesg_send_ping_req
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
int sdk_mesg_send_ping_req(sdk_cntx_t *ctx, sdk_ssvr_t *ssvr)
{
    void *addr;
    mesg_header_t *head;
    int size = sizeof(mesg_header_t);
    sdk_sck_t *sck = &ssvr->sck;
    wiov_t *send = &ssvr->sck.send;
    sdk_conn_info_t *info = &ssvr->conn_info;

    /* 1. 上次发送保活请求之后 仍未收到应答 */
    if ((sck->fd < 0) || (sck->kpalive_times > 3)) {
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
    head->from = info->sessionid;

    /* 3. 加入发送列表 */
    if (list_rpush(sck->mesg_list, addr)) {
        free(addr);
        log_error(ssvr->log, "Insert list failed!");
        return SDK_ERR;
    }

    log_debug(ssvr->log, "Add ping request success!");

    ++sck->kpalive_times;
    sck->next_kpalive_tm = time(NULL) + SDK_PING_MIN_SEC;
    return SDK_OK;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/******************************************************************************
 **函数名称: sdk_mesg_send_online_req
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
int sdk_mesg_send_online_req(sdk_cntx_t *ctx, sdk_ssvr_t *ssvr)
{
    void *addr;
    size_t size;
    mesg_header_t *head;
    sdk_sck_t *sck = &ssvr->sck;
    sdk_conf_t *conf = &ctx->conf;
    sdk_conn_info_t *info = &ssvr->conn_info;
    Online online = ONLINE__INIT;

    /* > 设置ONLINE字段 */
    online.cid = conf->clientid;
    online.appkey = conf->appkey;
    online.version = conf->version;
    online.token = info->token;

    /* > 申请内存空间 */
    size = sizeof(mesg_header_t) + online__get_packed_size(&online);

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
    head->from = info->sessionid;

    online__pack(&online, addr+sizeof(mesg_header_t));

    /* 3. 加入发送列表 */
    if (list_rpush(sck->mesg_list, addr)) {
        free(addr);
        log_error(ssvr->log, "Insert list failed!");
        return SDK_ERR;
    }

    SDK_SSVR_SET_ONLINE(ssvr, false);

    log_debug(ssvr->log, "Add online request success!");

    return SDK_OK;
}



////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/******************************************************************************
 **函数名称: sdk_mesg_send_sync_req
 **功    能: 发送SYNC命令
 **输入参数:
 **     ctx: 全局信息
 **     ssvr: 读写线程对象
 **     sck: 通信套接字
 **     addr: 读取的数据
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述:
 **注意事项:
 **作    者: # Qifeng.zou # 2016.11.09 09:54:53 #
 ******************************************************************************/
int sdk_mesg_send_sync_req(sdk_cntx_t *ctx, sdk_ssvr_t *ssvr, sdk_sck_t *sck)
{
    void *addr;
    size_t size;
    mesg_header_t *head;
    sdk_conn_info_t *info = &ssvr->conn_info;

    /* > 申请内存空间 */
    size = sizeof(mesg_header_t);

    addr = (void *)calloc(1, size);
    if (NULL == addr) {
        log_error(ssvr->log, "Alloc memory failed! errmsg:[%d] %s!", errno, strerror(errno));
        return SDK_ERR;
    }

    /* 2. 设置SYNC数据 */
    head = (mesg_header_t *)addr;

    head->cmd = CMD_SYNC;
    head->len = 0;
    head->flag = 0;
    head->from = info->sessionid;

    /* 3. 加入发送列表 */
    if (list_rpush(sck->mesg_list, addr)) {
        free(addr);
        log_error(ssvr->log, "Insert list failed!");
        return SDK_ERR;
    }

    log_debug(ssvr->log, "Add sync request success!");

    return SDK_OK;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/******************************************************************************
 **函数名称: sdk_mesg_pong_handler
 **功    能: 处理PONG命令
 **输入参数:
 **     ctx: 全局信息
 **     ssvr: Snd线程对象
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述:
 **注意事项:
 **作    者: # Qifeng.zou # 2016.11.08 20:42:56 #
 ******************************************************************************/
int sdk_mesg_pong_handler(sdk_cntx_t *ctx, sdk_ssvr_t *ssvr, sdk_sck_t *sck)
{
    log_debug(ssvr->log, "Recv pong!");

    sck->kpalive_times = 0;
    sck->next_kpalive_tm = time(NULL) + SDK_PING_MAX_SEC;

    return SDK_OK;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/******************************************************************************
 **函数名称: sdk_mesg_ping_handler
 **功    能: 处理PING命令
 **输入参数:
 **     ctx: 全局信息
 **     ssvr: Snd线程对象
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述:
 **注意事项: 收到服务端主动下发的PING, 客户端无需应答PONG, 而是直接发送PING请求.
 **作    者: # Qifeng.zou # 2016.11.08 20:42:56 #
 ******************************************************************************/
int sdk_mesg_ping_handler(sdk_cntx_t *ctx, sdk_ssvr_t *ssvr, sdk_sck_t *sck)
{
    sck->kpalive_times = 0;
    sck->next_kpalive_tm = time(NULL) + SDK_PING_MAX_SEC;
    sdk_mesg_send_ping_req(ctx, ssvr);
    return SDK_OK;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/******************************************************************************
 **函数名称: sdk_mesg_online_ack_handler
 **功    能: 处理ONLINE-ACK命令
 **输入参数:
 **     ctx: 全局信息
 **     ssvr: 读写线程对象
 **     sck: 通信套接字
 **     addr: 读取的数据
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述:
 **注意事项:
 **作    者: # Qifeng.zou # 2016.11.08 20:39:40 #
 ******************************************************************************/
int sdk_mesg_online_ack_handler(sdk_cntx_t *ctx, sdk_ssvr_t *ssvr, sdk_sck_t *sck, void *addr)
{
    OnlineAck *ack;
    mesg_header_t *head = (mesg_header_t *)addr;

    ack = online_ack__unpack(NULL, head->len, (void *)(head + 1));
    if (NULL == ack) {
        log_error(ctx->log, "Unpack online ack failed!");
        return SDK_ERR;
    }

    SDK_SSVR_SET_ONLINE(ssvr, ((ack->has_code && (0 == ack->code))? true : false));

    log_debug(ctx->log, "code:%d msg:%s", ack->code, ack->msg);

    online_ack__free_unpacked(ack, NULL);

    return SDK_OK;
}
