# 设置需要编译的源文件列表
#set(SRC_LIST ./chatserver.cpp)
# 定义了一个SRC_LIST 变量，包含了该目录下所有的源文件
aux_source_directory(. SRC_LIST)
aux_source_directory(./db DB_LIST)
aux_source_directory(./model MODEL_LIST)
aux_source_directory(./redis REDIS_LIST)
# 生成可执行文件ChatServer，由SRC_LIST变量所定义的源文件编译而来
add_executable(ChatServer ${SRC_LIST} ${DB_LIST} ${MODEL_LIST} ${REDIS_LIST})

# 表示ChatServer这个目标程序，需要连接的库文件
target_link_libraries(ChatServer muduo_net muduo_base mysqlclient hiredis pthread)