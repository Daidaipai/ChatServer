#cmake文件有固定的名字即CMakeLists.txt,在终端执行(cmake . )命令
#会生成一个Makefile文件，然后执行(make )命令就会生成可执行文件server

add_subdirectory(server)
add_subdirectory(client)

# 配置头文件搜索路径
# include_directories()
# 配置库文件搜索路径
# link_directories()


