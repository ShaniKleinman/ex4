#define _CRT_SECURE_NO_WARNINGS /* to suppress Visual Studio 2010 compiler warning */
#ifndef UTILS_H
#define UTILS_H

#include "list.h"
#include "server.h"

char* ConcatString(	char* source_a, char* source_b,char* source_c);
BOOL RequestAccessFlow(List* users, char* name,SOCKET socket);
ErrorCode_t LeaveSessionFlow(char* clientName,SOCKET socket,List* users,HANDLE mutex);
ErrorCode_t SendPrivateMessage(char* userDestName, char* userSourceName, char* message, HANDLE mutex, List* users,SOCKET *sourceSocket);
ErrorCode_t SendActiveUsers(SOCKET *sd, List* users, HANDLE mutex,char* clientNameStr,char** systemMessage);
ErrorCode_t SendPublicMessage(char* arg1,char* clientNameStr,HANDLE MutexHandle,List* Users,SOCKET sourceSocket);
#endif