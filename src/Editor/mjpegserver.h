#ifndef MJPEGSERVER_H
#define MJPEGSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QBuffer>
#include <QImage>

class MjpegServer : public QTcpServer {
  Q_OBJECT
 public:
  MjpegServer();
  explicit MjpegServer(QObject *parent = nullptr) : QTcpServer(parent), clientSocket(nullptr) {}

  void startServer(int port = 35587) {
    if (listen(QHostAddress::Any, port)) {
      qDebug() << "Server gestart op poort" << port;
    }
  }

  void sendFrame(const QImage &frame) {
    if (!clientSocket || clientSocket->state() != QAbstractSocket::ConnectedState) return;

    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    frame.save(&buffer, "JPG", 80);

    clientSocket->write("--frame\r\n");
    clientSocket->write("Content-Type: image/jpeg\r\n");
    clientSocket->write(QString("Content-Length: %1\r\n\r\n").arg(ba.size()).toUtf8());
    clientSocket->write(ba);
    clientSocket->write("\r\n");
  }

 protected:
  void incomingConnection(qintptr socketDescriptor) override {
    clientSocket = new QTcpSocket(this);
    clientSocket->setSocketDescriptor(socketDescriptor);

    connect(clientSocket, &QTcpSocket::readyRead, [this]() {
      clientSocket->readAll();
      clientSocket->write("HTTP/1.1 200 OK\r\n");
      clientSocket->write("Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n");
    });
  }

 private:
  QTcpSocket *clientSocket;
};

#endif  // MJPEGSERVER_H
