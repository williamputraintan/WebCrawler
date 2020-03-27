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


int main(int argc, char **argv)
{
	
	int portno	= PORTNO;	
	
	int url_size = strlen(argv[1]); 
	char host[url_size];
	char path[url_size];
	sscanf(argv[1], "http://%[^/]/%[^\n]", host, path);
	
    struct hostent *server;
    struct sockaddr_in serv_addr;
    int sockfd, bytes, sent, received, total, message_size, n;
    char *request_message, response[MAX_SIZE_RESPONSE];
    
    /*error no port provided*/
	if (argc < 2)
	{
		fprintf(stderr,"ERROR, no host provided\n");
		exit(1);
	}
    
/*    
    
  
    struct url_component{
    	char host[url_size];
    	int portno = PORTNO;
    	char path[url_size];
    }
    
    sscanf(argv[1], "http://%[^:]:%99d/%[^c]", url_component.host, &url_component.port, url.path);    
*/

    /*Calculate message size*/
    message_size = 0;
    message_size += url_size;
    message_size += strlen("GET %s HTTP/1.1\r\n");
    message_size += strlen("Host: %s\r\n");
    message_size += strlen("User-Agent: wintan\r\n");
    message_size += strlen("Content-Length: 100000\r\n");
    
    /*Allocating space for message*/
    request_message = malloc(message_size);
    
    
    sprintf(request_message, "GET %s HTTP/1.1\r\n"
    	"Host: %s\r\n"
    	"User-Agent: wintan\r\n"
    	"Content-Length: 100000\r\n",
    	strlen(path)>0?path:"/", host);
    

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

    bzero(response, MAX_SIZE_RESPONSE);

    n = read(sockfd, response, MAX_SIZE_RESPONSE-1);

    if (n < 0)
    {
        perror("ERROR reading from socket");
        exit(0);
    }

    printf("%s\n", response);

    return 0;
}