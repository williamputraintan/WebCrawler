#define PORTNO 80
#define MAX_SIZE_RESPONSE 100001
#define MAX_SIZE_URL 1000
#define METHOD "GET"
#define FALSE 0
#define MAX_NUM_FETCHES 100

/*Include files*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <assert.h>

/*function prototype*/
void http_get_html(char *html_response, char *host, char *path);
void slice_str(const char * str, char * buffer, size_t start, size_t end);
int find_url(char *html_response, char*url);

/*Main Function of the program*/
int main(int argc, char **argv)
{	
	//error when no URL provided
	if (argc < 2) {
		fprintf(stderr,"ERROR, no URL provided\n");
		exit(1);
	}
	
	//intialize
	char html_response[MAX_SIZE_RESPONSE+1];
	int init_url_size = strlen(argv[1]);
	//copying host and path to a string
	char *host=malloc(sizeof(char)*init_url_size);
	char *path=malloc(sizeof(char)*init_url_size);
	sscanf(argv[1], "http://%[^/]/%[^\n]", host, path);
	
	
	//getting a html response from host and path
	http_get_html(html_response, host, path);
	
	printf("html nya adalah:\n");
	printf("%s", html_response);
	/*
	char *url_copy = malloc(sizeof(char));
	
	if( find_url(html_response, url_copy) == 0){
		printf("no links found\n");
		return 0;
	}
	free(url_copy);	
	
	*/
	free(host);
	free(path);
	
    return 0;
}



/*
The function will get HTML code from the URL given and copy to the html_response string.
*/
void http_get_html(char *html_response, char *host, char *path){

	int portno	= PORTNO;		
	struct hostent *server;
    struct sockaddr_in serv_addr;
    int sockfd, bytes, message_size, total, received,sent;
    char *request_message;
    
    /*Calculate message size*/
    message_size = 0;
    message_size += strlen(host);
    message_size += strlen(path);
    message_size += strlen("GET /%s HTTP/1.1\r\n");
    message_size += strlen("Host: %s\r\n");
    message_size += strlen("User-Agent: wintan\r\n");
    message_size += strlen("Content-Type: text/html; charset=UTF-8\r\n\r\n");
    
    /*Allocating space for message*/
    request_message = malloc(sizeof(char)*message_size);
    
    sprintf(request_message, "GET /%s HTTP/1.1\r\n"
    	"Host: %s\r\n"
    	"User-Agent: wintan\r\n"
    	"Content-Type: text/html; charset=UTF-8\r\n\r\n",
    	path, host);
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


    /* Do processing */

    /* send the request */
    total = strlen(request_message);
    sent = 0;
    do {
        bytes = write(sockfd, request_message+sent, total-sent);
        if (bytes < 0){
			perror("ERROR writing to socket");
			exit(0);
		}
        if (bytes == 0){
            break;
        }
        sent+=bytes;
    } while (sent < total);

    bzero(html_response, MAX_SIZE_RESPONSE);
    
    /* receive the response */
    total = MAX_SIZE_RESPONSE-1;
    received = 0;
    do { 	
        bytes = read(sockfd,html_response+received,total-received);
        if (bytes < 0){
			perror("ERROR reading from socket");
			exit(0);
		}
        if (bytes == 0){
            break;
        }
        received+=bytes;
    } while (received < total);
    
    if (received == total){
        perror("ERROR storing complete response from socket");
    	exit(0);	
    }

    /* close the socket */
    close(sockfd);
    
    free(request_message);
	
}

/*
The function will copy the first link found in the HTML to a string called url.
Will return FALSE otherwise.
*/
int find_url(char *html_response, char*url)
{
	int start_tag, end_tag;
	char *html_tag;

	for(int i = 0; i < strlen(html_response)-1; i++){
		//skip index if its not a opening/closing tag
		if (html_response[i] != '<' && html_response[i] != '>'){
			continue;
		}
	
		//checking if its hyperlink tag
		if (html_response[i] == '<' && html_response[i+1] == 'a'){
			start_tag = i;
		}
		
		//checking for closing tag
		if (html_response[i] == '>'){
			end_tag = i;
            
			//copy the tag that contain a link
            int tag_size = (end_tag-start_tag);
            printf("\nstart = %d\n", start_tag);
            printf("\nend = %d\n", end_tag);
            printf("\n%d\n", tag_size);
			html_tag = malloc(sizeof(char)*tag_size);
            assert(html_tag);
			slice_str(html_response, html_tag, start_tag, end_tag);
			
			//check where the href tag begins
			char *href_tag;
			href_tag = strstr(html_tag, "href");
			if(href_tag == NULL){
				continue;
			}
            //check where the link starts
            //this anticipate if there are spaces between " = "
			char *href_link;
			href_link = strchr(href_tag, '"');
			if(href_link == NULL){
				continue;
			}
            
			//Copying the link to the string
			int url_size = strlen(href_link);
			url = realloc(url, sizeof(char)*url_size);
            assert(url);
			sscanf(href_link, "\"%[^\"]\"", url); 

            free(html_tag);
			return 1;
			
		}
	}	
    return FALSE;
}




/* 
This function will slice a string and copy to its buffer.
The function is taken from :
https://stackoverflow.com/questions/26620388/c-substrings-c-string-slicing
*/
void slice_str(const char * str, char * buffer, size_t start, size_t end)
{
    size_t j = 0;
    for ( size_t i = start; i <= end; ++i ) {
        buffer[j++] = str[i];
    }
    buffer[j] = 0;
};