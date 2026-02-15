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
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <signal.h>
char s[11] = {"\nabcdefghij"};
volatile sig_atomic_t keep_running = 1;
/* volatile ensures compiler reads it every time instead of caching the value for mem optimization
sig_atomic_t is a special integer type in signal.h. Guarantess read/write atomically (the CPU can
update it in single instruction )
*/
void handle_signal(int sig)
{
	(void)sig; // For unused variable warning, avoiding printing of signal for code safety reasons
	keep_running = 0;
}

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
	memset(&configstruct, 0, sizeof(configstruct));
	/* Local variables are not initialized automatically, if function fails to open or
	the config has fewer than 2 lines, configstruct.outputfile or configstruct.loopcount 
	would contain whatever garbage was on the stack. memset here fills the entire struct
	with zeros, so even on failure, the function call gets an empty string instead of 
	unpredictable garbage memory.*/
	FILE *file = fopen (filename, "r");

	if (file == NULL)
	{
		printf("Could not open config file: %s\n", filename);
		syslog(LOG_CRIT, "Could not open config file: %s\n", filename);
		return configstruct;
	} // Converted to a guarding clause for better readability(flattens the nesting) and early return

	char line[MAXBUF];
	int i = 0;

	while(fgets(line, sizeof(line), file) != NULL)
	{
		char *cfline;
		cfline = strstr((char *)line,DELIM);
		if (cfline == NULL)
		{
			i++;
			continue;
		}
		/*Guarding clause for strstr returning null in case DELIM, '=' is not in config file.
		Earlier we were simply adding 1 to NULL, potentially leading to instant crash. 
		Now we would simply skip the lines that don't have '=' delimiter. 
		*/
		cfline = cfline + strlen(DELIM);

		size_t copy_len = strlen(cfline); //Used twice later, better to call only once.
		if (copy_len >= MAXBUF)
				copy_len = MAXBUF - 1;  // Preventing buffer overflow in case cfline is > 1024

		if (i == 0){
				memcpy(configstruct.outputfile,cfline,copy_len);
				configstruct.outputfile[copy_len] = '\0'; //Null-terminate the string
				//printf("%s",configstruct.outputfile);
		} else if (i == 1){
				memcpy(configstruct.loopcount,cfline,copy_len);
				configstruct.loopcount[copy_len] = '\0'; //Null-terminate the string
				//printf("%s",configstruct.loopcount);
		}

		i++;
	} // End while

	fclose(file);
	return configstruct;
}

int main(int argc, char *argv[] )
{
	struct config configstruct;

	setlogmask (LOG_UPTO (LOG_NOTICE));
	openlog ("testprog", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
	signal(SIGINT, handle_signal);
	signal(SIGTERM, handle_signal);

	char *config_path = FILENAME;
	char *pid_path  = PIDFILE;

	if (argc >= 2) {
		config_path = argv[1];
	}
	if (argc >= 3) {
		pid_path = argv[2];
	}

	FILE *config_file = fopen( config_path, "r" );

	if ( config_file == NULL )
	{
		printf( "Could not open file: %s\n", config_path );
		syslog (LOG_CRIT, "Could not open file: %s\n", config_path );
		closelog();
		return 1;
	}
	fclose(config_file);

	configstruct = get_config(config_path);
	printf("Using configuration file: %s\n", config_path);
	syslog (LOG_NOTICE, "Using configuration file: %s\n", config_path);

	FILE *pidfile = fopen( pid_path, "wb" );
	if ( pidfile == NULL )
	{
		printf( "Could not open PID file: %s\n", pid_path );
		syslog (LOG_CRIT, "Could not open file: %s\n", pid_path );
		closelog();
		return 1;
	}
	fprintf(pidfile, "%d\n", getpid());
	fclose(pidfile);
	printf("Wrote PID to %s\n", pid_path);
	syslog (LOG_NOTICE, "Wrote PID to %s\n", pid_path );


	/* Trim trailing newline/carriage return from output filename */
	size_t len;
	//Removed unused outputfilePtr and updated the comment above. Since it's never used anywhere.
	len = strcspn(configstruct.outputfile, "\r\n");
	configstruct.outputfile[len] = '\0';

	printf("Writing output to: %s\n",configstruct.outputfile);
	syslog (LOG_NOTICE, "Writing output to: %s\n",configstruct.outputfile);

	/* Parse loop count as int */
	int x;
	x = atoi(configstruct.loopcount);
	printf("Iteration count: %d\n",x);
	syslog (LOG_NOTICE, "Iteration count: %d\n",x);

	int i = 0;
	while(keep_running && (i < x || x < 0))
	{
		FILE *file = fopen(configstruct.outputfile, "a");
		if (file != NULL) {
			fputs("\nHello World",file);
			fputs(s,file);
			fclose(file);
		}
		sleep(1);

		i++;
	}

	closelog();
	return 0;
}
