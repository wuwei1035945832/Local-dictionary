#ifndef _HEAD_H_
#define _HEAD_H_

#include <stdio.h>
#include <sqlite3.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h> 
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

//专门传输 用户名 状态 密码 信息等
typedef struct message  
{
    int type; 
    char name[32];
    char password[32];
    char data[128];
}msg_t;

//用于查询单词, 注释的传输
typedef struct words
{
    char word[4096];
    char data[4096];
}word_t;
              
#endif
