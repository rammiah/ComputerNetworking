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
#include <QDebug>

class Listener : public QObject
{
    Q_OBJECT
    volatile bool open;
    int server_sock;
    std::string root;
    std::thread *pt;
    void my_listen(int server_sock, std::string root);
    void process(int sockfd, sockaddr_in addr);
    void send_icon(int sockfd);
    void send_pic(int sockfd, const std::string &file);
    void send_html(int sockfd, const std::string &html);
    void send_not_found(int sockfd);
public:
    explicit Listener(QObject *parent = nullptr);
    void start(int sock, const QString &root);
    void stop();

signals:
    void send_message(QString msg);
};

#endif // LISTENER_H
