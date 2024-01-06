Local-dictionary

功能要求: 
1. 客户端每次连接服务器时,都需要登陆或注册对应的账号和密码;
2. 如果客户端没有账号,则应该注册账号和密码;
3. 客户端登陆成功后, 应该具有检索单词,查看历史的功能;
4. 可以根据自己情况自行添加其他功能;


编译： gcc client.c -o client -lpthread -lsqlite3
执行：./client <ip> <port>

编译：gcc server.c -o server -lsqlite3 -lpthread
执行：./server <port>
