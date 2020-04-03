#define PORTNO 				80
#define MAX_SIZE_RESPONSE 	100001	//including null byte
#define FALSE 				0
#define MAX_NUM_FETCHES 	100
#define NOT_ACCEPTABLE 		0
#define RELATIVE_URL		2

/*Include files*/
#define _GNU_SOURCE
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
#include <ctype.h>

/*function prototype*/
void http_get_html(char *html_response, char *host, char *path);
void find_url(char *html_response, char*second_current_host_component, char **url_list, int *url_count);
int add_new_url(char *new_url, char *url_list[100], int*url_count);
int is_acceptable_url(char *second_host_component, char *url);

/*Main Function of the program*/
int main(int argc, char **argv)
{	
	//error when no URL provided
	if (argc < 2) {
		fprintf(stderr,"ERROR, no URL provided\n");
		exit(1);
	}

	//intialize
	char *url_list[MAX_NUM_FETCHES];
	int url_count = 0;
	
	char html_response[MAX_SIZE_RESPONSE+1];
	int init_url_size = strlen(argv[1]);
	
	
	//copying host and path to a string
	char *current_host=malloc(sizeof(char)*init_url_size);
	char *current_path=malloc(sizeof(char)*init_url_size);
	sscanf(argv[1], "http://%[^/]/%[^\n]", current_host, current_path);
	

	//getting a html response from host and path
	http_get_html(html_response, current_host, current_path);
	

	
//	printf("html nya adalah:\n");
//	printf("%s", html_response);


	find_url(html_response, current_host, url_list, &url_count);

	
	printf("url prtama = %d", url_count);

	free(current_host);
	free(current_path);
	
	printf("\n\n\nKONKLUSI\n\n\n");
	
	//free memory allocated
	for (int i = 0; i < url_count; i++){
		printf("%s\n", url_list[i]);
		free(url_list[i]);
	}
	
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
The function will copy the all available valid link and copy it to the list.

*/
void find_url(char *html_response, char*current_host,char *url_list[100],  int *url_count)
{
	int host_size = strlen(current_host);
//	char first_current_host_component = malloc(sizeof(char)*host_size);
//	char second_current_host_component = malloc(sizeof(char)*host_size);
	
	char *second_current_host_component = strchr(current_host, '.')+1;
	
	//finding the length of the first component excluding " . "
	int length_first_comp = second_current_host_component - current_host -1;

	printf("2 = %d\n", length_first_comp);
	int start_tag, end_tag;
	int is_an_a_tag = 0;
	char *html_tag;
	char *url = malloc(sizeof(char));
	for(int i = 0; i < strlen(html_response)-1; i++){
		//skip index if its not a opening/closing tag
		if (html_response[i] != '<' && html_response[i] != '>'){
			continue;
		}
		
		//checking if its hyperlink tag
		if (html_response[i] == '<' && tolower(html_response[i+1]) == 'a'){
			start_tag = i;
			is_an_a_tag = 1;
		}
		
		//checking if it is an a tag
		if (is_an_a_tag == 1){
            
			//indicate where the <a> tag is 
			html_tag = &html_response[start_tag];
			
			//check where the href tag begins
			char *href_tag;
			href_tag = strcasestr(html_tag, "href");
			if(href_tag == NULL){
				is_an_a_tag = 0;
				continue;
			}
			
            //check where the link starts
            //this anticipate if there are spaces between " = "
			char *href_link_start;
			href_link_start = strchr(href_tag, '"');
			if(href_link_start == NULL){
				is_an_a_tag = 0;
				continue;
			}
			
			//identify size of url
			char *href_link_end;
			href_link_end = strchr(href_link_start+1, '"');
            int url_size =href_link_end - href_link_start;
            
			//Copying the link to the string
			url = realloc(url, sizeof(char)*url_size);
			assert(url);
			sscanf(href_link_start, "\"%[^\"]\"", url); 
			
//			printf("2nd host = %s\n", second_current_host_component);
//			printf("url = %s\n", url);
			
			//checking if it acceptable host
			int url_type = is_acceptable_url(second_current_host_component, url);
			
//			printf("test_type = %d\n\n", url_type);
			if ( url_type == NOT_ACCEPTABLE){
				is_an_a_tag = 0;
				continue;
			} else if ( url_type == RELATIVE_URL){
				char *temp = url;
				int absolute_url_size =0;
				absolute_url_size += host_size;
				absolute_url_size += url_size;
				url = realloc(url, sizeof(char) * absolute_url_size);
				assert(url);
				strcpy(url, current_host);
				strcat(url, temp);
				
			}
//			NEED SOME EDIT HERE
		
			add_new_url(url, url_list, url_count);

			is_an_a_tag = 0;

		}
	}
	free(url);
}
/*
Will check if the new url have the same 2nd component host as the original.
returns 0 - Not acceptable
		1 - match with the original host
		2 - relative URL
*/
int is_acceptable_url(char *second_host_component, char *url){
    char *new_host_start, *new_host_end;
    int size_new_host;
    new_host_start = strchr(url, '.'); //inluding dots
    
    /*return if it is a relative URL*/
    if (new_host_start == NULL){
        return 2;
    }
    new_host_start += 1;			//move pointer forward to remove " . "
    
    /*check if url dont have any path*/
    new_host_end = strchr(new_host_start + 1, '/');
    if (new_host_end == NULL){
    	size_new_host = strlen(new_host_start);
    } else {
    	size_new_host = new_host_end - new_host_start;
    }

    char *new_url_host = malloc(sizeof(char) * (size_new_host+1));
    
    strncpy(new_url_host, new_host_start, size_new_host);
    
    if (strcmp(second_host_component, new_url_host) == 0){
        free(new_url_host);
        return 1;
    }
    
    free(new_url_host);
    return 0;
}	
/*
the function will add new_url to the url_list if it does not exist in the list.
Return: 0 - nothing is added
		1 - url is added
*/
int add_new_url(char *new_url, char *url_list[100], int*url_count){
	
	//check if the url are already inside the list
	for (int i = 0; i < *url_count; i++){
		if( strcmp(url_list[i], new_url) == 0){
			return 0;
		}
	}
	
	//allocating new memory and add the url to the list
	int size_url = strlen(new_url)+1;
	url_list[*url_count] = malloc(sizeof(char)*size_url);
	strcpy(url_list[*url_count], new_url);
	*url_count +=1;
	return 1;
}
