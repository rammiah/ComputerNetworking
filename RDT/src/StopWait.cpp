

#include "../include/Base.h"
#include "../include/Global.h"
#include "../include/RdtSender.h"
#include "../include/RdtReceiver.h"
#include "../include/StopWaitRdtSender.h"
#include "../include/StopWaitRdtReceiver.h"
#include "GBNSender.hpp"
#include "GBNReceiver.hpp"
#include "SRSender.hpp"
#include "SRReceiver.hpp"
#include "TCPSender.hpp"

int main(int argc, char *argv[]) {
    int sele = -1;
    if (argc == 2) {
//        strto
        sele = std::stoi(argv[1]);
    }
    RdtSender *ps;
    RdtReceiver *pr;

    switch (sele) {
        case 0:
            ps = new GBNSender();
            pr = new GBNReceiver();
            break;
        case 1:
            ps = new SRSender();
            pr = new SRReceiver();
            break;
        case 2:
            ps = new TCPSender();
            pr = new GBNReceiver();
            break;
        default:
            ps = new StopWaitRdtSender();
            pr = new StopWaitRdtReceiver();
    }

    pns->init();
    pns->setRtdSender(ps);
    pns->setRtdReceiver(pr);
    pns->setInputFile("/home/yaning/Documents/clion/stop_wait/src/input.txt");
    pns->setOutputFile("/home/yaning/Documents/clion/stop_wait/src/output.txt");
    pns->start();

    delete ps;
    delete pr;
    delete pUtils;                                    //指向唯一的工具类实例，只在main函数结束前delete
    delete pns;                                        //指向唯一的模拟网络环境类实例，只在main函数结束前delete

    return 0;
}

