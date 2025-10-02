#include "Logger.h"
#include "TcpServer.h"
#include <functional>
#include <string>
using namespace myMuduo;

class EchoServe
{

public:
    EchoServe(EventLoop *loop, const InetAddress &addr, const std::string &name)
        : server_(loop, addr, name), loop_(loop)
    {
        // 注册回调
        server_.setConnectionCallBack(
            std::bind(&EchoServe::onConnection, this, std::placeholders::_1));
        server_.setMessageCallBack(
            std::bind(&EchoServe::onMessage, this, std::placeholders::_1,
                      std::placeholders::_2, std::placeholders::_3));

        // 设置线程数
        server_.setThreadNum(3);
    }

    void start() { server_.start(); }

private:
    // 连接建立和断开回调
    void onConnection(const TcpConnectionPtr &conn)
    {
        if (conn->connected())
        {
            LOG_INFO("Connection UP : %s",
                     conn->peerAddress().toIpPort().c_str());
        }
        else
        {
            LOG_INFO("Connection DOWN : %s",
                     conn->peerAddress().toIpPort().c_str());
        }
    }
    // 可读写事件回调
    void onMessage(const TcpConnectionPtr &conn, Buffer *buf, Timestamp nowtime)
    {
        std::string buffer = buf->retrieveAllAsString();
        std::cout << "recv data:" << buffer << "time:" << nowtime.toString()
                  << std::endl;
        if (!buffer.empty() && buffer.back() == '\n')
            conn->send(buffer);
    }

    EventLoop *loop_;
    TcpServer server_;
};

int main()
{
    EventLoop loop;
    InetAddress addr(8000);
    // 创建服务器，自动创建Acceptor
    EchoServe server(&loop, addr, "EchoServer-01");
    // listen loopthread listenfd => acceptChannel => mainloop
    server.start();
    // 启动mainloop的底层poller
    loop.loop();

    return 0;
}