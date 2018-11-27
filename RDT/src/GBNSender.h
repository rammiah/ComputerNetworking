//
// Created by yaning on 18-11-27.
//
#include "config.h"
#include "RdtSender.h"

class GBNSender : public RdtSender {
    // 发送方需要设置缓冲区
    Packet *buff;
    int base, next_seqnum, mod;
//    std::mutex lock;
public:
    GBNSender() {
        mod = 1 << BITS;
        buff = new Packet[mod];
        memset(buff, 0, mod * sizeof(Packet));
        base = 0;
        next_seqnum = 0;
    }

    virtual ~GBNSender() {
        // 父类中好像不是虚函数
        delete[](buff);
    }

    bool send(Message &message) override {
        // 发送时检查缓冲区是否可以再放数据
        if ((next_seqnum - base + mod) % mod == WINDOW_WIDTH) {
            // 放不下了
            return false;
        }
        // 发送出去
        Packet pkt;
        memcpy(pkt.payload, message.data, sizeof(message.data));
        // 无用
        pkt.acknum = -1;
        // 序号有用
        assert(next_seqnum >= 0 && next_seqnum < 8);
        pkt.seqnum = next_seqnum;
        pkt.checksum = pUtils->calculateCheckSum(pkt);
        buff[next_seqnum] = pkt;
        std::cout << "after copy: " << pkt.checksum << " " << pUtils->calculateCheckSum(buff[next_seqnum]) << " ";
        std::cout << buff[next_seqnum].checksum << "\n";
        // memcpy(buff + next_seqnum, &pkt, sizeof(pkt));
        if (base == next_seqnum) {
            pns->startTimer(SENDER, Configuration::TIME_OUT, 0);
        }
        next_seqnum = (next_seqnum + 1) % mod;
        std::cout << "after send seqnum: " << next_seqnum << "\n";
        pns->sendToNetworkLayer(RECEIVER, pkt);
        pUtils->printPacket("SENDER send: ", pkt);
        // 指向下一个空的缓冲区
        return true;
    }

    void receive(Packet &ackPkt) override {
        pUtils->printPacket("SENDER receive: ", ackPkt);
        int check_sum = pUtils->calculateCheckSum(ackPkt);
        std::cout << check_sum << " " << ackPkt.checksum << "\n";
        if (check_sum == ackPkt.checksum) {
            base = (ackPkt.acknum + 1) % mod;
            if (base == next_seqnum) {
                pns->stopTimer(SENDER, 0);
            } else {
                pns->stopTimer(SENDER, 0);
                pns->startTimer(SENDER, Configuration::TIME_OUT, 0);
            }
            std::cout << "after receive base: " << base << ", next_seqnum: " << next_seqnum << "\n";
        }
    }

    void timeoutHandler(int seqNum) override {
        // 发送所有未确认报文
        // 发送过程中不要改变base 和next_seqnum
//        std::lock_guard<std::mutex> guard(lock);
        if (next_seqnum != base) {
            pns->startTimer(SENDER, Configuration::TIME_OUT, 0);
            for (int i = base; i != next_seqnum; i = (i + 1) % mod) {
                // buff[i].checksum = pUtils->calculateCheckSum(buff[i]);
                // assert(buff[i].seqnum >= 0 && buff[i].seqnum < 8);
                pns->sendToNetworkLayer(RECEIVER, buff[i]);
                pUtils->printPacket("SENDER timeout: ", buff[i]);
            }
            std::cout << "after timeout next_seqnum: " << next_seqnum << "\n";
        }
    }


    bool getWaitingState() override {
        return (next_seqnum - base + mod) % mod == WINDOW_WIDTH;
    }
};
