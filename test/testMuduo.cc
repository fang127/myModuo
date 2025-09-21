#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
#include <iostream>
#include <functional>
#include <string>
using namespace muduo;
using namespace muduo::net;
using namespace std::placeholders;
/*
    基于muduo网络库开发服务器程序
    1. 组合TcpServer对象
    2. 创建EventLoop事件循环对象的指针
    3. 明确TcpServer构造函数需要什么参数，输出ChatServer的构造函数
    4. 在当前服务器类的构造函数中，注册处理连接的回调函数和处理读写事件的回调函数
    5. 设置合适的服务端线程数，muduo库会自己划分I/O线程和worker线程
*/
class ChatServer
{
public:
    ChatServer(EventLoop *loop,               // 事件循环
               const InetAddress &listenAddr, // IP地址+端口号
               const string &nameArg)         // 服务器名字
        : server_(loop, listenAddr, nameArg), loop_(loop)
    {
        // 给服务器注册用户连接的创建和断开回调
        server_.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));

        // 给服务器注册用户读写事件回调
        server_.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));

        // 设置服务器端的线程数量 1个I/O线程 3个worker线程
        server_.setThreadNum(4);
    }

    // 开启事件循环
    void start()
    {
        server_.start();
    }

private:
    // 专门处理用户的连接创建和断开
    void onConnection(const TcpConnectionPtr &conn)
    {
        if (conn->connected())
            std::cout << conn->peerAddress().toIpPort() << " -> "
                      << conn->localAddress().toIpPort()
                      << " state:online" << std::endl;
        else
        {
            std::cout << conn->peerAddress().toIpPort() << " -> "
                      << conn->localAddress().toIpPort()
                      << " state:offline" << std::endl;
            conn->shutdown(); // 关闭句柄文件
        }
    }

    // 专门处理用户的读写事件
    void onMessage(const TcpConnectionPtr &conn, // 连接
                   Buffer *buffer,               // 缓冲区
                   Timestamp time)               // 接收到数据的时间信息
    {
        std::string buf = buffer->retrieveAllAsString();
        std::cout << "recv data:" << buf
                  << " time:" << time.toString() << std::endl;
        if (!buf.empty() && buf.back() == '\n')
            conn->send(buf);
    }

    TcpServer server_; // server对象
    EventLoop *loop_;  // epoll
};

int main()
{
    EventLoop loop; // 事件循环器 epoll
    InetAddress addr("127.0.0.1", 6000);
    ChatServer server(&loop, addr, "MyChatServer");

    server.start(); // 监听开始
    loop.loop();    // 事件监听，epoll_wait，以阻塞方式等待新用户连接，已连接用户的读写事件等

    return 0;
}