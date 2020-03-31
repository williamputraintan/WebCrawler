#define PORTNO 80
#define MAX_SIZE_RESPONSE 100000
#define MAX_SIZE_URL 1000
#define METHOD "GET"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h> 

/*function prototype*/
void http_get_html(char* url, char *html_response);

int main(int argc, char **argv)
{
	
	/*error no port provided*/
	if (argc < 2)
	{
		fprintf(stderr,"ERROR, no host provided\n");
		exit(1);
	}
	
	char html_response[MAX_SIZE_RESPONSE];
	
	http_get_html(argv[1], html_response);

	printf("html nya adalah:\n");
	printf("%s", html_response);
	
    return 0;
}



void http_get_html(char* url, char *html_response){

	int portno	= PORTNO;	

	int url_size = strlen(url); 
	char host[url_size];
	char path[url_size];
	sscanf(url, "http://%[^/]/%[^\n]", host, path);
	
	struct hostent *server;
    struct sockaddr_in serv_addr;
    int sockfd, bytes, message_size, n;
    char *request_message;
    


    /*Calculate message size*/
    message_size = 0;
    message_size += url_size;
    message_size += strlen("GET %s HTTP/1.1\r\n");
    message_size += strlen("Host: %s\r\n");
    message_size += strlen("User-Agent: wintan\r\n");
    message_size += strlen("Content-Length: 100000\r\n");
    message_size += strlen("Content-Type: text/html;");
    
    /*Allocating space for message*/
    request_message = malloc(message_size);
    
    sprintf(request_message, "GET %s HTTP/1.1\r\n"
    	"Host: %s\r\n"
    	"User-Agent: wintan\r\n"
    	"Content-Length: 100000\r\n"
    	"Content-Type: text/html;",
    	strlen(path)>0?path:"/", host);
    printf("\n%s\n", request_message);

    /* Translate host name into peer's IP address ;
     * This is name translation service by the operating system
     */
    server = gethostbyname(host);

    if (server == NULL)
    {
        fprintf(stderr, "ERROR, no such host\n");
        exit(0);
    }

    /* Building data structures for socket */

    bzero((char *)&serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;

    bcopy(server->h_addr_list[0], (char *)&serv_addr.sin_addr.s_addr, server->h_length);

    serv_addr.sin_port = htons(portno);
    
    /* Create TCP socket -- active open
    * Preliminary steps: Setup: creation of active open socket
    */

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0)
    {
        perror("ERROR opening socket");
        exit(0);
    }
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("ERROR connecting");
        exit(0);
    }


    /* Do processing
    */

    n = write(sockfd, request_message, message_size);
    if (n < 0)
    {
        perror("ERROR writing to socket");
        exit(0);
    }

    bzero(html_response, MAX_SIZE_RESPONSE);
    n = read(sockfd, html_response, MAX_SIZE_RESPONSE-1);

    if (n < 0)
    {
        perror("ERROR reading from socket");
        exit(0);
    }
	
    free(request_message);
	
}

