#ifndef STOP_WAIT__SRRECEIVER_HPP
#define STOP_WAIT__SRRECEIVER_HPP

#include "config.h"
#include "RdtReceiver.h"

class SRReceiver : public RdtReceiver {
private:
    // 声明缓冲区和别的变量
    std::unique_ptr<Packet[]> buff;
    // 需要变量记录包是否接收了
    std::unique_ptr<bool[]> received;
    int mod;
    // 话说接收端也需要两端控制啊
    int base;

    // 是否可以放入缓冲区
    bool if_can_place(int num) {
        // 没有经过结尾处
        if (base + WINDOW_WIDTH < mod) {
            return num >= base && num < base + WINDOW_WIDTH;
        }

        return num >= base || num < (base + WINDOW_WIDTH) % mod;
    }

    bool is_old_packet(int num) {
        int _base = (base - WINDOW_WIDTH + mod) % mod;
        if (_base + WINDOW_WIDTH < mod) {
            return num >= _base && num < _base + WINDOW_WIDTH;
        }
        return num >= _base || num < (_base + WINDOW_WIDTH) % mod;
    }

    void print_window() {
        std::cout << "输出接收方窗口：\n";
        // 接收方的窗口其实是没什么意义的啊
        for (int i = base; i != (base + WINDOW_WIDTH) % mod; i = (i + 1) % mod) {
            pUtils->printPacket("窗口数据：", buff[i]);
        }
        std::cout << "\n";
    }

public:
    SRReceiver() {
        // 设置接收方的缓冲区
        // mod 是序号范围
        mod = 1 << BITS;
        buff = std::make_unique<Packet[]>(mod);
        received = std::make_unique<bool[]>(mod);
        std::fill(received.get(), received.get() + mod, false);
        base = 0;
    }

    void receive(Packet &packet) override {
        int checksum = pUtils->calculateCheckSum(packet);
        if (checksum == packet.checksum) {
            // 此包可缓存
            if (if_can_place(packet.seqnum)) {
                // 查看是否是自己需要的包
                buff[packet.seqnum] = packet;
                received[packet.seqnum] = true;
                // 发送ack
                Packet ack_pkt;
                ack_pkt.acknum = packet.seqnum;
                ack_pkt.seqnum = 0;
                memset(ack_pkt.payload, '*', sizeof(ack_pkt.payload));
                pns->sendToNetworkLayer(SENDER, ack_pkt);
                if (packet.seqnum == base) {
                    // 向上层递交数据
                    Message msg;
                    while (received[base]) {
                        memcpy(msg.data, buff[base].payload, sizeof(buff[base].payload));
                        pns->delivertoAppLayer(RECEIVER, msg);
                        received[base] = false;
                        base = (base + 1) % mod;
                    }
                }
                print_window();
            } else if (is_old_packet(packet.seqnum)) {
                // 发送ack包
                Packet ack_pkt;
                memset(ack_pkt.payload, '*', sizeof(ack_pkt.payload));
                ack_pkt.seqnum = 0;
                ack_pkt.acknum = packet.seqnum;
                ack_pkt.checksum = pUtils->calculateCheckSum(ack_pkt);
                pns->sendToNetworkLayer(SENDER, ack_pkt);
                print_window();
            }
        }
    }
};

#endif