# ChatServer
基于muduo库实现的集群聊天服务器和客户端源码，利用MySQL数据库存储用户和群组数据，为提高并发量利用了Nginx负载均衡器，利用Redis中间件实现不同服务器间消息的订阅和发布。
