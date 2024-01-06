#include "head.h"

void client_login(int sockfd, msg_t msg, word_t msd);
void client_register(int sockfd, msg_t msg);
void client_query(int socket, msg_t msg, word_t msd);
void client_history(int socket, msg_t msg);
void client_update(int sockfd, msg_t msg);
void login_success_show();
void show();
void client_success(int sockfd, msg_t msg, word_t msd);

int main(int argc, char const *argv[])
{
    // 1. 创建套接字
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("socket 错误：");
        return -1;
    }
    // 2. 填充结构体
    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(atoi(argv[2]));
    saddr.sin_addr.s_addr = inet_addr(argv[1]);

    // 链接
    if (connect(sockfd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0)
    {
        perror("connect 错误：");
        return -1;
    }
    printf("客户端连接成功---\n");

    msg_t msg;
    word_t msd;

    while (1)
    {
        show(); // 菜单
        printf("请输入你的选项号码：");
        scanf("%d", &msg.type);
        if ((msg.type == 1) || (msg.type == 2) || (msg.type == 3))
        {
            switch (msg.type)
            {
            // 注册
            case 1:
                client_register(sockfd, msg);
                break;
            case 2:
                client_login(sockfd, msg, msd);
                break;
            case 3:
                exit(0);
            }
        }
        else
        {
            printf("无效输入，请输入1-3。\n");
            continue;
        }
    }

    return 0;
}

void client_register(int sockfd, msg_t msg)
{
    // 将 type 设置为 注册
    msg.type = 'R';
    printf("请输入你的注册用户名：");
    scanf("%s", msg.name);
    printf("请输入你的注册密码：");
    scanf("%s", msg.password);

    send(sockfd, &msg, sizeof(msg), 0);
    // 等待服务器的反馈是否成功
    int recvbyte = recv(sockfd, &msg, sizeof(msg), 0);
    if (recvbyte < 0)
    {
        perror("接收错误：");
        return;
    }
    else
    {
        switch (msg.type)
        {
        case 'Y':
            printf("注册成功\n");
            break;

        case 'N':
            printf("帐号密码已存在\n");
            break;
        }
    }
}

void client_login(int sockfd, msg_t msg, word_t msd)
{
    msg.type = 'L';
    printf("\n请输入你的帐号：");
    scanf("%s", msg.name);
    printf("\n请输入你的密码：");
    scanf("%s", msg.password);

    send(sockfd, &msg, sizeof(msg), 0); // 发送帐号密码给服务器

    // 等待服务器的反馈是否成功
    int recvbyte = recv(sockfd, &msg, sizeof(msg), 0);
    if (recvbyte < 0)
    {
        perror("接收错误：");
        return;
    }
    else
    {
        switch (msg.type)
        {
        case 'N':
            printf("usarname or password is err:\n");
            break;
        case 'Y':
            putchar(10);
            printf("登录成功\n");
            client_success(sockfd, msg, msd); //进入另一个功能列表
            break;
            // 打印以下新的功能:  更新单词  查询单次  历史  退出
            // 然后终端输入 新的功能， 发送给服务器，然后执行新的功能函数
        }
    }
}
void client_success(int sockfd, msg_t msg, word_t msd)
{

    while (1)
    {
        login_success_show();
        printf("请输入你的选项号码：");
        scanf("%d", &msg.type);
        putchar(10);
        if ((msg.type == 0) || (msg.type == 1) || (msg.type == 2) || (msg.type == 3) || (msg.type == 4))
        {
            // 处理服务器的响应
            switch (msg.type)
            {
            case 1:
                // 执行查询的功能
                client_query(sockfd, msg, msd);
                break;
            case 2:
                // 执行查看历史记录的功能
                client_history(sockfd, msg);
                break;
            case 3:
                client_update(sockfd, msg);
                break;
            case 4:
                msg.type = 'D'; //状态为退出  发送给服务器
                send(sockfd, &msg, sizeof(msg), 0);
                return;
            default:
                printf("无效输入，请输入1-4。\n");
                break;
            }
        }
    }
}
//更新单词
void client_update(int sockfd, msg_t msg)
{
    printf("updata the word..........\n");
    //状态为 更新单词
    msg.type = 'C';
    send(sockfd, &msg, sizeof(msg), 0);

    //如果服务器更新完毕单词表后， 发送一个更新完毕的语句， 客户端接受后，打印更新完毕
    recv(sockfd, &msg, sizeof(msg), 0);
    printf("%s\n", msg.data);
}

void client_query(int sockfd, msg_t msg, word_t msd)
{
    int recvbyte;
    //将状态置为 查询 后发送给服务器
    msg.type = 'Q';
    send(sockfd, &msg, sizeof(msg), 0);

    while (1)
    {
        printf("please input the word:");
        //输入查找的单词
        scanf("%s", msd.word);
        putchar(10);
        send(sockfd, &msd, sizeof(msd), 0);

        //退出循环的方式，最好不要是单词， 输入quit！ 才退出   注意 ！ 为英文， 否则接受不到
        if (strncmp(msd.word, "quit!", 5) == 0)
        {
            printf("退出查询\n");
            return;
        }
        //接受服务器查询到的单词意思
        if ((recvbyte = recv(sockfd, &msd, sizeof(msd), 0)) < 0)
        {
            perror("recv login err:");
            return;
        }
        else
        {
            printf("%2s\n", msd.data);
        }
    }
}

void client_history(int sockfd, msg_t msg)
{
    //状态为历史
    msg.type = 'H';
    int recvbyte;
    //告诉服务器执行历史功能
    send(sockfd, &msg, sizeof(msg), 0);

    while (1)
    {
        recvbyte = recv(sockfd, &msg, sizeof(msg), 0);
        if (recvbyte < 0)
        {
            perror("recv history err");
            return;
        }
        else
        {
            if (strncmp(msg.data, "quit!", 5) == 0) //收到该指令,说明历史记录检索完毕,可以退出
                break;
            printf("%s\n", msg.data); //打印检索的历史记录
        }
        return;
    }
}
void login_success_show()
{
    putchar(10);
    printf("**********************************************\n");
    printf(" 1.查询 2.历史记录  3.更新词典 4.退出\n");
    printf("*********************************************\n");
    putchar(10);
}

void show()
{
    putchar(10);
    printf("**********************************************\n");
    printf("1. 注册   2. 登录  3. quit\n");
    printf("*********************************************\n");
    putchar(10);
}