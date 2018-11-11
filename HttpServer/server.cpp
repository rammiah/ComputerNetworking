#include "server.h"
#include "ui_server.h"

Server::Server(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Server)
{
    ui->setupUi(this);
    server_open = false; // server is not open
    ui->ip_edit->setText("0.0.0.0");
    ui->port_edit->setText("2333");
    ui->root_edit->setText("/home/yaning");
    lis_ptr = new Listener();
    connect(lis_ptr, &Listener::send_message, ui->messages, &QTextBrowser::append);
    setFixedSize(width(), height());
}

Server::~Server()
{
    delete ui;
    if (server_open) {
        shutdown(server_sock, SHUT_RDWR);
        lis_ptr->stop();
        delete lis_ptr;
    }
}

// check if the ip address and port number is valid
static bool is_ip_port_ok(const QString &ip, const QString &port) {
    QStringList digits = ip.split('.', QString::SkipEmptyParts);
    if (digits.length() != 4) {
        return false;
    }
    for (int i = 0; i < digits.length(); ++i) {
        bool ok;
        int num = digits[i].toInt(&ok, 10);
        if (!ok || num < 0 || num > 255) {
            return false;
        }
    }
    bool ok;
    int p = port.toInt(&ok, 10);
    if (!ok || p < 0 || p > 65535) {
        return false;
    }
    return true;
}

void Server::on_ip_edit_textChanged(const QString &)
{
    ui->power_btn->setEnabled(false);
    if (is_ip_port_ok(ui->ip_edit->text(), ui->port_edit->text())) {
        ui->power_btn->setEnabled(true);
    }
}

void Server::on_port_edit_textChanged(const QString &)
{
    // change if the port is ok or not
    ui->power_btn->setEnabled(false);
    if (is_ip_port_ok(ui->ip_edit->text(), ui->port_edit->text())) {
        ui->power_btn->setEnabled(true);
    }
}

void Server::on_root_btn_clicked()
{
    /// select the path of your index html
    QString dir = QFileDialog::getExistingDirectory(this, "Open root directory", "/home/yaning");
    // if dir is empty not replace the root dir
    if (!dir.isEmpty()) {
        ui->root_edit->setText(dir);
    }
}

void Server::on_power_btn_clicked()
{
    if (server_open) {
        // close the server
        ui->power_btn->setText("Start");
        ui->ip_edit->setFocusPolicy(Qt::StrongFocus);
        ui->port_edit->setFocusPolicy(Qt::StrongFocus);
        ui->root_btn->setEnabled(true);
        server_open = false;
        // ::close(server_sock);
        shutdown(server_sock, SHUT_RDWR);
        server_sock = -1;
        ui->messages->append("server closed.");
        lis_ptr->stop();
    } else {
        // 此处逻辑有可能过于复杂，不触及GUI的部分放到子线程中执行比较好
        // open the server
        server_sock = socket(AF_INET, SOCK_STREAM, 0);
        server_addr.sin_addr.s_addr = inet_addr(ui->ip_edit->text().toStdString().c_str());
        server_addr.sin_port = htons(ui->port_edit->text().toInt());
        server_addr.sin_family = AF_INET;
        bzero(server_addr.sin_zero, sizeof(server_addr.sin_zero));
        // set a option
        int option = 1;
        if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0) {
            ui->messages->append("setsockoption failed.");
            shutdown(server_sock, SHUT_RDWR);
            return;
        }
        if (bind(server_sock, (sockaddr*)&server_addr, sizeof(sockaddr)) < 0) {
            ui->messages->append("bind failed.");
            shutdown(server_sock, SHUT_RDWR);
            return;
        }
        if (listen(server_sock, 10) < 0) {
            ui->messages->append("listen failed.");
            shutdown(server_sock, SHUT_RDWR);
            return;
        }
        // server is open successfully
        server_open = true;
        ui->power_btn->setText("Stop");
        ui->ip_edit->setFocusPolicy(Qt::NoFocus);
        ui->port_edit->setFocusPolicy(Qt::NoFocus);
        ui->root_btn->setEnabled(false);
        ui->messages->clear();
        ui->messages->append(QString("listening on: ") + ui->ip_edit->text() + ":"
                             + ui->port_edit->text() + " " + ui->root_edit->text() + "\n");
        lis_ptr->start(server_sock, ui->root_edit->text());
         qDebug() << "Open server socket.\n";
    }
}
