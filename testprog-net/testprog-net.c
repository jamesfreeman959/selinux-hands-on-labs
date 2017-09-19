/*
 * A test program for playing with SELinux contexts and permissions
 *
 * Based on a simple premise of a native C binary, that reads one config file and writes an output file
 *
 * Credits for code:
 * https://gsamaras.wordpress.com/code/eat-spaces-and-newline-c/
 * https://www.pacificsimplicity.ca/blog/simple-read-configuration-file-struct-example
 * http://www.cprogramming.com/tutorial/c/lesson14.html
 * https://cboard.cprogramming.com/c-programming/155124-using-c-program-append-text-txt-file.html
 * https://stackoverflow.com/questions/24249369/how-to-write-pid-to-file-on-unix
 * https://www.gnu.org/software/libc/manual/html_node/Syslog-Example.html
 *
 * Thanks to http://www.binarytides.com/server-client-example-c-sockets-linux/ for the network socket example
 */

#include <stdio.h>
#include <sys/io.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include<sys/socket.h>
#include<arpa/inet.h>
char s[11] = {"\nabcdefghij"};

#define FILENAME "testprog-net.conf"
#define MAXBUF 1024
#define DELIM "="
#define PIDFILE "/var/run/testprog-net.pid"

struct config
{
   char outputfile[MAXBUF];
   char loopcount[MAXBUF];
   char networkport[MAXBUF];
};


struct config get_config(char *filename)
{
        struct config configstruct;
        FILE *file = fopen (filename, "r");

        if (file != NULL)
        {
                char line[MAXBUF];
                int i = 0;

                while(fgets(line, sizeof(line), file) != NULL)
                {
                        char *cfline;
                        cfline = strstr((char *)line,DELIM);
                        cfline = cfline + strlen(DELIM);
   
                        if (i == 0){
                                memcpy(configstruct.outputfile,cfline,strlen(cfline));
                                //printf("%s",configstruct.outputfile);
                        } else if (i == 1){
                                memcpy(configstruct.loopcount,cfline,strlen(cfline));
                                //printf("%s",configstruct.loopcount);
                        } else if (i == 2){
				memcpy(configstruct.networkport,cfline,strlen(cfline));
			}
                       
                        i++;
                } // End while
        } // End if file
       
               
        fclose(file);
       
        return configstruct;

}

int main(int argc, char *argv[] )
{
        struct config configstruct;

	setlogmask (LOG_UPTO (LOG_NOTICE));
	openlog ("testprog", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);       

	if (argc < 2) {
		// No arguments were passed
        	configstruct = get_config(FILENAME);
        	printf("Using configuration file: %s\n", FILENAME);
		syslog (LOG_NOTICE, "Using configuration file: %s\n", FILENAME);
		// We assume argv[2] is a pidfile to open
		FILE *file = fopen( PIDFILE, "wb" );
		fprintf(file, "%d\n", getpid());
		fclose(file);
		printf("Wrote PID to %s\n", PIDFILE );
		syslog (LOG_NOTICE, "Wrote PID to %s\n", PIDFILE );
	} else {
	       	// We assume argv[1] is a config file to open
	       	FILE *file = fopen( argv[1], "r" );
	       
	       	/* fopen returns 0, the NULL pointer, on failure */
	       	if ( file == 0 )
	       	{
	       		printf( "Could not open file\n" );
			syslog (LOG_CRIT, "Could not open file\n" );
	       	}
	       	else 
	       	{
			configstruct = get_config(argv[1]);
        		printf("Using configuration file: %s\n", argv[1]);
			syslog (LOG_NOTICE, "Using configuration file: %s\n", argv[1]);
		}
		fclose(file);

		// We assume argv[2] is a pidfile to open
		FILE *pidfile = fopen( argv[2], "wb" );
		fprintf(pidfile, "%d\n", getpid());
		fclose(pidfile);
		printf("Wrote PID to %s\n", argv[2] );
		syslog (LOG_NOTICE, "Wrote PID to %s\n", argv[2] );
	}

	/* Code to trim our string and prevent bad chars in filename */
      	char* outputfilePtr = configstruct.outputfile;
	size_t len;
	outputfilePtr += strspn(outputfilePtr, "\t\n\v\f\r ");
	len = strcspn(configstruct.outputfile, "\r\n");
	configstruct.outputfile[len] = '\0';

	printf("Writing output to: %s\n",configstruct.outputfile);
	syslog (LOG_NOTICE, "Writing output to: %s\n",configstruct.outputfile);
	
 
        /* Cast loopcount as int */
        int x;
        x = atoi(configstruct.loopcount);
        printf("Iteration count: %d\n",x);
	syslog (LOG_NOTICE, "Iteration count: %d\n",x);

	/* Cast networkport at int */
	int networkport;
	networkport = atoi(configstruct.networkport);
	if ( networkport == 0 )
	{
		printf("Network functionality disabled\n");
		syslog (LOG_NOTICE, "Network functionality disabled\n");

		int i = 0;
		while(i < x || x < 0)
		{
			FILE *file = fopen(configstruct.outputfile, "a");
			fputs("\nHello World",file);
			fputs(s,file);
 	
			fclose(file);
			sleep(1);
	
			i++;
		}
	} else if ( networkport > 0 ) {
                printf("Network functionality enabled on port %d\n",networkport);
                syslog (LOG_NOTICE, "Network functionality enabled on port %d\n",networkport);

		int socket_desc , client_sock , c , read_size;
		struct sockaddr_in server , client;
		char client_message[2000];
     
		//Create socket
		socket_desc = socket(AF_INET , SOCK_STREAM , 0);
		if (socket_desc == -1)
		{
			printf("Could not create socket\n");
			syslog (LOG_CRIT, "Could not create socket\n");
		}
		printf("Socket created\n");
		syslog (LOG_NOTICE, "Socket created\n");
     
		//Prepare the sockaddr_in structure
		server.sin_family = AF_INET;
		server.sin_addr.s_addr = INADDR_ANY;
		server.sin_port = htons( networkport );
     
		//Bind
		if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
		{
			//print the error message
			printf("bind failed. Error\n");
			syslog (LOG_CRIT, "bind failed. Error\n");
			return 1;
		}
		printf("bind done\n");
		syslog (LOG_NOTICE, "bind done\n");
     
		//Listen
		listen(socket_desc , 3);
    		while (1) { 
		//Accept and incoming connection
		printf("Waiting for incoming connections...\n");
		syslog (LOG_NOTICE, "Waiting for incoming connections...\n");
		c = sizeof(struct sockaddr_in);
     
		//accept connection from an incoming client
		client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
		if (client_sock < 0)
		{
			printf("accept failed\n");
			syslog (LOG_CRIT, "accept failed\n");
			return 1;
		}
		printf("Connection accepted\n");
		syslog (LOG_NOTICE, "Connection accepted\n");
     
		//Receive a message from client
		while( (read_size = recv(client_sock , client_message , 2000 , 0)) > 0 )
		{
			//Send the message back to client
			write(client_sock , client_message , strlen(client_message));
			//Write the message to our data file also
			FILE *file = fopen(configstruct.outputfile, "a");
                        fputs(client_message,file);

                        fclose(file);
		}
     
		if(read_size == 0)
		{
			printf("Client disconnected\n");
			syslog(LOG_NOTICE, "Client disconnected\n");
			fflush(stdout);
		}
		else if(read_size == -1)
		{
			printf("recv failed\n");
			syslog(LOG_CRIT, "recv failed\n");
		}
		}

	}
 
return 0;
}
