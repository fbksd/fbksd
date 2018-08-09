#ifndef RPC_H
#define RPC_H

#include <map>
#include <functional>
#include <QBuffer>
#include <QDataStream>

class QTcpServer;
class QTcpSocket;

/**
 * \brief The SerialSize class computes the size of an object when serialized.
 *
 * \ingroup Core
 */
class SerialSize
{
public:
    SerialSize() :
        stream(&data)
    {
        data.open(QIODevice::WriteOnly);
    }

    template <typename T>
    quint64 operator()(const T& t)
    {
        data.seek(0);
        stream << t;
        return data.pos();
    }

private:
    QBuffer data;
    QDataStream stream;
};


/**
 * \brief The RPCServer class.
 *
 * \ingroup Core
 */
class RPCServer: public QObject
{
    Q_OBJECT
public:
    typedef std::function<void(QDataStream&, QDataStream&)> CallFunctionType;

    RPCServer();
    virtual ~RPCServer();

    void startServer(int port);

protected:
    void registerCall(const std::string& name, const CallFunctionType& call)
    { callsMap[name] = call; }

private slots:
    void processNewConnection();
    void readData();

private:
    enum State
    {
        WAITING_CALL,
        WAITING_SIZE,
        WAITING_DATA,
    };

    QTcpServer *server;
    QTcpSocket *current_connection;
    QByteArray current_data;
    QBuffer* current_buffer;
    State state;
    QString callName;
    quint64 msgDataSize;
    std::map<std::string, CallFunctionType> callsMap;
};


/**
 * \brief The RPCClient class
 *
 * \ingroup Core
 */
class RPCClient
{
public:
    typedef std::function<void(QDataStream&)> CallbackFunctionType;

    RPCClient(int port, bool waitConnection = false);

    virtual ~RPCClient();

protected:
    virtual void call(const std::string& callName, quint64 msgSize, CallbackFunctionType writeCallback, CallbackFunctionType readCallback);

private:
    QTcpSocket *current_connection;
};


#endif // RPC_H
