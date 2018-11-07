#ifndef SERVER_H
#define SERVER_H

#include <QWidget>
#include <QFileDialog>
#include <QDebug>
#include "listener.h"

namespace Ui {
class Server;
}

class Server : public QWidget
{
    Q_OBJECT

public:
    explicit Server(QWidget *parent = 0);
    ~Server();
//    static Server *instance;

private slots:
    void on_ip_edit_textChanged(const QString &arg1);

    void on_port_edit_textChanged(const QString &arg1);

    void on_root_btn_clicked();

    void on_power_btn_clicked();
private:
    Ui::Server *ui;
    volatile bool server_open;
    int server_sock;
    sockaddr_in server_addr;
    Listener *lis_ptr;
};

#endif // SERVER_H
