//! \cond 1

#include "fbksd/core/RPC.h"

#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QTextStream>
#include <QBuffer>
#include <QThread>



////////////////////////////////////////////////
//                  RPCServer
////////////////////////////////////////////////

RPCServer::RPCServer():
    QObject(0)
{
    server = new QTcpServer(this);
    QObject::connect(server, &QTcpServer::newConnection, this, &RPCServer::processNewConnection);

    state = WAITING_CALL;
    current_buffer = new QBuffer(&current_data);
    current_buffer->open(QIODevice::ReadOnly);
}

RPCServer::~RPCServer()
{
    delete server;
    delete current_buffer;
}

void RPCServer::startServer(int port)
{
    server->listen(QHostAddress::LocalHost, port);
}

void RPCServer::processNewConnection()
{
    current_connection = server->nextPendingConnection();
    QObject::connect(current_connection, &QTcpSocket::readyRead, this, &RPCServer::readData);
}

void RPCServer::readData()
{
    current_data.append(current_connection->readAll());

    if(state == WAITING_CALL)
    {
//        qDebug("BenchmarkServer: receiving call...");
        if(!current_buffer->canReadLine())
            return;

        QTextStream textStream(&current_data, QIODevice::ReadOnly);
        callName = textStream.readLine();
        current_buffer->seek(textStream.pos());
//        qDebug("BenchmarkServer: call received = %s", callName.toStdString().data());

        state = WAITING_SIZE;
    }

    if(state == WAITING_SIZE)
    {
        if(current_buffer->bytesAvailable() < sizeof(quint64))
            return;

        QDataStream inDataStream(current_buffer);
        inDataStream >> msgDataSize;

        state = WAITING_DATA;
    }

    if(state == WAITING_DATA)
    {
        if(current_buffer->bytesAvailable() < msgDataSize)
            return;

        auto e = callsMap.find(callName.toStdString());
        if(e != callsMap.end())
        {
            QDataStream inStream(current_buffer);
            QDataStream outStream(current_connection);
            e->second(inStream, outStream);
            current_connection->waitForBytesWritten();
        }

        current_data.clear();
        current_buffer->reset();
        state = WAITING_CALL;
    }
}



////////////////////////////////////////////////
//                  RPCClient
////////////////////////////////////////////////

RPCClient::RPCClient(int port, bool waitConnection)
{
    current_connection = new QTcpSocket();

    if(waitConnection)
    {
        while(true)
        {
            current_connection->connectToHost(QHostAddress::LocalHost, port);
            if(current_connection->waitForConnected(-1))
                break;
            else
                QThread::sleep(1);
        }
    }
    else
    {
        current_connection->connectToHost(QHostAddress::LocalHost, port);
        if(!current_connection->waitForConnected())
            qDebug("could not connect with benchmark server: %s", current_connection->errorString().toStdString().c_str());
    }
}

RPCClient::~RPCClient()
{
    delete current_connection;
}

void RPCClient::call(const std::string &name, quint64 msgSize, CallbackFunctionType writeCallback, CallbackFunctionType readCallback)
{
    QTextStream textStream(current_connection);
    QDataStream dataStream(current_connection);

    QString callName = QString::fromStdString(name);
    callName += '\n';
    textStream << callName;
    textStream.flush();
    dataStream << msgSize;
    if(msgSize != 0)
        writeCallback(dataStream);
    while(current_connection->bytesToWrite())
        current_connection->waitForBytesWritten();

    // Call sent. Waiting answer...
    while(current_connection->bytesAvailable() < sizeof(quint64))
    {
        if(!current_connection->waitForReadyRead(-1))
        {
            qDebug() << "RPC: connection error while waiting for answer from server (" << current_connection->errorString() << ")";
            return;
        }
    }
    QDataStream inDataStream(current_connection);
    quint64 size = 0;
    inDataStream >> size;
    while(current_connection->bytesAvailable() < size)
    {
        if(!current_connection->waitForReadyRead(-1))
        {
            qDebug() << "RPC: connection error while waiting for answer from server (" << current_connection->errorString() << ")";
            return;
        }
    }

    readCallback(inDataStream);
}


//! \endcond
