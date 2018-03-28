//
// Created by mj on 18-3-27.
//

#ifndef KROOMAV_RTMPPUSHER_H
#define KROOMAV_RTMPPUSHER_H


#include "librtmp/rtmp.h"

namespace vvav{
#define RTMP_HEAD_SIZE  (sizeof(RTMPPacket) + RTMP_MAX_HEADER_SIZE)

#define NALU_SLICE      1
#define NALU_SLICE_DPA  2
#define NALU_SLICE_DPB  3
#define NALU_SLICE_DPC  4
#define NALU_SLICE_IDR  5
#define NALU_SEI        6
#define NALU_SPS        7
#define NALU_PPS        8
#define NALU_AUD        9
#define NALU_FILTER     12

#define STREAM_CHANNEL_METADATA     0X03
#define STREAM_CHANNEL_VIDEO        0X04
#define STREAM_CHANNEL_AUDIO        0X05


class RtmpPusher {
public:
    RtmpPusher();

    /**
     * 初始化 librtmp
     * @param url rtmp服务器地址
     * @param timeout  超时时间
     * @return 0 onSuccess; -1 on Error
     */
    int init(const char* url, int timeout);

    /**
     * 发送 sps 和 pps 需要在第一个视频包之前发送
     * @param sps           sps data,without startcode
     * @param spsLength     sps length
     * @param pps           pps data, without startcode
     * @param ppsLength     pps length
     * @return 0 onSuccess; -1 on Error
     */
    int sendSpsAndPps(uint8_t* sps, uint32_t spsLength, uint8_t* pps, uint32_t ppsLength);

    /**
     * 发送视频数据 包含startcode
     * @param data          // NALU data
     * @param length        // NALU length
     * @param timestamp     // 时间戳
     * @return 0 onSuccess; -1 on Error
     */
    int sendVideoData(uint8_t* data, uint32_t length, int64_t timestamp);

    /**
     * 发送AAC音频序列头,需要在第一个音频包之前发送
     * @param data      音频序列头
     * @param length    音频序列长度 应该固定为2
     * @return 0 onSuccess; -1 on Error
     */
    int sendAacSequenceHeader(uint8_t *data, uint32_t length);

    /**
     * 发送AAC音频数据数据
     * @param data          AAC RAW data,  without adts
     * @param length        AAC Raw data length
     * @param timestamp     时间戳
     * @return 0 onSuccess; -1 on Error
     */
    int sendAudioData(uint8_t* data, uint32_t length, int64_t timestamp);

    /**
     * 关闭rmtp 推流
     * @return 0 onSuccess; -1 on Error
     */
    int stop();

private:
    RTMP* rtmp;
};

} //namespace vvav

#endif //KROOMAV_RTMPPUSHER_H
