/**
 * @file fc_com.h
 * @author fool_cat (2696652257@qq.com)
 * @brief
 * @version 1.0
 * @date 2025-10-28
 *
 * @copyright Copyright (c) 2025
 *
 */

// 优化后的定义（通用二进制协议）
#define FRAME_HEAD (0xAA)
#define FRAME_END (0x55)
#define FRAME_ESCAPE (0x7D)  // 转义符

// 转义规则需同步调整（避开帧头/帧尾和转义符）
#define ENCODE_CHAR(p, c)                                                       \
    do                                                                          \
    {                                                                           \
        if (((c) == FRAME_HEAD) || ((c) == FRAME_END) || ((c) == FRAME_ESCAPE)) \
        {                                                                       \
            *p++ = FRAME_ESCAPE;                                                \
            *p++ = (c) ^ 0x20; /* 异或转义*/                                    \
        }                                                                       \
        else                                                                    \
        {                                                                       \
            *p++ = (c);                                                         \
        }                                                                       \
    } while (0)

#include <stdint.h>
#include <stdbool.h>

#define FRAME_HEAD (0x7E)
#define FRAME_END (0x7F)
#define FRAME_ESCAPE (0x7D)
#define FRAME_ESCAPE_OFFSET (0x20)

typedef struct
{
    uint16_t frameNum;         // 帧号
    uint8_t  type;             // 帧类型
    uint8_t *pData;            // 数据载荷指针
    uint16_t dataLen;          // 数据长度
    uint8_t  cmdFrameNumPre;   // 扩展字段（可选）
    uint8_t  cmdFrameNumBack;  // 扩展字段（可选）
} BMsg_t;

/**
 * @brief  解码单个字节（处理转义）
 * @input  pSrc: 源数据指针
 *         pDst: 解码后字节输出指针
 * @retval 是否成功解码（true: 成功，false: 需要继续处理转义）
 */
static bool DecodeByte(const uint8_t **pSrc, uint8_t *pDst)
{
    if (**pSrc == FRAME_ESCAPE)
    {
        (*pSrc)++;                               // 跳过转义符
        *pDst = (**pSrc) + FRAME_ESCAPE_OFFSET;  // 还原原始字符
        (*pSrc)++;
        return true;
    }
    else if (**pSrc == FRAME_HEAD || **pSrc == FRAME_END)
    {
        return false;  // 非法字符（未转义的帧头/帧尾）
    }
    else
    {
        *pDst = **pSrc;  // 直接拷贝
        (*pSrc)++;
        return true;
    }
}

/**
 * @brief  解码完整帧
 * @input  pPacket: 原始数据包（含转义字符）
 *         packetLen: 数据包长度
 *         pMsg: 解码后的消息结构体
 * @output 解码成功的数据长度（含帧头帧尾）
 * @retval 是否成功（true: 成功，false: CRC校验失败或格式错误）
 */
bool BLink_UnpackFrame(const uint8_t *pPacket, uint16_t packetLen, BMsg_t *pMsg)
{
    const uint8_t *ptr = pPacket;
    uint16_t       crc = CRC_INIT_VALUE;
    uint16_t       calculatedCrc, receivedCrc;
    uint8_t        ch;
    uint16_t       frameLen;

    // 1. 检查帧头
    if (*ptr++ != FRAME_HEAD)
    {
        return false;
    }

    // 2. 解码长度字段（大端序）
    if (!DecodeByte(&ptr, &ch))
        return false;
    frameLen = ch << 8;
    if (!DecodeByte(&ptr, &ch))
        return false;
    frameLen |= ch;

    // 3. 解码帧号
    if (!DecodeByte(&ptr, &ch))
        return false;
    pMsg->frameNum = ch << 8;
    if (!DecodeByte(&ptr, &ch))
        return false;
    pMsg->frameNum |= ch;

    // 4. 解码类型
    if (!DecodeByte(&ptr, &ch))
        return false;
    pMsg->type = ch;

    // 5. 解码扩展字段（若为特定类型）
    if (BCOM_GET_TYPE(pMsg->type) == BCOM_FRAME_RESULT ||
        BCOM_GET_TYPE(pMsg->type) == BCOM_FRAME_REPORT)
    {
        if (!DecodeByte(&ptr, &ch))
            return false;
        pMsg->cmdFrameNumPre = ch;
        if (!DecodeByte(&ptr, &ch))
            return false;
        pMsg->cmdFrameNumBack = ch;
    }

    // 6. 解码数据载荷
    pMsg->dataLen = frameLen - (ptr - pPacket) - 4;  // 减去已解码字段和CRC+帧尾
    for (uint16_t i = 0; i < pMsg->dataLen; i++)
    {
        if (!DecodeByte(&ptr, &pMsg->pData[i]))
            return false;
    }

    // 7. 解码CRC（大端序）
    if (!DecodeByte(&ptr, &ch))
        return false;
    receivedCrc = ch << 8;
    if (!DecodeByte(&ptr, &ch))
        return false;
    receivedCrc |= ch;

    // 8. 检查帧尾
    if (*ptr++ != FRAME_END)
    {
        return false;
    }

    // 9. CRC校验（需与编码时相同的计算逻辑）
    calculatedCrc = BlinkCRCStr(CRC_INIT_VALUE, pPacket + 1, frameLen - 4);  // 跳过帧头和CRC自身
    if (calculatedCrc != receivedCrc)
    {
        return false;
    }

    return true;
}

#define IS_LITTLE_ENDIAN ({ \
    union                   \
    {                       \
        uint16_t i;         \
        uint8_t  c[2];      \
    } u = {0x1234};         \
    u.c[0] == 0x34;         \
})

// 判断系统字节序（编译期）
#define IS_LITTLE_ENDIAN (*(uint16_t *)"\0\xFF" < 0x100)

// 手动实现htonl
uint32_t custom_htonl(uint32_t host)
{
    if (IS_LITTLE_ENDIAN)
    {
        return ((host & 0xFF) << 24) | ((host & 0xFF00) << 8) |
               ((host >> 8) & 0xFF00) | ((host >> 24) & 0xFF);
    }
    return host;  // 大端系统无需转换
}

struct vla_container
{
    size_t  len;
    uint8_t data[];  // 柔性数组（必须是最后一个成员）
};
