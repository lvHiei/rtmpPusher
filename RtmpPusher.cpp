//
// Created by mj on 18-3-27.
//

#include <malloc.h>
#include "librtmp/log.h"
//#include "../util/logUtil.h"
#include "RtmpPusher.h"

#include <string.h>

#ifdef ANDROID_PROJECT
#include <android/log.h>

#define TAG "RTMPPUSHER"

#define LOGD(LOGFMT, ...) __android_log_print(ANDROID_LOG_DEBUG,TAG ,LOGFMT, ##__VA_ARGS__);
#define LOGI(LOGFMT, ...) __android_log_print(ANDROID_LOG_INFO,TAG ,LOGFMT, ##__VA_ARGS__);
#define LOGW(LOGFMT, ...) __android_log_print(ANDROID_LOG_WARN,TAG ,LOGFMT, ##__VA_ARGS__);
#define LOGE(LOGFMT, ...) __android_log_print(ANDROID_LOG_ERROR,TAG ,LOGFMT, ##__VA_ARGS__);

#else
#include <stdio.h>

#define LOGD   printf
#define LOGI   printf
#define LOGW   printf
#define LOGE   printf
#endif


#define MAX_PRINT_LEN   2048

namespace vvav {

    RtmpPusher::RtmpPusher():rtmp(NULL) {

    }

    static void my_rtmp_log_callback(int level, const char *format, va_list vl){
        char str[MAX_PRINT_LEN]="";

        vsnprintf(str, MAX_PRINT_LEN-1, format, vl);

        switch (level){
            case RTMP_LOGALL:
            case RTMP_LOGDEBUG2:
            case RTMP_LOGDEBUG:
                LOGD("%s", str);
                break;
            case RTMP_LOGINFO:
                LOGI("%s", str);
                break;
            case RTMP_LOGWARNING:
                LOGW("%s", str);
                break;
            case RTMP_LOGERROR:
            case RTMP_LOGCRIT:
                LOGE("%s", str);
                break;
            default:
                break;
        }
    }

    int RtmpPusher::init(const char *url, int timeout) {
        RTMP_LogSetLevel(RTMP_LOGDEBUG);
        RTMP_LogSetCallback(my_rtmp_log_callback);
        rtmp = RTMP_Alloc();
        RTMP_Init(rtmp);
        LOGI("rtmp timeout:%d", timeout);
        rtmp->Link.timeout = timeout;
        RTMP_SetupURL(rtmp, (char *) url);
        RTMP_EnableWrite(rtmp);

        int ret = RTMP_Connect(rtmp, NULL);
        if(!ret){
            LOGE("rtmp connect error, url:%s,ret:%d", url, ret);
            return -1;
        }
        LOGI("RTMP_Connect success.");

        ret = RTMP_ConnectStream(rtmp, 0);
        if(!ret){
            LOGE("rtmp connect stream error, url:%s, ret:%d", url, ret);
            return -1;
        }

        LOGI("RTMP_ConnectStream success.");
        return 0;
    }

    int
    RtmpPusher::sendSpsAndPps(uint8_t *sps, uint32_t spsLength, uint8_t *pps, uint32_t ppsLength) {
        int i = 0;
        RTMPPacket *packet = (RTMPPacket *) malloc(RTMP_HEAD_SIZE + 1024);
        memset(packet, 0, RTMP_HEAD_SIZE);
        packet->m_body = (char *) packet + RTMP_HEAD_SIZE;
        uint8_t * body = (uint8_t *) packet->m_body;

        /***
         * format dont need startcode, but with the NAL_TYPE
         * for example
         * SPS 0x67 0xXX 0xXX ...
         * PPS 0x68 0xXX 0xXX ...
         */


        /**
         * 参见flv video tag
         */
        i = 0;
        body[i++] = 0x17; //UB[4] FrameType:1(keyframe)  UB[4] CodecID:7(AVC)

        body[i++] = 0x00; //UI8 AVCPacketType:0 (AVC sequence header)

        body[i++] = 0x00;//SI24 CompositionTime
        body[i++] = 0x00;
        body[i++] = 0x00; //fill in 0

        /**
         * 参见14496-15 5.2.4.1, 14496-10 7.3.2.1
         */

        /*AVCDecoderConfigurationRecord*/
        body[i++] = 0x01;   //UI8 configurationVersion : 1
        body[i++] = sps[1]; //UI8 AVCProfileIndecation : profile_idc(14496-10 7.3.2.1) that's is sps[1]
        body[i++] = sps[2]; //UI8 profile_compatibilty : byte between profile_idc and level_idc, that's sps[2]
        body[i++] = sps[3]; //UI8 AVCLevelIndication   : level_idc that's is sps[3]
        body[i++] = 0xff;   //UB[6]:111111 (reversed)
                            // UB[2]:11 lengthSizeMinusOne the nalu length size is 4 Bytes(
                            // the Nalu Data Length is stored in SI32), so the value is 4-1=3

        body[i++] = 0xe1;   // UB[3]:111 (reversed) UB[5]:the number of sps, that is 1,so the Byte is 0b11100001(0xe1)

        body[i++] = (spsLength >> 8) & 0xff; // UI16: SPS length
        body[i++] = spsLength & 0xff;

        /*sps data*/
        memcpy(&body[i], sps, spsLength);// UI8[spsLength]: bytes of sps

        i += spsLength;

        /*PPS*/
        body[i++] = 0x01;   // UI8: the number of pps, that is 1

        /*sps data length*/
        body[i++] = (ppsLength >> 8) & 0xff; // UI16: PPS length
        body[i++] = ppsLength & 0xff;


        memcpy(&body[i], pps, ppsLength);// UI8[ppsLength]:bytes of pps
        i += ppsLength;

        packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
        packet->m_nBodySize = i;
        packet->m_nChannel = STREAM_CHANNEL_VIDEO;
        packet->m_nTimeStamp = 0;
        packet->m_hasAbsTimestamp = 0;
        packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
        packet->m_nInfoField2 = rtmp->m_stream_id;

        /*发送*/
        if (RTMP_IsConnected(rtmp)) {
            RTMP_SendPacket(rtmp, packet, TRUE);
        }
        free(packet);


        return 0;
    }

    int RtmpPusher::sendVideoData(uint8_t *data, uint32_t length, int64_t timestamp) {
        int type;

        /*去掉帧界定符*/
        if (data[2] == 0x00) {/*00 00 00 01*/
            data += 4;
            length -= 4;
        } else if (data[2] == 0x01) {
            data += 3;
            length - 3;
        }

        type = data[0] & 0x1f;

        RTMPPacket *packet = (RTMPPacket *) malloc(RTMP_HEAD_SIZE + length + 9);
        memset(packet, 0, RTMP_HEAD_SIZE);
        packet->m_body = (char *) packet + RTMP_HEAD_SIZE;
        packet->m_nBodySize = length + 9;


        /* send video packet*/
        uint8_t *body = (uint8_t *) packet->m_body;
        memset(body, 0, length + 9);


        /**
         * 参见flv tag
         */

        /*key frame*/
        body[0] = 0x27;         // UB[4] FrameType: 2(inter frame, a non-seekable frame);UB[4] CodecID:7(AVC)
        if (type == NALU_SLICE_IDR) {
            body[0] = 0x17; //关键帧 UB[4] FrameType: 1(key frame, a seekable frame);UB[4] CodecID:7(AVC)
        }

        body[1] = 0x01;     //UI8 AVCPacketType:1(AVC NALU)

        body[2] = 0x00;     //SI24 CompositionTime there isn't B frame,set it 0
        body[3] = 0x00;
        body[4] = 0x00;

        body[5] = (length >> 24) & 0xff;    // SI32 NALU length
        body[6] = (length >> 16) & 0xff;
        body[7] = (length >> 8) & 0xff;
        body[8] = (length) & 0xff;

        /*copy data*/
        memcpy(&body[9], data, length);     // UI[length] NALU data

        packet->m_hasAbsTimestamp = 0;
        packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
        packet->m_nInfoField2 = rtmp->m_stream_id;
        packet->m_nChannel = STREAM_CHANNEL_VIDEO;
        packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
        packet->m_nTimeStamp = timestamp;

        if (RTMP_IsConnected(rtmp)) {
            RTMP_SendPacket(rtmp, packet, TRUE);
        }
        free(packet);
        return 0;
    }

    int RtmpPusher::sendAacSequenceHeader(uint8_t *data, uint32_t length) {
        RTMPPacket *packet;
        uint8_t *body;
        int len = length;//spec len 是2
        packet = (RTMPPacket *) malloc(RTMP_HEAD_SIZE + len + 2);
        memset(packet, 0, RTMP_HEAD_SIZE);
        packet->m_body = (char *) packet + RTMP_HEAD_SIZE;
        body = (uint8_t *) packet->m_body;

        /**
         * 参见flv tag
         */

        /*AF 00 +AAC RAW data*/
        body[0] = 0xAF;     // UB[4]: SoundFormat 0x0A,that's is aac;UB[2]:SoundRate:For aac is 3;
                            // UB[1]: SoundSize 1 that's is 16bit;UB[1]:SoundType 1,that's Stereo
        body[1] = 0x00;     // UB[8]: AACPacketType 0 that's AAC sequence header
        memcpy(&body[2], data, len);// UI8[length]: the data of AAC sequence header(AudioSpecificConfig, according 14496-3)

        packet->m_packetType = RTMP_PACKET_TYPE_AUDIO;
        packet->m_nBodySize = len + 2;
        packet->m_nChannel = STREAM_CHANNEL_AUDIO;
        packet->m_nTimeStamp = 0;
        packet->m_hasAbsTimestamp = 0;
        packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
        packet->m_nInfoField2 = rtmp->m_stream_id;

        if (RTMP_IsConnected(rtmp)) {
            RTMP_SendPacket(rtmp, packet, TRUE);
        }
        free(packet);

        return 0;
    }

    int RtmpPusher::sendAudioData(uint8_t *data, uint32_t length, int64_t timestamp) {
    //    data += 5;
    //    len += 5;
        if (length > 0) {
            RTMPPacket *packet;
            uint8_t *body;
            packet = (RTMPPacket *) malloc(RTMP_HEAD_SIZE + length + 2);
            memset(packet, 0, RTMP_HEAD_SIZE);
            packet->m_body = (char *) packet + RTMP_HEAD_SIZE;
            body = (uint8_t *) packet->m_body;

            /**
             * 参见flv tag
             */

            /*AF 00 +AAC Raw data*/
            body[0] = 0xAF;     // UB[4]: SoundFormat 0x0A,that's is aac;UB[2]:SoundRate:For aac is 3;
                                // UB[1]: SoundSize 1 that's is 16bit;UB[1]:SoundType 1,that's Stereo
            body[1] = 0x01;     // UB[8]: AACPacketType 1 that's AAC raw
            memcpy(&body[2], data, length);// UI8[length]: the data of AAC raw data

            packet->m_packetType = RTMP_PACKET_TYPE_AUDIO;
            packet->m_nBodySize = length + 2;
            packet->m_nChannel = STREAM_CHANNEL_AUDIO;
            packet->m_nTimeStamp = timestamp;
            packet->m_hasAbsTimestamp = 0;
            packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
            packet->m_nInfoField2 = rtmp->m_stream_id;
            if (RTMP_IsConnected(rtmp)) {
                RTMP_SendPacket(rtmp, packet, TRUE);
            }
            LOGD("send packet body[0]=%x,body[1]=%x", body[0], body[1]);
            free(packet);

        }
        return 0;
    }

    int RtmpPusher::stop() {
        RTMP_Close(rtmp);
        RTMP_Free(rtmp);
        rtmp = NULL;
        return 0;
    }
} // namespace vvav
