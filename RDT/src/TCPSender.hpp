//
// Created by yaning on 18-11-30.
//

#ifndef STOP_WAIT_TCPSENDER_HPP
#define STOP_WAIT_TCPSENDER_HPP

#include "RdtSender.h"
#include "config.h"


/*
在实现 GBN 协议的基础上,根据 TCP 的可靠数据传输机制(包括超时后只重传最
早发送且没被确认的报文、快速重传)实现一个简化版的 TCP 协议。报文段格式、报文段
序号编码方式和 GBN 协议一样保持不变,不考虑流量控制、拥塞控制,不需要估算 RTT 动
态调整定时器 Timeout 参数。分值 20%
*/
class TCPSender : public RdtSender {
    std::unique_ptr<int[]> acknums;
    // 要求基于GBN编写？？？
    std::unique_ptr<Packet[]> buff;
    int base, next_seqnum;
    int mod;
    int ack_seq;

public:
    TCPSender() {
        mod = 1 << BITS;
        base = next_seqnum = 0;
        buff = std::make_unique<Packet[]>(mod);
        // 只需要3个int记录冗余ack
        acknums = std::make_unique<int[]>(3);
        std::fill(acknums.get(), acknums.get() + 3, -1);
        ack_seq = 0;
    }

    bool send(Message &message) override {
        // 缓冲区判断
        if ((next_seqnum - base + mod) % mod == WINDOW_WIDTH) {
            return false;
        }
        auto &pkt = buff[next_seqnum];
        pkt.acknum = -1;
        pkt.seqnum = next_seqnum;
        // 拷贝数据
        memcpy(pkt.payload, message.data, sizeof(message.data));
        pkt.checksum = pUtils->calculateCheckSum(pkt);
        if (next_seqnum == base) {
            // 开启定时器
            pns->startTimer(SENDER, Configuration::TIME_OUT, 0);
        }
        // base不动，next动
        next_seqnum = (next_seqnum + 1) % mod;
        return true;
    }

    void receive(Packet &ackPkt) override {
        // 这里多了个快速重传
        int checksum = pUtils->calculateCheckSum(ackPkt);
        if (checksum == ackPkt.checksum) {
            // 最早未确认的包
            if (ackPkt.acknum == base) {
                // 向后传递
                base = (base + 1) % mod;
                pns->stopTimer(SENDER, 0);
                if (base != next_seqnum) {
                    pns->startTimer(SENDER, Configuration::TIME_OUT, 0);
                }
            } else {
                // 记录重复的ack
                acknums[ack_seq] = ackPkt.acknum;
                ack_seq = (ack_seq + 1) % 3;
                if (std::all_of(acknums.get(), acknums.get() + 3, [&](int i) {
                    return acknums[0] == i;
                })) {
                    // 快速重传最早未确认包
                    pns->sendToNetworkLayer(RECEIVER, buff[base]);
                }
            }
        }
    }

    void timeoutHandler(int seqNum) override {
        // 重传最早未确认包
        pns->sendToNetworkLayer(RECEIVER, buff[base]);
        pns->stopTimer(SENDER, 0);
        pns->startTimer(SENDER, Configuration::TIME_OUT, 0);
    }

    bool getWaitingState() override {
        return (next_seqnum - base + mod) % mod == WINDOW_WIDTH;
    }
};


#endif //STOP_WAIT_TCPSENDER_HPP
