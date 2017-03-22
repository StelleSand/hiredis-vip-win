#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include "hiredis/hiredis.h"
#include "hiredis/hircluster.h"
#include "hiredis/async.h"
#include "hiredis/adapters/libevent.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "libevent.lib")
#pragma comment(lib, "libevent_core.lib")
#pragma comment(lib, "libevent_extras.lib")

struct event_base *base;

void PrintTime()
{
	SYSTEMTIME sys;
	GetLocalTime( &sys );
	printf( "%d.%d.%d\n", sys.wMinute, sys.wSecond, sys.wMilliseconds);
}

void test1()
{
	redisContext *c = redisConnect("127.0.0.1", 6379);
	if (c != NULL && c->err) {
		printf("Error: %s\n", c->errstr);
	} else {
		printf("Connected to Redis\n");
	}


	redisReply *reply;
	reply = (redisReply *)redisCommand(c,"SET %s %s","fddoo", "hello");
	freeReplyObject(reply);

	reply = (redisReply *)redisCommand(c,"MGET %s %s","foo", "hello");
	for (int i = 0; i < reply->elements; i++)
	{
		redisReply * p = reply->element[i];
		p->str;
		int d = 0;
	}
	printf("%s\n",reply->str);
	freeReplyObject(reply);

	redisFree(c);
}

void test2()
{
	redisClusterContext *cc = redisClusterConnect("127.0.0.1:7000", HIRCLUSTER_FLAG_NULL);
	if (cc != NULL && cc->err) {
		printf("Error: %s\n", cc->errstr);
		// handle error
	}

	for (int i = 0; i< 1000; i++)
	{
		redisReply * reply;
		reply = (redisReply *)redisClusterCommand(cc, "mset fo7 aaa fo8 bbb fo9 dddd");
		if (reply)
			freeReplyObject(reply);
	}

	redisClusterFree(cc);
}

int redisClusterStart(struct event_base* base);
void redisAsyncCommandCallback(redisClusterAsyncContext *c, void *r, void *privdata)   
{  
	redisReply *reply = (redisReply *)r;  
	static unsigned int icount= 0;
	if(NULL != reply)  
	{
		printf("redisAsyncCommandCallback: %lld, %s\n", reply->integer, reply->str);  
	}
	else  
	{
		printf("redisAsyncCommandCallback: reply is NULL\n");  
	}  
}

void redisAsyncConnectCallback(const redisAsyncContext *c, int status)   
{  
	if(status != REDIS_OK)  
	{  
		printf("redisAsyncConnectCallback failed\n");  
	}  
	else  
	{  
		printf("redisAsyncConnectCallback success\n");  
	}  
}

void redisAsyncDisconnectCallback(const redisAsyncContext *c, int status)   
{
	if(status != REDIS_OK)  
	{
		printf("redisAsyncDisconnectCallback failed\n");  
	}
	else  
	{  
		printf("redisAsyncDisconnectCallback success\n");  
	}
} 
int toRedisClusterAsyncCommand(redisClusterAsyncContext *c, char *command)  
{
	if(NULL == command)  
	{  
		printf("NULL == command \n");  
		return 1;  
	} 

	//execute redis command       
	if(REDIS_OK != redisClusterAsyncCommand(c, redisAsyncCommandCallback, NULL, command))  
	{  
		redisClusterStart(base);
		printf("redisClusterAsyncCommand fail: command=%s, errstr=%s\n", command, c->errstr);  
	}   
	return 0;  
}  

void timeout_cb(int fd, short event, void *params)
{
	redisClusterAsyncContext * pp = (redisClusterAsyncContext *)params;
	for (int i = 0; i<500000; i++)
	{
		char buf[1024] = {0};
		sprintf(buf, "DEL hqk_change_%d", i);
		toRedisClusterAsyncCommand((redisClusterAsyncContext *)params, buf);
	}
}
int redisClusterStart(struct event_base* base)
{
	static struct event* stimeevent = NULL;
	redisClusterAsyncContext *acc = redisClusterAsyncConnect("10.201.60.111:7000", HIRCLUSTER_FLAG_NULL);
	if (acc->err) {
		printf("Error: %s\n", acc->errstr);
		redisClusterAsyncFree(acc);
		return REDIS_ERR;
	}

	redisClusterLibeventAttach(acc, base);
	redisClusterAsyncSetConnectCallback(acc, redisAsyncConnectCallback);
	redisClusterAsyncSetDisconnectCallback(acc, redisAsyncDisconnectCallback);

	if (stimeevent)
	{
		evtimer_del(stimeevent);
		event_free(stimeevent);
		stimeevent = NULL;
	}
	struct timeval tv = {0, 100};
	stimeevent = event_new(base, -1, EV_READ|EV_PERSIST, timeout_cb, acc);
	evtimer_add(stimeevent, &tv);
	return REDIS_OK;
}

void test3()
{
	base = event_base_new();//新建一个libevent事件处理 
	redisClusterStart(base);
	event_base_dispatch(base);//开始libevent循环
}

void main()
{
	test3();
}
