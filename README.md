# epoll-server
A epoll  server based thread pool.

- 监听6666端口
- 客户端发送以'\n'结尾的字符串，服务端翻转后返回。比如客户端发送"hello world\n"，服务端返回"dlrow olleh\n"
- 服务器使用4线程，测试Client请求在4B~4KB之间时，服务端的qps和流量

## Usage

```shell
$> make
```

```shell
$> ./server
$> ./client 5
```

```
final_finish: 64000 
cost: 503
qps: 12.7237 w/s
rate: 0.485369 MBps
```

