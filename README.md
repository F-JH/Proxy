操作系统：macos

ProxyServer是一个基于boost.asio和boost.beast的中间人代理工具（类似fiddler/mitmproxy)，可编译成静态库libProxyServer.a，对外提供的接口中，add_mock需要传入函数指针 OnRequest或者OnResponse，该函数指针的入参使用了boost::beast::http::request和boost::beast::http::response，因此要使用ProxyServer，需要安装boost.beast，或者可以改造一下将request和response包装一层（但我懒得弄了），对外不输出boost的符号，库的使用者就不用安装boost了。

因为需要签发个人证书，所以用到了openssl的库，但是相关符号不会暴露给库的使用者，因此打包成静态库后使用者不需要连接openssl的库

项目路径下有CMakeLists.txt和makefile，如果需要编译成静态库，建议用makefile

```shell
# 编译ProxyServer库
make

# 编译example
make example

# build目录下找到lib/libProxyServer.a、lib/libProxyServer.dylib
# build/include 目录下找到对外输出的头文件
# build/example/example_main 即为例子
```

使用cmake可以直接用Clion打开
