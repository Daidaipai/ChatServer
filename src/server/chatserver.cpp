/*
muduo网络库给用户提供了两个主要的类
TcpServer：用于编写服务器程序
TcpClient：用于编写客户端程序

epoll+线程池
好处：能够把网络I/O的代码和业务代码分开
    只需要关注：用户的连接和断开，用户的可读写事件
*/

#include "chatserver.hpp"
#include "json.hpp"
#include "chatservice.hpp"
#include <functional>
#include <string>

using namespace std;
using namespace placeholders;
using json=nlohmann::json;

ChatServer::ChatServer(EventLoop* loop,
            const InetAddress& listenAddr,
            const string& nameArg)
            :_server(loop,listenAddr,nameArg),_loop(loop){
            //注册连接回调
            _server.setConnectionCallback(bind(&ChatServer::onConnection,this,_1));
            //注册读写回调
            _server.setMessageCallback(bind(&ChatServer::onMessage,this,_1,_2,_3));
            //设置线程数量
            _server.setThreadNum(4);
}

//启动服务
void ChatServer::start(){
    _server.start();
}

//专门处理用户的连接创建和断开的回调函数
void ChatServer::onConnection(const TcpConnectionPtr&conn){
    if(!conn->connected()){
        ChatService::getInstance()->clientCloseException(conn);
        conn->shutdown();
    }
}
//专门处理用户的读写事件的回调函数
void ChatServer::onMessage(const TcpConnectionPtr&conn,
                            Buffer*buffer,
                            Timestamp time){
    string  buf=buffer->retrieveAllAsString();
    //数据的反序列化
    json js=json::parse(buf);
    //达到目的：完全解耦网络模块的代码和业务模块的代码(利用回调的思想)
    //通过js["msgid"]获取=》业务handler=》conn js time
    //通过 get<int>()，将js["msgid"]对应的值强转成int类型
    auto msghandler=ChatService::getInstance()->getHandler(js["msgid"].get<int>());
    //回调消息绑定好的事件处理器，来执行相应的业务处理
    msghandler(conn,js,time);
}