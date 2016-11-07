/******************************************************************************
 ** Coypright(C) 2014-2024 Qiware technology Co., Ltd
 **
 ** 文件名: proto.h
 ** 版本号: 1.0
 ** 描  述: 
 ** 作  者: # Qifeng.zou # 2016年11月03日 星期四 10时00分15秒 #
 ******************************************************************************/
#if !defined(__PROTO_H__)
#define __PROTO_H__

// 命令类型
typedef enum
{
    CMD_PING                    = 0X0001        // c   c=client a=access u=upstream
    , CMD_PONG                  = 0X0002        // a
    , CMD_SUB                   = 0X0003        // a
    , CMD_SUBACK                = 0X0004        // u
    , CMD_RPSTATE               = 0X0005        // u
    , CMD_RPSTATE_ACK           = 0X0006        // u
    , CMD_SHORT_PING            = 0X0007
    , CMD_SHORT_PONG            = 0X0008
    , CMD_KEEPALIVE             = 0X0009        // 内部模块间TCP连接保活请求
    , CMD_KEEPALIVE_ACK         = 0X000A        // 内部模块间TCP连接保活应答

    , CMD_ONLINE                = 0X0101        // c
    , CMD_ONLINE_ACK            = 0X0102        // u
    , CMD_OFFLINE               = 0X0103        // c
    , CMD_OFFLINE_ACK           = 0X0104        // u

    // chat-room for
    , CMD_IM_JOIN               = 0X0105        
    , CMD_IM_JOINACK            = 0X0106
    , CMD_IM_UNJOIN             = 0X0107
    , CMD_IM_UNJOINACK          = 0X0108

    , CMD_NOTIFY                = 0X0201        // u
    , CMD_SYNC                  = 0X0202        // c
    , CMD_MSG                   = 0X0203        // c
    , CMD_MSG_ACK               = 0X0204        // u

    , CMD_MULTICAST             = 0X0205        // group-cast
    , CMD_MULTICASTACK          = 0X0206        // group-cast ack

    , CMD_PUSH_NOTIFY           = 0X0301        // u
    , CMD_PUSH_SYNC             = 0X0302        // c
    , CMD_PUSH_MSG              = 0X0303        // u c 
    , CMD_PUSH_MSG_ACK          = 0X0304        // u c 
    , CMD_PUSH_BROCST           = 0X0305
    , CMD_PUSH_BROCST_ACK       = 0X0306
    , CMD_PUSH_MULTCAST         = 0X0307
    , CMD_PUSH_OFFLINE_MSG      = 0X0308
    , CMD_PUSH_OFFLINE_MSG_ACK  = 0X0309

    // chat room for
    , CMD_IM_ROOM_MSG           = 0X0401
    , CMD_IM_ROOM_MSGACK        = 0X0402
    , CMD_ROOM_BROCST           = 0X0403
    , CMD_ROOM_BROCST_ACK       = 0X0404

    // User data
    , CMD_USR_DATA              = 0x0501 // 在线中心: 用户数据
    , CMD_USR_DATA_ACK          = 0x0502 // 在线中心: 用户数据应答
    , CMD_USR_PUSH_DATA         = 0x0503 // 在线中心: 下发数据
    , CMD_USR_PUSH_DATA_ACK     = 0x0504 // 在线中心: 下发数据应答
} mesg_type_e;

typedef struct
{
    uint16_t cmd;       // 协议类型，命令ID,定义规则：奇数是请求，偶数是应答
    uint16_t flag;      // 校验码
    uint32_t len;       // 消息body长度
    uint64_t from;      // 发送者连接sessionID
    uint64_t to;        // 目标者sessionID
    uint32_t seq;       // 消息序列号
    uint16_t mid;       // 内部通信时的模块ID
    uint16_t version;   // 版本
    uint32_t rsv1;      // 预留字段
    uint32_t rsv2;      // 预留字段
} mesg_header_t;

#define SDK_DATA_TOTAL_LEN(head) (head->len + sizeof(mesg_header_t))

#define SDK_HEAD_NTOH(s, d) do {\
    (d)->cmd = ntohs((s)->cmd); \
    (d)->flag = ntohl((s)->flag); \
    (d)->len = ntohl((s)->len); \
    (d)->from = ntoh64((s)->from); \
    (d)->to = ntoh64((s)->to); \
    (d)->seq = ntohl((s)->seq); \
    (d)->mid = ntohs((s)->mid); \
    (d)->version = ntohs((s)->version); \
    (d)->rsv1 = ntohl((s)->rsv1); \
    (d)->rsv2 = ntohl((s)->rsv2); \
} while(0)

#define SDK_HEAD_HTON(s, d) do {\
    (d)->cmd = htons((s)->cmd); \
    (d)->flag = htonl((s)->flag); \
    (d)->len = htonl((s)->len); \
    (d)->from = hton64((s)->from); \
    (d)->to = hton64((s)->to); \
    (d)->seq = htonl((s)->seq); \
    (d)->mid = htons((s)->mid); \
    (d)->version = htons((s)->version); \
    (d)->rsv1 = htonl((s)->rsv1); \
    (d)->rsv2 = htonl((s)->rsv2); \
} while(0)

/* 校验数据头 */
#define SDK_HEAD_ISVALID(head) (head->len > 1024*1024*1024)

#endif /*__PROTO_H__*/
