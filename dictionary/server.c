#include "head.h"

sqlite3 *db = NULL;
char *errmsg = NULL;
char **sp = NULL;
int hang, lie;

void *handler(void *arg);
void server_register(int acceptfd, msg_t msg);
void server_login(int acceptfd, msg_t msg);
void server_history(int acceptfd, msg_t msg);
void server_query(int acceptfd, word_t msd, msg_t msg);
void server_update(int acceptfd, msg_t msg);
int callback(void *arg, int f_num, char **f_value, char **f_name);

int main(int argc, char const *argv[])
{
    //1.创建套接字
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("socker is err:");
        return -1;
    }
    //2.填充结构体
    struct sockaddr_in saddr, caddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(atoi(argv[1]));
    saddr.sin_addr.s_addr = inet_addr("0.0.0.0");

    int len = sizeof(caddr);
    //绑定
    if (bind(sockfd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0)
    {
        perror("bind is err:");
        return -1;
    }

    //3.监听
    if (listen(sockfd, 5) < 0)
    {
        perror("listen is err:");
        return -1;
    }
    msg_t msg;
    word_t msd;
    //创建 或 打开库
    if (sqlite3_open("./my.dictionary", &db) != 0)
    {
        //perror 不能，因为sqlite库是外加的库
        //fprintf: 使用格式化输出到流中
        fprintf(stderr, "sqlite3_open is err: %s\n", sqlite3_errmsg(db));
        return -1;
    }
    // 创建 用户信息表  单词表   历史记录表
    sqlite3_exec(db, "create table user(name char primary key,password char);", NULL, NULL, &errmsg);
    sqlite3_exec(db, "create table word(words char,mean char);", NULL, NULL, &errmsg);
    sqlite3_exec(db, "create table history(name char,time char,world char);", NULL, NULL, &errmsg);

    printf("等待连接--------\n");
    //4.阻塞等待客户端链接
    while (1)
    {
        //建立通信的文件描述符
        int acceptfd = accept(sockfd, (struct sockaddr *)&caddr, &len);
        if (acceptfd < 0)
        {
            perror("accept is err:");
            return -1;
        }
        printf("server连接成功---\n");
        printf("ip: %s  port: %d\n", inet_ntoa(caddr.sin_addr), ntohs(caddr.sin_port));

        //发送或接受
        pthread_t tid;
        pthread_create(&tid, NULL, handler, &acceptfd);
        //子线程退出后，自动回收资源  - 非阻塞-
        pthread_detach(tid);
    }
}

void *handler(void *arg)
{
    msg_t msg;
    word_t msd;
    int acceptfd = *((int *)arg);
    while (1)
    {
        int recvbyte = recv(acceptfd, &msg, sizeof(msg), 0);
        if (recvbyte < 0)
        {
            perror("recv is err:");
            return NULL;
        }
        else if (recvbyte == 0) //客户端退出
        {
            printf(" %d  client is exit\n", acceptfd);

            break;
        }
        else
        {
            switch (msg.type)
            {
            case 'R':
                server_register(acceptfd, msg);
                break; //注册
            case 'L':
                server_login(acceptfd, msg);
                break; //登录
            case 'C':
                server_update(acceptfd, msg);
                break; //更新
            case 'Q':
                server_query(acceptfd, msd, msg);
                break; //查找
            case 'H':
                server_history(acceptfd, msg);
                break; //历史
            case 'D':
                printf("%d client is exit\n", acceptfd);
                break; //退出
            }
        }
    }
}
//注册
void server_register(int acceptfd, msg_t msg)
{
    char sql[128];
    sprintf(sql, "insert into user values(\"%s\",\"%s\");", msg.name, msg.password);

    if (sqlite3_exec(db, sql, NULL, NULL, &errmsg) != 0)
    {
        fprintf(stderr, "insert user is err: %s\n", errmsg);
        msg.type = 'N';
    }
    else
    {
        msg.type = 'Y';
        printf("insert username and password is success\n");
    }
    send(acceptfd, &msg, sizeof(msg), 0);
}
//登录
//查找 name 和 passowrd 是否在用户表内， 如果在则登录成功
void server_login(int acceptfd, msg_t msg)
{
    char sql[128];
    sprintf(sql, "select *from user where name=\"%s\"and password =\"%s\";", msg.name, msg.password);

    if (sqlite3_get_table(db, sql, &sp, &hang, &lie, &errmsg) != SQLITE_OK)
    {
        fprintf(stderr, "select user is err:%s \n", errmsg);
    }
    if (hang == 0)
    {
        msg.type = 'N';
        printf("login err\n");
    }
    else
    {
        msg.type = 'Y';
        printf("login success\n");
    }
    send(acceptfd, &msg, sizeof(msg), 0);
}

//查询单次
void server_query(int acceptfd, word_t msd, msg_t msg)
{
    int recvtype;
    int nrow, lie;
    int i, j, k = 0;
    char sql[128] = " ";
    //不断接受客户端要查寻的内容
    while (1)
    {
        recvtype = recv(acceptfd, &msd, sizeof(msd), 0);
        if (recvtype < 0)
        {
            perror("recv is err:");
            return;
        }
        else if (recvtype == 0)
        {
            printf("client is err:\n");
            break;
        }
        else
        {
            //如果客户端发送的内容为退出， 则退出， 该内容最好不是单词
            if (strncmp(msd.word, "quit!", 5) == 0)
            {
                printf("client exit this found\n");
                break;
            }
            //查询单词
            sprintf(sql, "select * from word where words = \"%s\";", msd.word);
            if (sqlite3_get_table(db, sql, &sp, &nrow, &lie, &errmsg) != SQLITE_OK)
            {
                fprintf(stderr, "found word err:%s\n", errmsg);
                return;
            }
            else if (nrow == 0) //满足条件的行数为0，说明没有找到该单词
            {
                strcpy(msd.data, "no found");
                send(acceptfd, &msd, sizeof(msd), 0);
            }
            else
            {
                //sp[0]为 第一列名: word   sp[1]为 第二列名: mean  sp[2]为 单词   sp[3]为 注释
                sprintf(msd.data, "%s : %s\n", sp[2], sp[3]);
                send(acceptfd, &msd, sizeof(msd), 0);

                //每次查询成功后,记录时间传入历史表格
                char buf[1024];
                char sql[128];
                time_t tm;
                struct tm *tp;
                //获取当前系统的秒数，并存放在tm中
                time(&tm);
                //将time获取到的事件转换为 年月日 时分秒， 返回值为保存时间的地址
                tp = localtime(&tm); //现在时间
                //参考笔记，因为struct tm中， year -1900， 所以我们需要加上1900  mon为 0- 11 ， 所以我们需要再加1
                sprintf(buf, "%d/%d/%d  %d:%d:%d", tp->tm_year + 1900, tp->tm_mon + 1, tp->tm_mday, tp->tm_hour, tp->tm_min, tp->tm_sec);
                //客户端的name 事件buf 单词word 插入到历史表内
                sprintf(sql, "insert into history values(\"%s\",\"%s\",\"%s\");", msg.name, buf, msd.word);
                if (sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK)
                {
                    fprintf(stderr, "insert time err:%s\n", errmsg);
                    return;
                }
            }
        }
    }
}

void server_history(int acceptfd, msg_t msg)
{
    char sql[128] = "";
    //查询历史记录表
    sprintf(sql, "select * from history where name = \"%s\";", msg.name);
    //将acceptfd传入回调函数
    if (sqlite3_exec(db, sql, callback, &acceptfd, &errmsg) != SQLITE_OK)
    {
        printf("error: %s\n", errmsg);
    }

    strncpy(msg.data, "quit!", 5); //历史记录检索完毕,发送退出指令
    send(acceptfd, &msg, sizeof(msg), 0);

}

//回调函数   每找到一次记录,自动执行一次回调
//char **f_value针指向查询结果的内容
int callback(void *arg, int f_num, char **f_value, char **f_name)
{
    int acceptfd = *(int *)arg;
    msg_t msg;
    //f_value[0] 用户名 f_value[1] 时间    f_value[2] 单词
    sprintf(msg.data, "%s: %s -- %s", f_value[0], f_value[1], f_value[2]);//历史表中的数据
    send(acceptfd, &msg, sizeof(msg), 0); //将找到的内容以指定格式,发送给客户端
    return 0;
}

//更新单词表
void server_update(int acceptfd, msg_t msg)
{
    char buf[2048];
    char word[2048];
    char sql[128];
    //这个可以写完删除再加入
    //先删除旧的单词表， 否则重复插入单词文件的数据
    strcpy(sql, "delete from word;");
    sqlite3_exec(db, sql, NULL, NULL, &errmsg);

    //打开单词文件
    FILE *fd = fopen("./dict.txt", "r");
    if (fd == NULL)
    {
        perror("fopen is err:");
        strcpy(msg.data, "open is err:");
        send(acceptfd, &msg, sizeof(msg), 0);

        return;
    }
    //导入词典,根据词典的规律来写
    //使用fgets, 一行一行读取最好
    //读取的内容, 不能直接存放在结构体内, 我们需要区分单词 和 注释
    while (fgets(buf, sizeof(buf), fd) != NULL)
    {
        char *p = buf;
        while (1)
        {
            //p指针向后移动
            p++;
            //如果p移动到了空格
            if ((*p == ' '))
            {
                //将第一个空格赋值为 '\0'
                *p = '\0';
                //此时, buf的内容为:  zoo\0,
                strcpy(word, buf);
                break;
            }
        }
        //跳过当前的 *p = '\0', 后面的如果仍然是 ' ',则继续跳过
        p++;
        while (*p == ' ')
            p++;
        //结束while循环后, 此时p指向的地址, 一定不是 ' ' 了,而是注释
        printf("%s\n", word);
        //插入单词表
        sprintf(sql, "insert into word values(\"%s\",\"%s\");", word, p);
        sqlite3_exec(db, sql, NULL, NULL, &errmsg);
    }
    printf("updata table is success\n");
    //更新完毕后， 将完毕信息告诉客户端
    strcpy(msg.data, "updata table is success");
    send(acceptfd, &msg, sizeof(msg), 0);
}