//
// Created by yaning on 18-11-27.
//
#include "config.h"
#include "RdtSender.h"

class GBNSender : public RdtSender {
    // 发送方需要设置缓冲区
    std::unique_ptr<Packet[]> buff;
    int base, next_seqnum, mod;
//    std::mutex lock;
public:
    GBNSender() {
        mod = 1 << BITS;
        buff = std::make_unique<Packet[]>(mod);
        base = 0;
        next_seqnum = 0;
    }

    bool send(Message &message) override {
        // 发送时检查缓冲区是否可以再放数据
        if ((next_seqnum - base + mod) % mod == WINDOW_WIDTH) {
            // 放不下了
            return false;
        }
        // 发送出去
        auto &pkt = buff[next_seqnum];
        // 无用
        pkt.acknum = -1;
        // 序号有用
        pkt.seqnum = next_seqnum;
        pkt.checksum = pUtils->calculateCheckSum(pkt);
        if (base == next_seqnum) {
            pns->startTimer(SENDER, Configuration::TIME_OUT, 0);
        }
        next_seqnum = (next_seqnum + 1) % mod;
        pns->sendToNetworkLayer(RECEIVER, pkt);
        // 指向下一个空的缓冲区
        return true;
    }

    void receive(Packet &ackPkt) override {
        int check_sum = pUtils->calculateCheckSum(ackPkt);
        if (check_sum == ackPkt.checksum) {
            base = (ackPkt.acknum + 1) % mod;
            if (base == next_seqnum) {
                pns->stopTimer(SENDER, 0);
            } else {
                pns->stopTimer(SENDER, 0);
                pns->startTimer(SENDER, Configuration::TIME_OUT, 0);
            }
        }
    }

    void timeoutHandler(int seqNum) override {
        // 发送所有未确认报文
        // 发送过程中不要改变base 和next_seqnum
        if (next_seqnum != base) {
            pns->startTimer(SENDER, Configuration::TIME_OUT, 0);
            for (int i = base; i != next_seqnum; i = (i + 1) % mod) {
                pns->sendToNetworkLayer(RECEIVER, buff[i]);
            }
        }
    }


    bool getWaitingState() override {
        return (next_seqnum - base + mod) % mod == WINDOW_WIDTH;
    }
};
