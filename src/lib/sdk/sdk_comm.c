#include "sdk.h"
#include "redo.h"
#include "rb_tree.h"

/******************************************************************************
 **函数名称: sdk_queue_init
 **功    能: 队列初始化(内部接口)
 **输入参数:
 **     q: 队列
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述:
 **注意事项:
 **作    者: # Qifeng.zou # 2016.11.09 17:47:09 #
 ******************************************************************************/
int sdk_queue_init(sdk_queue_t *q)
{
    pthread_mutex_init(&q->lock, NULL);
    q->list = list_creat(NULL);
    if (NULL == q->list) {
        pthread_mutex_destroy(&q->lock);
        return -1;
    }

    return 0;
}

/******************************************************************************
 **函数名称: sdk_queue_length
 **功    能: 获取队列长度
 **输入参数:
 **     q: 队列
 **输出参数: NONE
 **返    回: 队列长度
 **实现描述:
 **注意事项:
 **作    者: # Qifeng.zou # 2016.11.09 17:47:16 #
 ******************************************************************************/
int sdk_queue_length(sdk_queue_t *q)
{
    int len;

    pthread_mutex_lock(&q->lock);
    len = list_length(q->list);
    pthread_mutex_unlock(&q->lock);

    return len;
}

/******************************************************************************
 **函数名称: sdk_queue_rpush
 **功    能: 插入队尾
 **输入参数:
 **     q: 队列
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **注意事项:
 **作    者: # Qifeng.zou # 2016.11.09 17:47:23 #
 ******************************************************************************/
int sdk_queue_rpush(sdk_queue_t *q, void *data)
{
    int ret;

    pthread_mutex_lock(&q->lock);
    ret = list_lpush(q->list, data);
    pthread_mutex_unlock(&q->lock);

    return ret;
}

/******************************************************************************
 **函数名称: sdk_queue_lpop
 **功    能: 弹队头
 **输入参数:
 **     q: 队列
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **注意事项:
 **作    者: # Qifeng.zou # 2016.11.09 17:47:23 #
 ******************************************************************************/
void *sdk_queue_lpop(sdk_queue_t *q)
{
    void *data;

    pthread_mutex_lock(&q->lock);
    data = list_lpop(q->list);
    pthread_mutex_unlock(&q->lock);

    return data;
}

/******************************************************************************
 **函数名称: sdk_queue_remove
 **功    能: 移除某数据
 **输入参数:
 **     q: 队列
 **     data: 数据指针
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **注意事项:
 **作    者: # Qifeng.zou # 2016.11.10 14:34:11 #
 ******************************************************************************/
int sdk_queue_remove(sdk_queue_t *q, void *data)
{
    int ret;

    pthread_mutex_lock(&q->lock);
    ret = list_remove(q->list, data);
    pthread_mutex_unlock(&q->lock);

    return ret;
}

/******************************************************************************
 **函数名称: sdk_queue_empty
 **功    能: 队列是否为空
 **输入参数:
 **     q: 队列
 **输出参数: NONE
 **返    回: true:空 false:非空
 **实现描述: 
 **注意事项:
 **作    者: # Qifeng.zou # 2016.11.09 17:52:58 #
 ******************************************************************************/
bool sdk_queue_empty(sdk_queue_t *q)
{
    bool ret;

    pthread_mutex_lock(&q->lock);
    ret = list_empty(q->list);
    pthread_mutex_unlock(&q->lock);

    return ret;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/* 比较回调 */
static int sdk_send_mgr_cmp_cb(sdk_send_item_t *item1, sdk_send_item_t *item2)
{
    return item1->seq - item2->seq;
}

/******************************************************************************
 **函数名称: sdk_send_mgr_init
 **功    能: 初始化发送管理
 **输入参数:
 **     ctx: 全局对象
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **注意事项:
 **作    者: # Qifeng.zou # 2016.11.10 10:06:53 #
 ******************************************************************************/
int sdk_send_mgr_init(sdk_cntx_t *ctx)
{
    sdk_send_mgr_t *mgr = &ctx->mgr;

    mgr->seq = 0;
    pthread_rwlock_init(&mgr->lock, NULL);
    mgr->tab = rbt_creat(NULL, (cmp_cb_t)sdk_send_mgr_cmp_cb);
    if (NULL == mgr->tab) {
        return -1;
    }

    return 0;
}

/******************************************************************************
 **函数名称: sdk_gen_seq
 **功    能: 生成序列号
 **输入参数:
 **     ctx: 全局对象
 **输出参数: NONE
 **返    回: 序列号
 **实现描述: 
 **注意事项:
 **作    者: # Qifeng.zou # 2016.11.09 17:52:58 #
 ******************************************************************************/
uint64_t sdk_gen_seq(sdk_cntx_t *ctx)
{
    uint64_t seq;
    sdk_send_mgr_t *mgr = &ctx->mgr;

    pthread_rwlock_wrlock(&mgr->lock);
    seq = ++mgr->seq;
    pthread_rwlock_unlock(&mgr->lock);

    return seq;
}

/******************************************************************************
 **函数名称: sdk_send_mgr_insert
 **功    能: 新增发送项
 **输入参数:
 **     ctx: 全局对象
 **     item: 发送项
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **注意事项:
 **作    者: # Qifeng.zou # 2016.11.10 10:09:45 #
 ******************************************************************************/
int sdk_send_mgr_insert(sdk_cntx_t *ctx, sdk_send_item_t *item)
{
    int ret;
    sdk_send_mgr_t *mgr = &ctx->mgr;

    pthread_rwlock_wrlock(&mgr->lock);
    ret = rbt_insert(mgr->tab, item);
    pthread_rwlock_unlock(&mgr->lock);

    return ret;
}

/******************************************************************************
 **函数名称: sdk_send_mgr_delete
 **功    能: 删除发送项
 **输入参数:
 **     ctx: 全局对象
 **     set: 序列号
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **注意事项:
 **作    者: # Qifeng.zou # 2016.11.10 10:09:45 #
 ******************************************************************************/
int sdk_send_mgr_delete(sdk_cntx_t *ctx, uint64_t seq)
{
    sdk_send_item_t key, *item;
    sdk_send_mgr_t *mgr = &ctx->mgr;

    key.seq = seq;

    pthread_rwlock_wrlock(&mgr->lock);
    rbt_delete(mgr->tab, (void *)&key, (void **)&item);
    pthread_rwlock_unlock(&mgr->lock);
    if (NULL == item) {
        return -1;
    }

    free(item->data);
    free(item);

    return 0;
}

/******************************************************************************
 **函数名称: sdk_send_item_timeout_hdl
 **功    能: 超时发送项的处理
 **输入参数:
 **     ctx: 全局对象
 **     item: 发送单元
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **注意事项:
 **作    者: # Qifeng.zou # 2016.11.10 15:04:50 #
 ******************************************************************************/
static int sdk_send_item_timeout_hdl(sdk_cntx_t *ctx, sdk_send_item_t *item)
{
    void *data;
    sdk_send_item_t key, *temp;
    sdk_send_mgr_t *mgr = &ctx->mgr;

    data = (void *)(item->data + sizeof(mesg_header_t));

    switch (item->stat) {
        case SDK_STAT_IN_SENDQ: /* 依然在发送队列 */
            key.seq = item->seq;
            rbt_delete(mgr->tab, (void *)&key, (void **)&temp);
            sdk_queue_remove(&ctx->sendq, item->data);
            item->stat = SDK_STAT_SEND_TIMEOUT; /* 发送超时 */
            if (item->cb) {
                item->cb(item->cmd, data, item->len, item->stat, item->param);
            }
            FREE(item->data);
            FREE(item);
            return SDK_OK;
        case SDK_STAT_SENDING:  /* 正在发送中...(无法撤回) */
            return SDK_OK;
        case SDK_STAT_SEND_SUCC: /* 已发送成功 */
            key.seq = item->seq;
            rbt_delete(mgr->tab, (void *)&key, (void **)&temp);
            item->stat = SDK_STAT_ACK_TIMEOUT;
            if (item->cb) {
                item->cb(item->cmd, data, item->len, item->stat, item->param);
            }
            FREE(item->data);
            FREE(item);
            return SDK_OK;
        case SDK_STAT_SEND_FAIL:
            key.seq = item->seq;
            rbt_delete(mgr->tab, (void *)&key, (void **)&temp);
            if (item->cb) {
                item->cb(item->cmd, data, item->len, item->stat, item->param);
            }
            FREE(item->data);
            FREE(item);
            return SDK_OK;
        case SDK_STAT_SEND_TIMEOUT:
            key.seq = item->seq;
            rbt_delete(mgr->tab, (void *)&key, (void **)&temp);
            if (item->cb) {
                item->cb(item->cmd, data, item->len, item->stat, item->param);
            }
            FREE(item->data);
            FREE(item);
            return SDK_OK;
        case SDK_STAT_ACK_SUCC:
            key.seq = item->seq;
            rbt_delete(mgr->tab, (void *)&key, (void **)&temp);
            if (item->cb) {
                item->cb(item->cmd, data, item->len, item->stat, item->param);
            }
            FREE(item->data);
            FREE(item);
            return SDK_OK;
        case SDK_STAT_ACK_TIMEOUT:
            key.seq = item->seq;
            rbt_delete(mgr->tab, (void *)&key, (void **)&temp);
            if (item->cb) {
                item->cb(item->cmd, data, item->len, item->stat, item->param);
            }
            FREE(item->data);
            FREE(item);
            return SDK_OK;
    }

    return SDK_OK;
}

/* 获取超时发送项 */
static int sdk_send_mgr_trav_timeout_cb(sdk_send_item_t *item, list_t *list)
{
    time_t tm = time(NULL);

    if (tm < item->ttl) {
        return 0; /* 未超时 */
    }
    else if (SDK_STAT_SENDING == item->stat) {
        return 0;
    }

    list_rpush(list, item);

    return 0;
}

/******************************************************************************
 **函数名称: sdk_trav_send_item
 **功    能: 遍历发送项
 **输入参数:
 **     ctx: 全局对象
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **注意事项:
 **作    者: # Qifeng.zou # 2016.11.10 10:23:34 #
 ******************************************************************************/
int sdk_trav_send_item(sdk_cntx_t *ctx)
{
    list_t *list;
    time_t tm = time(NULL);
    sdk_send_item_t *item;
    sdk_send_mgr_t *mgr = &ctx->mgr;

    if (tm - mgr->trav_tm < 3) {
        return 0; /* 未超时 */
    }

    list = list_creat(NULL);
    if (NULL == list) {
        log_error(ctx->log, "Create timeout list failed!");
        return -1;
    }

    pthread_rwlock_wrlock(&mgr->lock);
    rbt_trav(mgr->tab, (trav_cb_t)sdk_send_mgr_trav_timeout_cb, (void *)list);
    while (1) {
        item = list_lpop(list);
        if (NULL == item) {
            break;
        }

        sdk_send_item_timeout_hdl(ctx, item);
    }
    list_destroy(list, NULL, mem_dummy_dealloc);
    pthread_rwlock_unlock(&mgr->lock);

    return 0;
}

/******************************************************************************
 **函数名称: sdk_send_mgr_query
 **功    能: 更新发送项
 **输入参数:
 **     ctx: 全局对象
 **     set: 序列号
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **注意事项:
 **作    者: # Qifeng.zou # 2016.11.10 10:09:45 #
 ******************************************************************************/
sdk_send_item_t *sdk_send_mgr_query(sdk_cntx_t *ctx, uint64_t seq, lock_e lock)
{
    sdk_send_item_t key, *item;
    sdk_send_mgr_t *mgr = &ctx->mgr;

    key.seq = seq;

    if (RDLOCK == lock) {
        pthread_rwlock_rdlock(&mgr->lock);
    }
    else if (WRLOCK == lock) {
        pthread_rwlock_wrlock(&mgr->lock);
    }

    item = rbt_query(mgr->tab, (void *)&key);
    if (NULL == item) {
        pthread_rwlock_unlock(&mgr->lock);
        return NULL;
    }

    return item;
}

/******************************************************************************
 **函数名称: sdk_send_mgr_unlock
 **功    能: 解锁发送项
 **输入参数:
 **     ctx: 全局对象
 **     set: 序列号
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **注意事项:
 **作    者: # Qifeng.zou # 2016.11.10 10:23:34 #
 ******************************************************************************/
int sdk_send_mgr_unlock(sdk_cntx_t *ctx, lock_e lock)
{
    sdk_send_mgr_t *mgr = &ctx->mgr;

    pthread_rwlock_unlock(&mgr->lock);

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/******************************************************************************
 **函数名称: sdk_send_succ_hdl
 **功    能: 发送成功后的处理
 **输入参数:
 **     ctx: 全局对象
 **     set: 序列号
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **注意事项:
 **作    者: # Qifeng.zou # 2016.11.10 10:23:34 #
 ******************************************************************************/
int sdk_send_succ_hdl(sdk_cntx_t *ctx, void *addr, size_t len)
{
    int cmd;
    void *data;
    uint64_t seq;
    sdk_send_item_t *item;
    mesg_header_t *head = (mesg_header_t *)addr;

    cmd = ntohs(head->cmd);
    seq = ntoh64(head->seq);

    item = sdk_send_mgr_query(ctx, seq, WRLOCK);
    if (NULL == item) {
        return 0;
    }

    item->stat = SDK_STAT_SEND_SUCC;
    if (item->cb) {
        data = (void *)(head + 1);
        item->cb(cmd, data, item->len, item->stat, item->param);
    }

    sdk_send_mgr_unlock(ctx, WRLOCK);

    return 0;
}

/******************************************************************************
 **函数名称: sdk_send_fail_hdl
 **功    能: 发送失败后的处理
 **输入参数:
 **     ctx: 全局对象
 **     addr: 序列号
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **注意事项:
 **作    者: # Qifeng.zou # 2016.11.10 10:23:34 #
 ******************************************************************************/
int sdk_send_fail_hdl(sdk_cntx_t *ctx, void *addr, size_t len)
{
    int cmd;
    void *data;
    uint64_t seq;
    sdk_send_item_t *item;
    mesg_header_t *head = (mesg_header_t *)addr;

    cmd = ntohs(head->cmd);
    seq = ntoh64(head->seq);

    /* > 更新发送状态 */
    item = sdk_send_mgr_query(ctx, seq, WRLOCK);
    if (NULL == item) {
        return 0;
    }

    item->stat = SDK_STAT_SEND_FAIL;
    if (item->cb) {
        data = (void *)(head + 1);
        item->cb(cmd, data, item->len, item->stat, item->param);
    }

    sdk_send_mgr_unlock(ctx, WRLOCK);

    /* > 删除失败数据 */
    sdk_send_mgr_delete(ctx, seq);

    return 0;
}

/******************************************************************************
 **函数名称: sdk_send_timeout_hdl
 **功    能: 发送超时的处理
 **输入参数:
 **     ctx: 全局对象
 **     addr: 处理数据(头+体)
 **输出参数: NONE
 **返    回: true:已超时 false:未超时
 **实现描述: 
 **注意事项: 此时协议头依然为主机字节序
 **作    者: # Qifeng.zou # 2016.11.10 11:48:21 #
 ******************************************************************************/
bool sdk_send_timeout_hdl(sdk_cntx_t *ctx, void *addr)
{
    void *data;
    sdk_send_item_t *item;
    mesg_header_t *head = (mesg_header_t *)addr;

    item = sdk_send_mgr_query(ctx, head->seq, WRLOCK);
    if (time(NULL) >= item->ttl) {
        if (item->cb) {
            data = (void *)(head + 1);
            item->cb(head->cmd, data, head->len, SDK_STAT_SEND_TIMEOUT, item->param);
        }
        item->stat = SDK_STAT_SEND_TIMEOUT;
        sdk_send_mgr_unlock(ctx, WRLOCK);

        sdk_send_mgr_delete(ctx, head->seq);
        return true;
    }
    item->stat = SDK_STAT_SENDING;
    sdk_send_mgr_unlock(ctx, WRLOCK);

    return false;
}

/******************************************************************************
 **函数名称: sdk_ack_succ_hdl
 **功    能: 应答成功的处理
 **输入参数:
 **     ctx: 全局对象
 **     seq: 序列号
 **输出参数: NONE
 **返    回: 0:成功 !0:失败
 **实现描述: 
 **注意事项: 此时协议头依然为网络字节序
 **作    者: # Qifeng.zou # 2016.11.10 11:48:21 #
 ******************************************************************************/
int sdk_ack_succ_hdl(sdk_cntx_t *ctx, uint64_t seq)
{
    void *data;
    sdk_send_item_t key, *item;
    sdk_send_mgr_t *mgr = &ctx->mgr;

    key.seq = seq;

    pthread_rwlock_wrlock(&mgr->lock);
    rbt_delete(mgr->tab, (void *)&key, (void **)&item);
    pthread_rwlock_unlock(&mgr->lock);
    if (NULL == item) {
        return -1;
    }

    data = (void *)(item->data + sizeof(mesg_header_t));
    item->cb(item->cmd, data, item->len, SDK_STAT_ACK_SUCC, item->param);

    free(item->data);
    free(item);

    return false;
}
