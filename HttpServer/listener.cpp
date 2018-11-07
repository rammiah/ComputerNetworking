#include "listener.h"

static bool endswith(const std::string &main_str, const std::string &end_str) {
    if (end_str.size() > main_str.size()) {
        return false;
    }
    int idx_main = main_str.size() - 1, idx_end = end_str.size() - 1;
    while (idx_end >= 0) {
        if (main_str[idx_main] != end_str[idx_end]) {
            return false;
        }
        --idx_end;
        --idx_main;
    }
    return true;
}

Listener::Listener(QObject *parent) : QObject(parent)
{
    // qDebug() << open << "\n";
    pt = nullptr;
    open = false;
    server_sock = -1;
    root = "/home/yaning";
}

void Listener::start(int sock, const QString &root)
{
    // qDebug() << "start server.\n";
    // qDebug() << open << "\n";
    // qDebug() << !open << "\n";
    if (!open) {
        // qDebug() << open << "\n";
        pt = new std::thread(&Listener::my_listen, this, sock, root.toStdString());
        pt->detach();
        open = true;
        this->root = root.toStdString();
        // qDebug() << "Server opened\n";
    }
    // qDebug() << pt << "\n";
}

void Listener::stop()
{
    if (open) {
        delete pt;
        pt = nullptr;
        open = false;
    }
}

void Listener::send_not_found(int sockfd) {
    // 不知道发送啥，404吧
    emit send_message("HTTP/1.1 404 Not Found\n");
    std::string msg;
    msg.append("HTTP/1.1 404 Not Found\r\n"
               "Content-Type: text/html;charset=UTF-8\r\n"
               "Content-Length: 145\r\n"
               "Connection: Close\r\n"
               "Content-Encoding: utf-8\r\n\r\n"
               "<!DOCTYPE html>"
               "<html lang=\"en\">"
               "<head>"
                   "<meta charset=\"UTF-8\">"
                   "<title>Error!</title>"
               "</head>"
               "<body>"
               "<p>404 Not Found.</p>"
               "</body>"
               "</html>");
    send(sockfd, msg.c_str(), msg.size(), 0);
}

// 发送请求的html文件
void Listener::send_html(int sockfd, const std::string &html) {
    // 转换为相对路径执行
    std::string filename = root + html;
    std::ifstream in(filename);
    if (!in.is_open()) {
        // 没有这个文件，返回404即可
        send_not_found(sockfd);
    } else {
        // 文件打开成功
        in.seekg(0, std::ios_base::end);
        int len = in.tellg();
        in.seekg(0, std::ios_base::beg);
        char *buf = new char[len];
        in.read(buf, len);
        in.close();
        std::string data;
        data.append(buf, len);
        delete[]buf;
        std::string msg;
        msg.append("HTTP/1.1 200 OK\r\n"
                   "Content-Type: text/html;charset=UTF-8\r\n"
                   "Content-Length: " + std::to_string(len) + "\r\n" +
                   "Connection: Close\r\n"
                   "Content-Encoding: utf-8\r\n\r\n");
        msg.append(data.c_str(), data.size());
        send(sockfd, msg.c_str(), msg.size(), 0);
        emit send_message("HTTP/1.1 200 OK\n");
    }
}

// 发送请求的jpeg图片
void Listener::send_pic(int sockfd, const std::string &file) {
    std::string filename = root + file;
    std::string type = file.substr(file.rfind('.') + 1);
    // 二进制读入
    std::ifstream in(filename, std::ios_base::binary | std::ios_base::in);
    if (!in.is_open()) {
        send_not_found(sockfd);
    } else {
        in.seekg(0, std::ios_base::end);
        int len = in.tellg();
        in.seekg(0, std::ios_base::beg);
        char *buf = new char[len];
        in.read(buf, len);
        in.close();
        std::string msg = "HTTP/1.1 200 OK\r\n"
                     "Connection: Close\r\n"
                     "Content-Length: " + std::to_string(len) + "\r\n" +
                     "Content-Type: image/" + type + "\r\n\r\n";// 图片类型
        send(sockfd, msg.c_str(), msg.size(), 0);
        send(sockfd, buf, len, 0);
        delete[]buf;
        emit send_message("HTTP/1.1 200 OK\n");
    }
}

void Listener::send_icon(int sockfd) {
    std::string type = "x-icon";
    // 二进制读入
    std::ifstream in(root + "/favicon.ico", std::ios_base::binary | std::ios_base::in);
    if (!in.is_open()) {
        send_not_found(sockfd);
    } else {
        in.seekg(0, std::ios_base::end);
        int len = in.tellg();
        in.seekg(0, std::ios_base::beg);
        char *buf = new char[len];
        in.read(buf, len);
        in.close();
        std::string msg = "HTTP/1.1 200 OK\r\n"
                     "Connection: Close\r\n"
                     "Content-Length: " + std::to_string(len) + "\r\n" +
                     "Content-Type: image/" + type + "\r\n\r\n";// 图片类型
        send(sockfd, msg.c_str(), msg.size(), 0);
        send(sockfd, buf, len, 0);
        delete[]buf;
        emit send_message("HTTP/1.1 200 OK\n");
    }
}

void Listener::process(int sockfd, sockaddr_in addr) {

    // 输出一下地址信息
    QString addr_msg = QString("client address: ") +
            inet_ntoa(addr.sin_addr) + ":" + QString::number(ntohs(addr.sin_port));
    // qDebug() << addr_msg << "\n";
    emit send_message(addr_msg);
    // 逻辑处理在这里
    // 首先读取消息
    std::string recv_data;
    long int len;
    char buf[BUFSIZ];
    while ((len = recv(sockfd, buf, BUFSIZ, 0)) > 0) {
        recv_data.append(buf, len);
        if (recv_data.find("\r\n\r\n") != std::string::npos) {
            break;
        }
    }
    // 找到头部
    size_t head_end = recv_data.find("\r\n\r\n");
    if (head_end == std::string::npos) {
        shutdown(sockfd, SHUT_RDWR);
        return;
    }

    std::string headers = recv_data.substr(0, head_end);
    // 分析头部获取要求的文件
    std::regex reg_req(R"(([A-Z]+) (.*?) HTTP/\d\.\d)");
    std::smatch match_result;
    std::string op, uri;
    if (std::regex_search(headers, match_result, reg_req)) {
        op = match_result[1].str();
        uri = match_result[2].str();
        emit send_message(QString::fromStdString(match_result[0].str()));
    }

    if (endswith(uri, ".html")) {
        // 有可能发送失败，不过那是发送函数需要处理的东西
        send_html(sockfd, uri);
    } else if (endswith(uri, ".jpeg") || endswith(uri, ".png")) {
        // 暂时发送这两种比较常见的格式的图片试一下吧
        send_pic(sockfd, uri);
    } else if (endswith(uri, "favicon.ico")) {
        // 返回网站的图标啊
        send_icon(sockfd);
    } else {
        // 返回404
        send_not_found(sockfd);
    }

//    close(sockfd);
    shutdown(sockfd, SHUT_RDWR);
}

void Listener::my_listen(int server_sock, std::string root) {
    while (open) {
        this->root = root;
        sockaddr_in client_addr;
        socklen_t len;
        int client_sock = accept(server_sock, (sockaddr*)&client_addr, &len);
        if (client_sock < 0) {
            return;
        }
        std::thread t(&Listener::process, this, client_sock, client_addr);
        t.detach();
    }
}
