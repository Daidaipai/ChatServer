#include "chatservice.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h>
using namespace muduo;
#include <vector>
using namespace std;

//# ChatService* ChatService::service=  nullptr;

// 获取单例对象的接口函数
ChatService* ChatService::getInstance(){
    static ChatService service;
    return &service;

    //# 获取单例对象有两种方法，一种是上面的方法
    //# 下面的做法还需要在类中定义static ChatService* service，然后在源文件中将其初始化
    //# service =new ChatService();
    //# return service;
}


// 用构造函数注册消息以及对应的Handler回调操作
ChatService::ChatService(){
    // 用户基本业务管理相关事件处理回调注册
    _msgHandlerMap.insert({LOGIN_MSG,bind(&ChatService::login,this,_1,_2,_3)});
    _msgHandlerMap.insert({REG_MSG,bind(&ChatService::reg,this,_1,_2,_3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG,bind(&ChatService::oneChat,this,_1,_2,_3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG,bind(&ChatService::addFriend,this,_1,_2,_3)});
    _msgHandlerMap.insert({LOGINOUT_MSG,bind(&ChatService::loginout,this,_1,_2,_3)});
    // 群组业务管理相关事件处理回调注册
    _msgHandlerMap.insert({CREATE_GROUP_MSG,bind(&ChatService::createGroup,this,_1,_2,_3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG,bind(&ChatService::addGroup,this,_1,_2,_3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG,bind(&ChatService::groupChat,this,_1,_2,_3)});

    // 连接redis服务器
    if(_redis.connect()){
        // 设置上报消息的回调
        _redis.init_notify_handler(bind(&ChatService::handleRedisSubscribeMessage,this,_1,_2));
    }
}
// 获取消息对应的处理器
MsgHandler ChatService::getHandler(int msgid){
    //记录错误日志，msgid没有对应的事件处理回调
    auto it = _msgHandlerMap.find(msgid);
    if(it==_msgHandlerMap.end()){
        //返回一个默认的处理器，空操作
        return [=](const TcpConnectionPtr&conn,json&js,Timestamp time){
        //用muduo自带的日志输出
        LOG_ERROR<<"msgid "<<msgid<<" can not find handler!";
        };
    }else{
        return _msgHandlerMap[msgid];
    }
}

// 处理客户端异常退出
void ChatService::clientCloseException(const TcpConnectionPtr&conn)
{
    User user;
    {
        lock_guard<mutex> lock(_connMutex);
        for(auto it=_userConnMap.begin();it!=_userConnMap.end();it++){
            if(it->second==conn){
                // 从Map表中删除用户的连接信息
                _userConnMap.erase(it);
                user.setId(it->first);
                break;
            }
        }
    }

    // 用户注销下线，在redis中取消订阅unsubscribe
    _redis.unsubscribe(user.getId());

    // 更新用户状态信息
    if(user.getId()!=-1){
        user.setState("offline");
        _userModel.updateState(user);
    } 
}

// 服务器异常，业务重置方法
void ChatService::reset()
{
    _userModel.resetState();
}

// 处理登录业务 传 id password
void ChatService::login(const TcpConnectionPtr&conn,json&js,Timestamp time){
    int id=js["id"];
    string pwd=js["password"];

    User user=_userModel.query(id);
    if(user.getId()==id && user.getPassword()==pwd){
        if(user.getState()=="online"){
            // 该用户已经登录，不允许重复登录
            json response;
            response["msgid"]=LOGIN_MSG_ACK;
            response["errno"]=2;
            response["errmsg"]="该账号已经登录";
            conn->send(response.dump());
        }else{
            // 登录成功，记录用户连接信息
            {
                lock_guard<mutex> lock(_connMutex);
                _userConnMap.insert({id,conn});
            }

            // id用户登录成功后，像redis订阅(subscribe)channel id
            _redis.subscribe(id);

            // 登录成功，更新用户状态信息 state offline->online
            user.setState("online");
            _userModel.updateState(user);
            
            json response;
            response["msgid"]=LOGIN_MSG_ACK;
            response["errno"]=0;
            response["id"]=user.getId();
            response["name"]=user.getName();
            // 查询该用户是否有离线消息
            vector<string>vec=_offlineMsgModel.query(id);
            if(!vec.empty()){
                response["offlinemsg"]=vec;
                // 读取该用户的离线消息后，把该用户的所有离线消息删除掉
                _offlineMsgModel.remove(id);
            }
            // 查询该用户的好友信息并返回
            vector<User> userVec=_friendModel.query(id);
            if(!userVec.empty()){
                vector<string> userVec2;
                for(User &user:userVec){
                    json js;
                    js["id"]=user.getId();
                    js["name"]=user.getName();
                    js["state"]=user.getState();
                    userVec2.push_back(js.dump());
                }
                response["friends"]=userVec2;
            }
            // 查询用户的群组信息
            vector<Group> groupuserVec = _groupModel.queryGroups(id);
            if (!groupuserVec.empty())
            {
                vector<string> groupV;
                for (Group &group : groupuserVec)
                {
                    json grpjson;
                    grpjson["id"] = group.getId();
                    grpjson["groupname"] = group.getName();
                    grpjson["groupdesc"] = group.getDesc();
                    vector<string> userV;
                    for (GroupUser &user : group.getUsers())
                    {
                        json js;
                        js["id"] = user.getId();
                        js["name"] = user.getName();
                        js["state"] = user.getState();
                        js["role"] = user.getRole();
                        userV.push_back(js.dump());
                    }
                    grpjson["users"] = userV;
                    groupV.push_back(grpjson.dump());
                }
                response["groups"] = groupV;
            }
            conn->send(response.dump());
        } 
    }else{
        // 登录失败(用户不存在或密码错误)
        json response;
        response["msgid"]=LOGIN_MSG_ACK;
        response["errno"]=1;
        response["errmsg"]="用户名或密码错误";
        conn->send(response.dump());
    }
}
//处理注册业务 传 name password
void ChatService::reg(const TcpConnectionPtr&conn,json&js,Timestamp time){
    string name=js["name"];
    string pwd=js["password"];

    User user;
    user.setName(name);
    user.setPassword(pwd);

    bool state=_userModel.insert(user);
    if(state){
        //注册成功
        json response;
        response["msgid"]=REG_MSG_ACK;
        response["errno"]=0;
        response["id"]=user.getId();
        conn->send(response.dump());
    }else{
        //注册失败
        json response;
        response["msgid"]=REG_MSG_ACK;
        response["errno"]=1;
        conn->send(response.dump());
    }
}

// 处理注销业务
void ChatService::loginout(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid=js["id"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it=_userConnMap.find(userid);
        if(it!=_userConnMap.end()){
            _userConnMap.erase(it);
        }
    }
    
    // 用户注销下线，在redis中取消订阅unsubscribe
    _redis.unsubscribe(userid);

    // 更新用户状态信息
    User user(userid,"","","offline");
    _userModel.updateState(user); 
}

// 一对一聊天业务
void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int toid=js["toid"].get<int>();
    {
        // toid在线且在当前服务器上
        lock_guard<mutex> lock(_connMutex);
        auto it=_userConnMap.find(toid);
        if(it!=_userConnMap.end()){
            // toid在线，转发消息，服务器主动推送给toid用户
            it->second->send(js.dump());
            return;
        }
    }

    // 查询用户在线状态，若toid在线，但不在当前服务器上，发布(publish)信息
    User user=_userModel.query(toid);
    if(user.getState()=="online"){
        _redis.publish(toid,js.dump());
        return;
    }

    // toid不在线，存储离线消息
    _offlineMsgModel.insert(toid,js.dump());
}

// 添加好友业务
void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid=js["id"].get<int>();
    int friendid=js["friendid"].get<int>();

    // 存储好友信息
    _friendModel.insert(userid,friendid);
}

// 创建群组业务
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid=js["id"].get<int>();
    string name=js["groupname"];
    string desc=js["groupdesc"];

    Group group;
    group.setName(name);
    group.setDesc(desc);
    if(_groupModel.createGroup(group)){
        // 存储群组创建人信息
        _groupModel.addGroup(userid,group.getId(),"creator");
    }
}

// 加入群组业务
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid=js["id"].get<int>();
    int groupid=js["groupid"].get<int>();
    _groupModel.addGroup(userid,groupid,"normal");
}

// 群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid=js["id"].get<int>();
    int groupid=js["groupid"].get<int>();
    vector<int> useridVec=_groupModel.queryGroupUsers(userid,groupid);

    lock_guard<mutex> lock(_connMutex);
    for(auto &id:useridVec){
        // 看用户是否在线
        auto it=_userConnMap.find(id);
        if(it!=_userConnMap.end()){
            // 在线且在当前服务器直接转发消息
            it->second->send(js.dump());
        }else{
            // 判断是否在线，若在线则在其他服务器上，此时publish消息
            User user=_userModel.query(id);
            if(user.getState()=="online"){
                _redis.publish(id,js.dump());
            }else{
                // 存储离线信息
                _offlineMsgModel.insert(id,js.dump());
            } 
        }
    }
}

// 从redis消息队列中获取订阅的信息
void ChatService::handleRedisSubscribeMessage(int userid, string msg)
{
    lock_guard<mutex> lock(_connMutex);
    auto it=_userConnMap.find(userid);
    if(it!=_userConnMap.end()){
        it->second->send(msg);
        return;
    }

    // 若userid用户在这个过程中下线
    _offlineMsgModel.insert(userid,msg);
}