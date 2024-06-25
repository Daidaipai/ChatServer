#ifndef CHATSERVER_H
#define CHATSERVER_H


/*
muduo网络库给用户提供了两个主要的类
TcpServer：用于编写服务器程序
TcpClient：用于编写客户端程序

epoll+线程池
好处：能够把网络I/O的代码和业务代码分开
    只需要关注：用户的连接和断开，用户的可读写事件
*/
#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
using namespace muduo;
using namespace muduo::net;

/*
基于muduo网络库开发服务器程序
1.组合TcpServer对象
2.创建EventLoop事件循环对象的指针
3.明确TcpServer构造对象需要什么参数，输出ChatServer的构造函数
4.在当前服务器类的构造函数中，注册处理连接的回调函数和处理读写事件的回调函数
5.设置合理的服务端线程数量，muduo库会自己分配I/O线程和worker线程
*/


//聊天服务器的主类
class ChatServer{
public:
    //初始化聊天服务器对象
    ChatServer(EventLoop* loop,
            const InetAddress& listenAddr,
            const string& nameArg);
    //启动服务
    void start();
private:
    //专门处理用户的连接创建和断开的回调函数
    void onConnection(const TcpConnectionPtr&);
    //专门处理用户的读写事件的回调函数
    void onMessage(const TcpConnectionPtr&,
                            Buffer*,
                            Timestamp);
    TcpServer _server;//组合的muduo库中实现服务器功能的类对象
    EventLoop * _loop;//指向事件循环对象的指针
};

#endif