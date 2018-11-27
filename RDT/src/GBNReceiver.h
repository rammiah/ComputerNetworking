//
// Created by yaning on 18-11-27.
//
#include "RdtReceiver.h"
#include "config.h"

class GBNReceiver : public RdtReceiver {
    // 接收方不需要缓冲区
    int expect_seq = 0;
    int mod = 1 << BITS;
public:
    void receive(Packet &packet) override {
        // 设置接收后
        int check_sum = pUtils->calculateCheckSum(packet);
        int seq_num = packet.seqnum;
        pUtils->printPacket("RECEIVER: ", packet);
        std::cout << "recveiver expected: " << expect_seq << ", get: " << packet.seqnum << "\n";
        std::cout << check_sum << " " << packet.checksum << "\n";
        if (check_sum == packet.checksum && packet.seqnum == expect_seq) {
            // 得到的包正是期望的
            Message msg;
            memcpy(msg.data, packet.payload, sizeof(packet.payload));
            // 递交给上层应用层
            pns->delivertoAppLayer(RECEIVER, msg);
            Packet ack_pkt;
            ack_pkt.acknum = expect_seq;
            ack_pkt.seqnum = 0;
            memset(ack_pkt.payload, 0, sizeof(ack_pkt.payload));
            ack_pkt.checksum = pUtils->calculateCheckSum(ack_pkt);
            pns->sendToNetworkLayer(SENDER, ack_pkt);
            expect_seq = (expect_seq + 1) % (mod);
        } else {
            Packet ack_pkt;
            ack_pkt.acknum = (expect_seq - 1 + mod) % mod;
            ack_pkt.seqnum = 0;
            ack_pkt.checksum = pUtils->calculateCheckSum(ack_pkt);
            pns->sendToNetworkLayer(SENDER, ack_pkt);
        }
    }
};
