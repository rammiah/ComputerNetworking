#ifndef LISTENER_H
#define LISTENER_H

#include <QObject>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <thread>
#include <string>
#include <regex>
#include <fstream>
#include <iostream>
#include <map>
#include <QDebug>
#include <memory>

class Listener : public QObject
{
    Q_OBJECT
    // 每次的数据都从内存中读取，防止被缓存
    volatile bool open;
    // 服务器的socket
    int server_sock;
    // 虚拟路径的根目录
    std::string root;
    // 监听的线程指针，根据stl的描述其实不需要使用指针也能保证资源被释放
    std::thread *pt;
    std::map<std::string, std::string> file_types;
    // 监听函数
    void my_listen(int server_sock, std::string root);
    // 得到连接后的处理函数
    void process(int sockfd, sockaddr_in addr);
    // 404可以单独保留
    QString send_file(int sockfd, const std::string &file);
    QString send_not_found(int sockfd);
public:
    explicit Listener(QObject *parent = nullptr);
    // 暴露给外界的启动接口
    void start(int sock, const QString &root);
    // 停止接口
    void stop();

signals:
    // 用于给GUI线程发送消息
    void send_message(QString msg);
};

#endif // LISTENER_H
