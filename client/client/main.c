#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include"client.h"

int main(int argc,char** argv)
{
	char* serverIp;
	int serverPort;
	char* clientName;
	
	if (argc < 4)
	{
		printf("ERROR - Not enough arguments\n");
		return 1;
	}
	serverPort = atoi(argv[2]);
	serverIp = (char*) malloc(strlen(argv[1])*sizeof(char));
	if (serverIp == NULL)
	{
		printf("ERROR - Malloc failed \n");
		return 1;
	}
	strcpy(serverIp,argv[1]);//,strlen(argv[1]));
	clientName = (char*) malloc(strlen(argv[3])*sizeof(char));
	if (clientName == NULL)
	{
		printf("ERROR - Malloc failed \n");
		return 1;
	}
	strcpy(clientName,argv[3]);//,strlen(argv[3]));
	if (strlen(clientName) > MAX_USER_NAME_LENGTH)
	{
		printf("ERROR - user name length is exceeds the limit\n");
		return 1;
	}
	printf("<%s><%s><%d>\n",serverIp,clientName,serverPort);
	MainClient(serverIp,clientName,serverPort);
	while(1);
	return 0;
}