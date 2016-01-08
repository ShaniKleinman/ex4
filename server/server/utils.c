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

BOOL RequestAccessFlow(List* users, char* name,SOCKET socket)
{
	int i;
	Node* node = ReturnElementByName(users,name);
	Node* currentNode;
	BOOL statusRequest = FALSE;
	char* message;
	if (node == NULL)
	{
		node = AddElementAtEnd(users,name,socket);
		statusRequest= TRUE;
	}
	else if (node->activeStatus == NOT_ACTIVE)
	{
		node->activeStatus = ACTIVE;
		node->socket = socket;
		statusRequest= TRUE;
	}
	if (statusRequest == TRUE)
	{
		currentNode = users->firstNode;
		while(currentNode != NULL)
		{
			if(currentNode->activeStatus== ACTIVE && !STRINGS_ARE_EQUAL(currentNode->name,node->name))
			{
				message = ConcatString("*** ",name," has joined the session ***");
				SendString(message,currentNode->socket);
			}
			currentNode = currentNode->next;
		}
	}
	return statusRequest;
}

ErrorCode_t LeaveSessionFlow(char* clientName,SOCKET socket,List* users,HANDLE mutex)
{
	DWORD WaitRes;
	Node* userNode;
	Node* currentNode;
	TransferResult_t sendRes;
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
						sendRes = SendString(message,currentNode->socket);
						if ( sendRes == TRNS_FAILED ) 
						{
							printf( "Service socket error while writing, closing thread.\n" );
							currentNode->activeStatus= NOT_ACTIVE;
							closesocket( currentNode->socket );
							currentNode->socket= NULL;

						}
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

ErrorCode_t SendActiveUsers(SOCKET *sd, List* users, HANDLE mutex,char* clientNameStr,char** systemMessage)
{
	DWORD WaitRes;
	Node* currentNode = users->firstNode;
	Node* userNode;
	char* activeUsersNameList;
	BOOL firstActiveUser= FALSE;
	TransferResult_t sendRes;
	activeUsersNameList = (char*) malloc (MAX_USER_NAME_LENGTH*sizeof(*activeUsersNameList));
	if (activeUsersNameList == NULL)
	{
		exit(1);
	}
	__try 
	{
		WaitRes = WaitForSingleObject( mutex, INFINITE );
		switch (WaitRes) 
		{
		case WAIT_OBJECT_0: 
			{
				while(currentNode != NULL)
				{
					if(currentNode->activeStatus== ACTIVE && ! firstActiveUser)
					{
						strcpy(activeUsersNameList,currentNode->name);
						firstActiveUser=TRUE;
					}
					else if (currentNode->activeStatus== ACTIVE)
					{
						activeUsersNameList = ConcatString(activeUsersNameList,", ",currentNode->name);
					}
					currentNode = currentNode->next;
				}
				*systemMessage = activeUsersNameList;
				sendRes = SendString(activeUsersNameList,*sd);
				if ( sendRes == TRNS_FAILED ) 
				{
					printf( "Service socket error while writing, closing thread.\n" );
					userNode = ReturnElementByName(users,clientNameStr);
					userNode->activeStatus = NOT_ACTIVE;
					closesocket( userNode->socket );
					userNode->socket=NULL;
					return ISP_NO_SUCCESS;
				}
			}
			break; 
		case WAIT_ABANDONED: 
			return ISP_MUTEX_ABANDONED; 
		}
	}
	__finally 
	{ 
		if ( !ReleaseMutex(mutex)) 
		{
			return ( ISP_MUTEX_RELEASE_FAILED );
		} 
		
		//printf("%d is releasing mutex %d\n",GetCurrentThreadId(),MutexHandleTop);
		return ( ISP_SUCCESS );
	}

	return 0;
}

ErrorCode_t SendPrivateMessage(char* userDestName, char* userSourceName, char* message, HANDLE mutex, List* users,SOCKET *sourceSocket)
{
	DWORD WaitRes;
	Node* userDestNode;
	Node* userSourceNode;
	TransferResult_t sendRes;
	char* messageConcat;
	__try 
	{
		WaitRes = WaitForSingleObject( mutex, INFINITE );
		switch (WaitRes) 
		{
		case WAIT_OBJECT_0: 
			{
				userDestNode = ReturnElementByName(users,userDestName);
				if (userDestNode != NULL && userDestNode->activeStatus == ACTIVE)
				{
					messageConcat = ConcatString(userSourceName,": ",message);
					sendRes = SendString(messageConcat,userDestNode->socket);
					if ( sendRes == TRNS_FAILED ) 
					{
						printf( "Service socket error while writing, closing thread.\n" );
						userDestNode->activeStatus = NOT_ACTIVE;
						closesocket( userDestNode->socket );
						userDestNode->socket=NULL;
						return ISP_NO_SUCCESS;
					}
				}
				else
				{
					messageConcat = ConcatString("No such user ","",userDestName);
					sendRes = SendString(messageConcat,*sourceSocket);
					if ( sendRes == TRNS_FAILED ) 
					{
						printf( "Service socket error while writing, closing thread.\n" );
						userSourceNode = ReturnElementByName(users,userSourceName);
						userSourceNode->activeStatus= NOT_ACTIVE;
						closesocket( userSourceNode->socket );
						userSourceNode->socket= NULL;
						return ISP_NO_SUCCESS;
					}
				}
			}
			break; 
		case WAIT_ABANDONED: 
			return ISP_MUTEX_ABANDONED; 
		}
	}
	__finally 
	{ 
		if ( !ReleaseMutex(mutex)) 
		{
			return ( ISP_MUTEX_RELEASE_FAILED );
		} 
		//printf("%d is releasing mutex %d\n",GetCurrentThreadId(),MutexHandleTop);
		return ( ISP_SUCCESS );
	}

	return 0;
}

ErrorCode_t SendPublicMessage(char* arg1,char* clientNameStr,HANDLE mutexHandle,List* users,SOCKET sourceSocket)
{
	DWORD WaitRes;
	Node* sourceNode;
	Node* currentNode;
	TransferResult_t sendRes;
	char* messageConcat;
	__try 
	{
		WaitRes = WaitForSingleObject( mutexHandle, INFINITE );
		switch (WaitRes) 
		{
		case WAIT_OBJECT_0: 
			{
				sourceNode = ReturnElementByName(users,clientNameStr);
				currentNode = users->firstNode;
				while(currentNode != NULL)
				{
					if(currentNode->activeStatus== ACTIVE && !STRINGS_ARE_EQUAL(currentNode->name,sourceNode->name))
					{
						messageConcat = ConcatString(clientNameStr,": ",arg1);
						sendRes = SendString(messageConcat,currentNode->socket);
						if ( sendRes == TRNS_FAILED ) 
						{
							printf( "Service socket error while writing, closing thread.\n" );
							currentNode->activeStatus= NOT_ACTIVE;
							closesocket( currentNode->socket );
							currentNode->socket= NULL;
							return ISP_NO_SUCCESS;
						}
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
		if ( !ReleaseMutex(mutexHandle)) 
		{
			return ( ISP_MUTEX_RELEASE_FAILED );
		} 
		//printf("%d is releasing mutex %d\n",GetCurrentThreadId(),MutexHandleTop);
		return ( ISP_SUCCESS );
	}

	return 0;
}

