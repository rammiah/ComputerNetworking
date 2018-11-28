//
// Created by yaning on 18-11-28.
//

#ifndef STOP_WAIT_SRSENDER_HPP
#define STOP_WAIT_SRSENDER_HPP

#include "config.h"
#include "RdtSender.h"

class SRSender : public RdtSender {
    int mod;
    std::unique_ptr<Packet[]> buff;
    std::unique_ptr<bool[]> acked;
    int base, next_seqnum;

    bool is_valid_packet(int num) {
        if (base <= next_seqnum) {
            return num >= base && num < next_seqnum;
        }
        return num >= base || num < next_seqnum;
    }

public:
    SRSender() {
        mod = 1 << BITS;
        base = next_seqnum = 0;
        buff = std::make_unique<Packet[]>(mod);
        acked = std::make_unique<bool[]>(mod);
        std::fill(acked.get(), acked.get() + mod, false);
    }

    bool send(Message &message) override {
        // 查看缓冲区是否已满
        if ((next_seqnum - base + mod) % mod == WINDOW_WIDTH) {
            return false;
        }
        // 准备发包
        auto &pkt = buff[next_seqnum];
        memcpy(pkt.payload, message.data, sizeof(message.data));
        pkt.seqnum = next_seqnum;
        pkt.acknum = -1;
        pkt.checksum = pUtils->calculateCheckSum(pkt);
        // 发送包并启动定时器
        pns->sendToNetworkLayer(RECEIVER, pkt);
        pns->startTimer(SENDER, Configuration::TIME_OUT, next_seqnum);
        next_seqnum = (next_seqnum + 1) % mod;
        return true;
    }

    void receive(Packet &ackPkt) override {
        int checksum = pUtils->calculateCheckSum(ackPkt);
        if (checksum == ackPkt.checksum) {
            // 定时器先停了
            pns->stopTimer(SENDER, ackPkt.acknum);
            // 先判断是不是窗口内的包，再标记为已经ack
            if (is_valid_packet(ackPkt.acknum)) {
                acked[ackPkt.acknum] = true;
                // 查看是否自己的base包
                if (ackPkt.acknum == base) {
                    while (acked[base]) {
                        acked[base] = false;
                        base = (base + 1) % mod;
                    }
                }
            }
        }
    }

    void timeoutHandler(int seqNum) override {
        pns->sendToNetworkLayer(RECEIVER, buff[seqNum]);
        pns->startTimer(SENDER, Configuration::TIME_OUT, seqNum);
    }

    bool getWaitingState() override {
        // 缓冲区满了就是在等待了吧
        return (next_seqnum - base + mod) % mod == WINDOW_WIDTH;
    }
};

#endif //STOP_WAIT_SRSENDER_HPP
