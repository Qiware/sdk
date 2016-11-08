#include "str.h"
#include "sdk.h"
#include "redo.h"
#include "queue.h"
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
    sdk_sct_t *sck = &ssvr->sck;
    wiov_t *send = &ssvr->sck.send;
    sdk_conn_info_t *info = &ssvr->conn_info;

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
    head->from = info->sessionid;

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
int sdk_mesg_pong_handler(sdk_cntx_t *ctx, sdk_ssvr_t *ssvr, sdk_sct_t *sck)
{
    sck->kpalive_times = 0;
    sdk_set_kpalive_stat(sck, SDK_KPALIVE_STAT_SUCC);
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
 **注意事项:
 **作    者: # Qifeng.zou # 2016.11.08 20:42:56 #
 ******************************************************************************/
int sdk_mesg_ping_handler(sdk_cntx_t *ctx, sdk_ssvr_t *ssvr, sdk_sct_t *sck)
{
    sck->kpalive_times = 0;
    sdk_set_kpalive_stat(sck, SDK_KPALIVE_STAT_SUCC);
    sdk_mesg_send_ping_req(ctx, ssvr);
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
    head->from = info->sessionid;

    mesg__online__pack(&online, addr+sizeof(mesg_header_t));

    /* 3. 加入发送列表 */
    if (list_rpush(sck->mesg_list, addr)) {
        free(addr);
        log_error(ssvr->log, "Insert list failed!");
        return SDK_ERR;
    }

    log_debug(ssvr->log, "Add online request success! fd:[%d]", sck->fd);

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
int sdk_mesg_online_ack_handler(sdk_cntx_t *ctx, sdk_ssvr_t *ssvr, sdk_sct_t *sck, void *addr)
{
    return SDK_OK;
}