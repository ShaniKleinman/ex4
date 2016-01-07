#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <Windows.h>
#include <ctype.h>
#include <assert.h>
#include <tchar.h>

#include "utils.h"
#include "list.h"
#include "server.h"
#include "SocketSendRecvTools.h"



char* ConcatString(	char* source_a, char* source_b,char* source_c)
{
	int total_size = strlen(source_a)+strlen(source_b)+strlen(source_c);
	char* string = (char*) malloc(total_size* sizeof(char));
	if (string == NULL)
	{
		exit(1);
	}
	strcpy(string,source_a);
	strcat(string,source_b);
	strcat(string,source_c);
	return string;
}

BOOL CheckIfNameValid(List* users, char* name,SOCKET socket)
{
	int i;
	Node* node = ReturnElementByName(users,name);
	if (node == NULL)
	{
		AddElementAtEnd(users,name,socket);
		return TRUE;
	}
	if (node->activeStatus == NOT_ACTIVE)
	{
		node->activeStatus = ACTIVE;
		node->socket = socket;
		return TRUE;
	}
	return FALSE;
}


DWORD LeaveSessionFlow(char* clientName,SOCKET socket,List* users,HANDLE mutex)
{
	DWORD WaitRes;
	List* activeUsers;
	Node* userNode;
	Node* currentNode;
	char* message;
	__try 
	{
		WaitRes = WaitForSingleObject( mutex, INFINITE );
		switch (WaitRes) 
		{
		case WAIT_OBJECT_0: 
			{
				userNode=ReturnElementByName(users, clientName);
				userNode->activeStatus=NOT_ACTIVE;
				userNode->socket=NULL;
				currentNode = users->firstNode;
				while(currentNode != NULL)
				{
					if(currentNode->activeStatus== ACTIVE)
					{
						message = ConcatString("*** ",clientName," has left the session ***");
						SendString(message,currentNode->socket);
					}
					currentNode = currentNode->next;
				}
			} 
			break; 
		case WAIT_ABANDONED: 
			return ISP_MUTEX_ABANDONED; 
		}
	}
	__finally 
	{ 
		if ( !ReleaseMutex(mutex)) {
			return ( ISP_MUTEX_RELEASE_FAILED );
		} 
		//printf("%d is releasing mutex %d\n",GetCurrentThreadId(),MutexHandleTop);
		return ( ISP_SUCCESS );
	}
}
DWORD EnterSessionFlow(char* clientName,SOCKET socket,List* users,HANDLE mutex)
{
	DWORD WaitRes;
	List* activeUsers;
	Node* userNode;
	Node* currentNode;
	char* message;
	__try 
	{
		WaitRes = WaitForSingleObject( mutex, INFINITE );
		switch (WaitRes) 
		{
		case WAIT_OBJECT_0: 
			{
				userNode=ReturnElementByName(users, clientName);
				currentNode = users->firstNode;
				while(currentNode != NULL)
				{
					if(currentNode->activeStatus== ACTIVE && !STRINGS_ARE_EQUAL(currentNode->name,userNode->name))
					{
						message = ConcatString("*** ",clientName," has joined the session ***");
						SendString(message,currentNode->socket);
					}
					currentNode = currentNode->next;
				}
			} 
			break; 
		case WAIT_ABANDONED: 
			return ISP_MUTEX_ABANDONED; 
		}
	}
	__finally 
	{ 
		if ( !ReleaseMutex(mutex)) {
			return ( ISP_MUTEX_RELEASE_FAILED );
		} 
		//printf("%d is releasing mutex %d\n",GetCurrentThreadId(),MutexHandleTop);
		return ( ISP_SUCCESS );
	}
}
