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

#include "utils.h"
#include "SocketExampleShared.h"
#include "SocketSendRecvTools.h"
#include "client.h"

/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/

SOCKET m_socket;

/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/

DWORD CheckSessionAccess(char* clientName)
{
	TransferResult_t sendRes;
	TransferResult_t recvRes;
	char *acceptedStr = NULL;
	
	sendRes = SendString( clientName, m_socket);
	if ( sendRes == TRNS_FAILED ) 
	{
		printf("Socket error while trying to write data to socket\n");
		return 0x555;
	}

	recvRes = ReceiveString( &acceptedStr , m_socket );
	if ( recvRes == TRNS_FAILED )
	{
		printf("Socket error while trying to write data to socket\n");
		return 0x555;
	}
	else if ( recvRes == TRNS_DISCONNECTED )
	{
		printf("Server closed connection. Bye!\n");
		return 0x555;
	}

	if (STRINGS_ARE_EQUAL(acceptedStr,"already taken!"))
	{
		printf("%s %s\n",clientName,acceptedStr);
		return NO_ACCESS;
	}
	if (STRINGS_ARE_EQUAL(acceptedStr,"No available socket at the moment. Try again later."))
	{
		printf("%s\n",acceptedStr);
		return NO_ACCESS;
	}
	if (STRINGS_ARE_EQUAL(acceptedStr,"welcome to the session."))
	{
		printf("Hello <%s>, welcome to the session.\n",clientName);
		return ACCESS;
	}
	return NO_ACCESS;
}

//Reading data coming from the server
static DWORD RecvDataThread(void)
{
	TransferResult_t RecvRes;

	while (1) 
	{
		char *acceptedStr = NULL;
		RecvRes = ReceiveString( &acceptedStr , m_socket );

		if ( RecvRes == TRNS_FAILED )
		{
			printf("Socket error while trying to write data to socket\n");
			return 0x555;
		}
		else if ( RecvRes == TRNS_DISCONNECTED )
		{
			printf("Server closed connection. Bye!\n");
			return 0x555;
		}
		else
		{
			printf("%s\n",acceptedStr);
		}
		
		free(acceptedStr);
	}

	return 0;
}

/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/

//Sending data to the server
static DWORD SendDataThread(void)
{
	char SendStr[256];
	TransferResult_t SendRes;
	
	while (1) 
	{
		gets(SendStr); //Reading a string from the keyboard
		
		SendRes = SendString( SendStr, m_socket);
	
		if ( SendRes == TRNS_FAILED ) 
		{
			printf("Socket error while trying to write data to socket\n");
			return 0x555;
		}

		if ( STRINGS_ARE_EQUAL(SendStr,"quit") ) 
			return 0x555; //"quit" signals an exit from the client side
	}
}

/*oOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoOoO*/

void MainClient(char* serverIp,char* clientName,int serverPort)
{
	SOCKADDR_IN clientService;
	HANDLE hThread[2];
	char* accessResult;

    // Initialize Winsock.
    WSADATA wsaData; //Create a WSADATA object called wsaData.
	//The WSADATA structure contains information about the Windows Sockets implementation.
	
	//Call WSAStartup and check for errors.
    int iResult = WSAStartup( MAKEWORD(2, 2), &wsaData );
    if ( iResult != NO_ERROR )
        printf("Error at WSAStartup()\n");

	//Call the socket function and return its value to the m_socket variable. 
	// For this application, use the Internet address family, streaming sockets, and the TCP/IP protocol.
	
	// Create a socket.
    m_socket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );

	// Check for errors to ensure that the socket is a valid socket.
    if ( m_socket == INVALID_SOCKET ) {
        printf( "Error at socket(): %ld\n", WSAGetLastError() );
        WSACleanup();
        return;
    }
	/*
	 The parameters passed to the socket function can be changed for different implementations. 
	 Error detection is a key part of successful networking code. 
	 If the socket call fails, it returns INVALID_SOCKET. 
	 The if statement in the previous code is used to catch any errors that may have occurred while creating 
	 the socket. WSAGetLastError returns an error number associated with the last error that occurred.
	 */


	//For a client to communicate on a network, it must connect to a server.
    // Connect to a server.

    //Create a sockaddr_in object clientService and set  values.
    clientService.sin_family = AF_INET;
	clientService.sin_addr.s_addr = inet_addr( serverIp ); //Setting the IP address to connect to
    clientService.sin_port = htons( serverPort ); //Setting the port to connect to.
	
	/*
		AF_INET is the Internet address family. 
	*/


    // Call the connect function, passing the created socket and the sockaddr_in structure as parameters. 
	// Check for general errors.
	if ( connect( m_socket, (SOCKADDR*) &clientService, sizeof(clientService) ) == SOCKET_ERROR) {
        printf( "Failed to connect.\n" );
        WSACleanup();
        return;
    }

	// after connect established, check accsess to the session in front of the server
	if (CheckSessionAccess(clientName) == NO_ACCESS)
	{
		closesocket(m_socket);
		WSACleanup();
		exit(1);
	}

    // Send and receive data.
	/*
		In this code, two integers are used to keep track of the number of bytes that are sent and received. 
		The send and recv functions both return an integer value of the number of bytes sent or received, 
		respectively, or an error. Each function also takes the same parameters: 
		the active socket, a char buffer, the number of bytes to send or receive, and any flags to use.

	*/	

	hThread[0]=CreateThread(
		NULL,
		0,
		(LPTHREAD_START_ROUTINE) SendDataThread,
		NULL,
		0,
		NULL
	);
	hThread[1]=CreateThread(
		NULL,
		0,
		(LPTHREAD_START_ROUTINE) RecvDataThread,
		NULL,
		0,
		NULL
	);

	WaitForMultipleObjects(2,hThread,FALSE,INFINITE);

	TerminateThread(hThread[0],0x555);
	TerminateThread(hThread[1],0x555);

	CloseHandle(hThread[0]);
	CloseHandle(hThread[1]);
	

	closesocket(m_socket);
	WSACleanup();
    
	return;
}

