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
 */

#include <stdio.h>
#include <sys/io.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
char s[11] = {"\nabcdefghij"};

#define FILENAME "testprog.conf"
#define MAXBUF 1024
#define DELIM "="
#define PIDFILE "/var/run/testprog.pid"

struct config
{
   char outputfile[MAXBUF];
   char loopcount[MAXBUF];
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
	
 
        /* Cast port as int */
        int x;
        x = atoi(configstruct.loopcount);
        printf("Iteration count: %d\n",x);
	syslog (LOG_NOTICE, "Iteration count: %d\n",x);

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
 
return 0;
}
