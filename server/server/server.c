/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/
/* 
 This file was written for instruction purposes for the 
 course "Introduction to Systems Programming" at Tel-Aviv
 University, School of Electrical Engineering.
Last updated by Amnon Drory, Winter 2011.
 */
/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <Windows.h>
#include <ctype.h>
#include <assert.h>
#include <tchar.h>

#include "list.h"
#include "server.h"
#include "utils.h"
#include "SocketSendRecvTools.h"
#include "SocketExampleShared.h"

/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/
#define SEND_STR_SIZE 35
/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/

HANDLE* ThreadHandles;
SOCKET* ThreadInputs;
FILE *ServerLog;
/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/

int NUM_OF_WORKER_THREADS;
int MAX_LOOPS;
static int FindFirstUnusedThreadSlot();
static void CleanupWorkerThreads();
static DWORD ServiceThread( SOCKET *t_socket );

LPCTSTR MutexName = _T( "MutexTop" );
HANDLE MutexHandle;
SOCKET MainSocket;

/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/
void InitParams(int maxClients);
List* Users;
/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/

HANDLE CreateMutexSimple( LPCTSTR MutexName )
{
	return CreateMutex( 
		NULL,              // default security attributes
		FALSE,             // initially not owned
		MutexName);             
}

void MainServer(int portNumber,int maxClients)
{
	int Ind;
	int Loop;
	unsigned long Address;
	SOCKADDR_IN service;
	int bindRes;
	int ListenRes;
	WSADATA wsaData;
	// Initialize Winsock.
    int StartupRes = WSAStartup( MAKEWORD( 2, 2 ), &wsaData );	           
	ServerLog = fopen("server_log.text","w+");
	MainSocket = INVALID_SOCKET;

    if ( StartupRes != NO_ERROR )
	{
        printf( "error %ld at WSAStartup( ), ending program.\n", WSAGetLastError() );
		// Tell the user that we could not find a usable WinSock DLL.                                  
		return;
	}
	
    /* The WinSock DLL is acceptable. Proceed. */
	InitParams(maxClients);

    // Create a socket.    
    MainSocket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );

    if ( MainSocket == INVALID_SOCKET ) 
	{
        printf( "Error at socket( ): %ld\n", WSAGetLastError( ) );
		exit(1);
    }

    // Bind the socket.
	/*
		For a server to accept client connections, it must be bound to a network address within the system. 
		The following code demonstrates how to bind a socket that has already been created to an IP address 
		and port.
		Client applications use the IP address and port to connect to the host network.
		The sockaddr structure holds information regarding the address family, IP address, and port number. 
		sockaddr_in is a subset of sockaddr and is used for IP version 4 applications.
   */
	// Create a sockaddr_in object and set its values.
	// Declare variables

	Address = inet_addr( SERVER_ADDRESS_STR );
	if ( Address == INADDR_NONE )
	{
		printf("The string \"%s\" cannot be converted into an ip address. ending program.\n",
				SERVER_ADDRESS_STR );
		exit(1);
	}

    service.sin_family = AF_INET;
    service.sin_addr.s_addr = Address;
    service.sin_port = htons( portNumber ); //The htons function converts a u_short from host to TCP/IP network byte order 
	                                   //( which is big-endian ).
	/*
		The three lines following the declaration of sockaddr_in service are used to set up 
		the sockaddr structure: 
		AF_INET is the Internet address family. 
		"127.0.0.1" is the local IP address to which the socket will be bound. 
	    2345 is the port number to which the socket will be bound.
	*/

	// Call the bind function, passing the created socket and the sockaddr_in structure as parameters. 
	// Check for general errors.
    bindRes = bind( MainSocket, ( SOCKADDR* ) &service, sizeof( service ) );
	if ( bindRes == SOCKET_ERROR ) 
	{
        printf( "bind( ) failed with error %ld. Ending program\n", WSAGetLastError( ) );
		exit(1);
	}
    
    // Listen on the Socket.
	ListenRes = listen( MainSocket, SOMAXCONN );
    if ( ListenRes == SOCKET_ERROR ) 
	{
        printf( "Failed listening on socket, error %ld.\n", WSAGetLastError() );
		exit(1);
	}

	MutexHandle = CreateMutexSimple( MutexName );
	if (MutexHandle == NULL) 
	{
		printf("CreateMutex error: %d\n", GetLastError());
	}

	// Initialize all thread handles to NULL, to mark that they have not been initialized
	for ( Ind = 0; Ind < NUM_OF_WORKER_THREADS; Ind++ )
		ThreadHandles[Ind] = NULL;

    printf( "Waiting for a client to connect...\n" );
    
	for ( Loop = 0; Loop < MAX_LOOPS; Loop++ )
	{
		SOCKET AcceptSocket = accept( MainSocket, NULL, NULL );
		if ( AcceptSocket == INVALID_SOCKET )
		{
			printf( "Accepting connection with client failed, error %ld\n", WSAGetLastError() ) ; 
			CleanupWorkerThreads();
			exit(1);
		}

        printf( "Client Connected.\n" );

		Ind = FindFirstUnusedThreadSlot();

		if ( Ind == NUM_OF_WORKER_THREADS ) //no slot is available
		{ 
			printf( "No available socket at the moment. Try again later." );
			if (SendString( "No available socket at the moment. Try again later.", AcceptSocket ) == TRNS_FAILED ) 
			{
				printf( "Service socket error while writing, closing thread.\n" );
				closesocket( AcceptSocket );
				exit(1);
			}
			closesocket( AcceptSocket ); //Closing the socket, dropping the connection.
		} 
		else 	
		{
			ThreadInputs[Ind] = AcceptSocket; // shallow copy: don't close 
											  // AcceptSocket, instead close 
											  // ThreadInputs[Ind] when the
											  // time comes.
			ThreadHandles[Ind] = CreateThread(
				NULL,
				0,
				( LPTHREAD_START_ROUTINE ) ServiceThread,
				&( ThreadInputs[Ind] ),
				0,
				NULL
			);
		}
    } 


}

/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/

static int FindFirstUnusedThreadSlot()
{ 
	int Ind;

	for ( Ind = 0; Ind < NUM_OF_WORKER_THREADS; Ind++ )
	{
		if ( ThreadHandles[Ind] == NULL )
			break;
		else
		{
			// poll to check if thread finished running:
			DWORD Res = WaitForSingleObject( ThreadHandles[Ind], 0 ); 
				
			if ( Res == WAIT_OBJECT_0 ) // this thread finished running
			{				
				CloseHandle( ThreadHandles[Ind] );
				ThreadHandles[Ind] = NULL;
				break;
			}
		}
	}

	return Ind;
}

/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/

static void CleanupWorkerThreads()
{
	int Ind; 

	for ( Ind = 0; Ind < NUM_OF_WORKER_THREADS; Ind++ )
	{
		if ( ThreadHandles[Ind] != NULL )
		{
			// poll to check if thread finished running:
			DWORD Res = WaitForSingleObject( ThreadHandles[Ind], INFINITE ); 
				
			if ( Res == WAIT_OBJECT_0 ) 
			{
				closesocket( ThreadInputs[Ind] );
				CloseHandle( ThreadHandles[Ind] );
				ThreadHandles[Ind] = NULL;
				break;
			}
			else
			{
				printf( "Waiting for thread failed. Ending program\n" );
				return;
			}
		}
	}
}

DWORD HandleAccessRequest(char* AcceptedStr,AccessResult* accessResult,SOCKET socket)
{
	DWORD WaitRes;
	__try 
	{
		WaitRes = WaitForSingleObject( MutexHandle, INFINITE );
		switch (WaitRes) 
		{
		case WAIT_OBJECT_0: 
			{
				if (RequestAccessFlow(Users,AcceptedStr,socket))
				{
					*accessResult = ACCESS;
				}
				else
				{
					*accessResult = NO_ACCESS;
				}
			} 
			break; 
		case WAIT_ABANDONED: 
			return ISP_MUTEX_ABANDONED; 
		}
	}
	__finally 
	{ 
		if ( !ReleaseMutex(MutexHandle)) {
			return ( ISP_MUTEX_RELEASE_FAILED );
		} 
		//printf("%d is releasing mutex %d\n",GetCurrentThreadId(),MutexHandleTop);
		return ( ISP_SUCCESS );
	}
}
int ValidateReceivingString(TransferResult_t recvRes)
{
	if ( recvRes == TRNS_FAILED )
	{
		printf( "Service socket error while reading, closing thread.\n" );
		return 0;
	}
	else if ( recvRes == TRNS_DISCONNECTED )
	{
		printf( "Connection closed while reading, closing thread.\n" );
		return 0;
	}
	return 1;
}
DWORD HandleClientCommand(char* str, SOCKET *sourceSocket,BOOL* Done,char* clientNameStr)
{
	char* pch;
	char* command;
	char* arg1;
	char* arg2;
	ErrorCode_t errorCode;
	char* systemMessage;
	command = strtok (str," ");

	if (STRINGS_ARE_EQUAL(command,"/active_users"))
	{
		errorCode = SendActiveUsers(sourceSocket, Users, MutexHandle,clientNameStr,&systemMessage);
		if (errorCode == ISP_SUCCESS)
		{
			printf("REQUEST::from %s: %s\n",clientNameStr,systemMessage);
		}
		return errorCode;
	}
	
	else if (STRINGS_ARE_EQUAL(command,"/message"))
	{
		arg1 = strtok (NULL,"\0");
		if (arg1 != NULL)
		{
			errorCode = SendPublicMessage(arg1,clientNameStr,MutexHandle,Users,*sourceSocket);
			if (errorCode == ISP_SUCCESS)
			{
				printf("CONVERSATION:: %s: %s\n",clientNameStr,arg1);
			}
			return errorCode;
		}
		else
		{
			return ISP_NO_SUCCESS;
		}
	}

	else if (STRINGS_ARE_EQUAL(command,"/private_message"))
	{
		arg1 = strtok (NULL," ");
		arg2 = strtok (NULL,"\0");
		if (arg1 != NULL && arg2 != NULL) 
		{
			errorCode = SendPrivateMessage(arg1,clientNameStr,arg2, MutexHandle, Users,sourceSocket);
			if (errorCode == ISP_SUCCESS)
			{
				printf("CONVERSATION:: private message from %s to %s: %s\n",clientNameStr,arg1,arg2);
			}
			return errorCode;
		}
		else
		{
			return ISP_NO_SUCCESS;
		}
	}

	else if (STRINGS_ARE_EQUAL(command,"/broadcast"))
	{
		// send file to everybody
	}

	else if (STRINGS_ARE_EQUAL(command,"/p2p"))
	{
		// open p2p connection between 2 users
	}

	else if (STRINGS_ARE_EQUAL(command,"/quit"))
	{
		*Done = TRUE;
		return ISP_SUCCESS;
	}
	return ISP_NO_SUCCESS;
}

//Service thread is the thread that opens for each successful client connection and "talks" to the client.
DWORD ServiceThread( SOCKET *t_socket ) 
{
	char SendStr[SEND_STR_SIZE];
	char *clientNameStr = NULL;
	AccessResult accessResult = NO_ACCESS;
	BOOL Done = FALSE;
	TransferResult_t sendRes;
	TransferResult_t recvRes;
	recvRes = ReceiveString( &clientNameStr , *t_socket );
	if (HandleAccessRequest(clientNameStr,&accessResult,*t_socket) != ISP_SUCCESS)
	{
		closesocket( *t_socket );
		return 1;
	}
	if (accessResult == NO_ACCESS)
	{
		if (SendString("already taken!", *t_socket) == TRNS_FAILED)
		{
			printf( "Service socket error while writing, closing thread.\n" );
			closesocket( *t_socket );
			return 1;
		}
		closesocket( *t_socket );
		return 0;
	}

	//Session connected
	printf("%s welcome to the session.\n",clientNameStr);
	sendRes = SendString("welcome to the session.", *t_socket );
	if ( sendRes == TRNS_FAILED ) 
	{
		printf( "Service socket error while writing, closing thread.\n" );
		LeaveSessionFlow(clientNameStr,*t_socket,Users,MutexHandle);
		closesocket( *t_socket );
		return ISP_NO_SUCCESS;
	}
	while ( !Done ) 
	{		
		char* sessionStr = NULL;
		recvRes = ReceiveString( &sessionStr , *t_socket );
		if (!ValidateReceivingString(recvRes))
		{
			LeaveSessionFlow(clientNameStr,*t_socket,Users,MutexHandle);
			closesocket( *t_socket );
			return ISP_NO_SUCCESS;
		}
		//printf("SYSTEM:: sent to %s:,Got string : %s\n",clientNameStr,sessionStr);
		
		if (HandleClientCommand( sessionStr, t_socket ,&Done,clientNameStr) != ISP_SUCCESS)
		{
			char* message = ConcatString("No such command: ",sessionStr,"");
			sendRes = SendString(message, *t_socket );
			if ( sendRes == TRNS_FAILED ) 
			{
				printf( "Service socket error while writing, closing thread.\n" );
				LeaveSessionFlow(clientNameStr,*t_socket,Users,MutexHandle);
				closesocket( *t_socket );
				return ISP_NO_SUCCESS;
			}
		}
		free( sessionStr );		
	}
	
	printf("%s,Conversation ended.\n",clientNameStr);
	LeaveSessionFlow(clientNameStr,*t_socket,Users,MutexHandle);
	closesocket( *t_socket );
	return ISP_SUCCESS;
}

void InitParams(int maxClients)
{
	NUM_OF_WORKER_THREADS = maxClients;
	MAX_LOOPS = NUM_OF_WORKER_THREADS +1;

	ThreadHandles =(HANDLE*) malloc(NUM_OF_WORKER_THREADS*sizeof(*ThreadHandles));
	if (ThreadHandles == NULL){
		exit(1);
	}
	ThreadInputs =(SOCKET*) malloc(NUM_OF_WORKER_THREADS*sizeof(*ThreadInputs));
	if (ThreadInputs == NULL){
		exit(1);
	}
	
	Users = CreateList();


}

void CloseSession(SOCKET sourceSocket)
{
	closesocket(sourceSocket);
	closesocket(MainSocket);
	CloseHandle(MutexHandle);
	exit(1);
}
