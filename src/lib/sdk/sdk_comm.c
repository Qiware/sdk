#include "comm.h"
#include "lock.h"
#include "redo.h"
#include "syscall.h"
#include "sdk.h"
#include "sdk_mesg.h"

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

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
