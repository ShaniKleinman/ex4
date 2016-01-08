#define _CRT_SECURE_NO_WARNINGS /* to suppress Visual Studio 2010 compiler warning */
#ifndef UTILS_H
#define UTILS_H

#include "list.h"
#include "server.h"

char* ConcatString(	char* source_a, char* source_b,char* source_c);
BOOL CheckIfNameValid(List* users, char* name,SOCKET socket);
DWORD LeaveSessionFlow(char* clientName,SOCKET socket,List* users,HANDLE mutex);
DWORD EnterSessionFlow(char* clientName,SOCKET socket,List* users,HANDLE mutex);
int SendPrivateMessage(char* userDest, char* message, SOCKET *sd, List* users);
int SendActiveUsers(SOCKET *sd, List* users, HANDLE mutex);
#endif