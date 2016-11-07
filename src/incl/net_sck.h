/******************************************************************************
 ** Coypright(C) 2014-2024 Qiware technology Co., Ltd
 **
 ** 文件名: net_sck.h
 ** 版本号: 1.0
 ** 描  述: 
 ** 作  者: # Qifeng.zou # 2015年11月10日 星期二 14时59分56秒 #
 ******************************************************************************/
#if !defined(__NET_SCK_H__)
#define __NET_SCK_H__

/* 16位字节序翻转 */
#define swap16(x) ((((uint16_t)(x) & 0xff) << 8) | (((uint16_t)(x) >> 8) & 0xff))

/* 32位字节序翻转 */
#define swap32(x) ((uint32_t)(x) >> 24) | \
                    (((uint32_t)(x) & 0x00ff0000) >> 8)  |\
                    ((((uint32_t)x) & 0x0000ff00) << 8)  | \
                    (((uint32_t)(x) << 24))

/* 64位字节序翻转 */
#define swap64(v) ((uint64_t)( \
        (uint64_t)(v) & 0x00000000000000FF) << 56 \
        | ((uint64_t)(v) & 0x000000000000FF00) << 40 \
        | ((uint64_t)(v) & 0x0000000000FF0000) << 24 \
        | ((uint64_t)(v) & 0x00000000FF000000) << 8 \
        | ((uint64_t)(v) & 0x000000FF00000000) >> 8 \
        | ((uint64_t)(v) & 0x0000FF0000000000) >> 24 \
        | ((uint64_t)(v) & 0x00FF000000000000) >> 40 \
        | ((uint64_t)(v) & 0xFF00000000000000) >> 56)

/* CPU为小端 则进行转换  */
#if __BYTE_ORDER == __LITTLE_ENDIAN 

#define ntoh64(x) swap64(x)
#define hton64(x) swap64(x)
#define ntoh32(x) swap32(x)
#define hton32(x) swap32(x)
#define ntoh16(x) swap16(x)
#define hton16(x) swap16(x)

/* CPU 为大端 不需要转换 */
#else

#define ntoh64(x) (x)
#define hton64(x) (x)
#define ntoh32(x) (x)
#define hton32(x) (x)
#define ntoh16(x) (x)
#define hton16(x) (x)

#endif

#endif /*__NET_SCK_H__*/

