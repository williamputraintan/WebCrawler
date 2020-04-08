#define PORTNO 				80
#define MAX_SIZE_RESPONSE 	100001	//including null byte
#define TRUE				1
#define FALSE 				0
#define MAX_NUM_URL			100		


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
void http_get_html(char *html_response, char *url, char*additional_header);
void find_url(char *html_response, char *current_url, char *url_list[MAX_NUM_URL],  int *url_count);
int add_new_url(char *new_url, char *url_list[MAX_NUM_URL], int*url_count);
int find_url_type(char *url);
int is_eligible_url(char * old_url, char * new_url);
int add_hyperlink_from_url(char *url_list[MAX_NUM_URL], int *url_count, char* url);
int status_response(char*response);
void moved_site(char*response, char**moved_url);
int check_content_type(char*response);

/*Main Function of the program*/
int main(int argc, char **argv)
{	
//	fprintf(stderr, "TESTING\n");
	//error when no URL provided
	if (argc < 2) {
		fprintf(stderr,"ERROR, no URL provided\n");
		exit(1);
	}

	//intialize
	char *url_list[MAX_NUM_URL];
	int url_count = 0;
	char *curr_url = argv[1];
	add_new_url(curr_url, url_list, &url_count);
//	fprintf(stderr, "CURRENT URL = %s\n", curr_url);
	for(int i = 0; i < url_count; i++){
		curr_url = url_list[i];
		add_hyperlink_from_url(url_list, &url_count, curr_url);
//		printf("url count = %d\n", url_count);

	}
	
	//free memory allocated
	for (int i = 0; i < url_count; i++){
		free(url_list[i]);
	}
	return 0;
}



/*
The function will get HTML code from the URL given and copy to the html_response string.
*/
void http_get_html(char *html_response, char *url, char*additional_header){
	
	struct hostent *server;
	struct sockaddr_in serv_addr;
	int sockfd, bytes, message_size, total, received,sent;
	char *request_message;
	
	//parsing url to host and string
	int init_url_size = strlen(url);
	char *host=calloc(init_url_size, sizeof(char));
	assert(host);
	char *path=calloc(init_url_size, sizeof(char));
	assert(path);
	sscanf(url, "http://%[^/]/%[^\n]", host, path);
	
	/*Calculate message size*/
	message_size = 0;
	message_size += strlen(url);
	message_size += strlen("GET /%s HTTP/1.1\r\n");
	message_size += strlen("Host: %s\r\n");
	message_size += strlen("User-Agent: wintan\r\n");
	message_size += strlen("Content-Type: text/html; charset=UTF-8\r\n");
	message_size += strlen(additional_header);
	message_size += strlen("\r\n");
	
	/*Allocating space for message*/
	request_message = malloc(sizeof(char)*message_size);
	assert(request_message);

	sprintf(request_message, "GET /%s HTTP/1.1\r\n"
		"Host: %s\r\n"
		"User-Agent: wintan\r\n"
		"Content-Type: text/html; charset=UTF-8\r\n"
		"%s"
		"\r\n",
		path, host, additional_header);
	fprintf(stderr, "\n%s\n", request_message);

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
	serv_addr.sin_port = htons(PORTNO);
	
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
	total = MAX_SIZE_RESPONSE-1;
	bzero(html_response, MAX_SIZE_RESPONSE);
	bytes = read(sockfd,html_response,total);
	if (bytes < 0){
		perror("ERROR reading from socket");
		exit(0);
	}
//	fprintf(stderr, "weh1\n");
	/* receive the response */
/*	
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
*/
//	fprintf(stderr, "weh2\n");
	fprintf(stderr, "%s\n", html_response);
	/* close the socket */
	close(sockfd);
	
	free(host);
	free(path);
	free(request_message);
	
}

/*
The function will copy the all available valid link and copy it to the list.
*/
void find_url(char *html_response, char *current_url, char *url_list[MAX_NUM_URL],  int *url_count)
{
	
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
			
			//check where the hyperlink starts
			//( this anticipate if there are spaces between " = " )
			char *href_link_start;
			href_link_start = strchr(href_tag, '"')+1;
			if(href_link_start == NULL){
				is_an_a_tag = 0;
				continue;
			}
			
			//identify size of url	
			char *href_link_end;
			href_link_end = strchr(href_link_start, '"');
			int url_size =href_link_end - href_link_start+1;
			
			//Copying the link to the string
			url = realloc(url, sizeof(char)*url_size);
			assert(url);
			bzero(url, url_size);	
			strncpy(url, href_link_start, (url_size-1));
//			sscanf(href_link_start, "\"%[^\"]\"", url); 
//			fprintf(stderr, "url found = %s\n", url);
			//checking if it acceptable host
			int url_type = find_url_type(url);
			
			if (url_type == 1) {
				// Relative URL (implied protocol)
				
				char *after_protocol = strstr(current_url, "//");
				int current_protcol_size = after_protocol - current_url;
				
				int new_url_size = url_size + current_protcol_size + 1;
				char * temp = malloc(url_size*sizeof(char));
				assert(url);
				strcpy(temp, url);
				
				url = realloc(url, sizeof(char)*new_url_size);
				bzero(url, new_url_size);
				strncpy(url, current_url, current_protcol_size);
				strcat(url, temp);
				
				free(temp);
			} else if (url_type == 2) {
				//Relative URL (implied protocol + host)
				char *after_protocol = strstr(current_url, "//")+2;
				char *after_host = strchr(after_protocol, '/');
				int current_host_size = after_host - current_url;

				int new_url_size = url_size + current_host_size + 1;
				char * temp = malloc(url_size*sizeof(char));
				assert(url);
				strcpy(temp, url);

				url = realloc(url, sizeof(char)*new_url_size);
				bzero(url, new_url_size);
				strncpy(url, current_url, current_host_size);
				strcat(url, temp);
				
				free(temp);
			} else if (url_type == 3) {
				//Relative URL (implied protocol + host)
				char *after_path = strrchr(current_url, '/');
				int current_path_size = after_path - current_url + 1;
				
				int new_url_size = url_size + current_path_size + 1;
				char * temp = malloc(url_size*sizeof(char));
				assert(temp);
				strcpy(temp, url);
				
				url = realloc(url, sizeof(char)*new_url_size);
				assert(url);
				bzero(url, new_url_size);
				strncpy(url, current_url, current_path_size);
				strcat(url, temp);
				
				free(temp);
			}

			if (is_eligible_url(current_url, url) == TRUE){
				fprintf(stderr, "url add = %s\n", url);
				add_new_url(url, url_list, url_count);
			}
			
			if (*url_count == MAX_NUM_URL){
				break;
			}
			is_an_a_tag = 0;

		}
	}
	free(url);
}
/*
Will return the URL type.
returns 0 - Absolute URL
		1 - Relative URL (implied protocol)
		2 - Relative URL (implied protocol + host)
		3 - Relative URL (implied protocol + host + path)
*/
int find_url_type(char *url){
	char *new_host_start;
	
//	printf("URL error = %s\n", url);	
	
	//Checking if Absolute URL
	//Check if the URL starts with " http: "
	new_host_start = strstr(url, "http:");
	if(new_host_start != NULL && (new_host_start - url) == 0 ){
		return 0;
	}
	
	//Checking if Relative (implied protocol)
	//Check if the URL starts with " // "
	new_host_start = strstr(url, "//");
	if(new_host_start != NULL && (new_host_start - url) == 0 ){
		return 1;
	}
	
	//Checking if Relative (implied protocol + host)
	//Check if the URL starts with ' / '
	new_host_start = strchr(url, '/');
	if(new_host_start != NULL && (new_host_start - url) == 0 ){
		return 2;
	}
	
	//Relative URL (implied protocol + host + path)
	return 3; 
	
}

/*
the function will add new_url to the url_list if it does not exist in the list.
Return: 0 - nothing is added
		1 - url is added
*/
int add_new_url(char *new_url, char *url_list[MAX_NUM_URL], int*url_count){
	
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

/*
The function will check if the url will check if the url have the same 
second component host. Will return 1 if its the same and 0 otherwise.
*/
int is_eligible_url(char * old_url, char * new_url){
//	printf("\n\nurl ny = %s\n", new_url);
	char *current_host = calloc(strlen(old_url), sizeof(char));
	assert(current_host);
	sscanf(old_url, "http://%[^/]/", current_host);

	//Will assign the variable below to point after the first " . "
	char *component_cur_host = strchr(current_host, '.') + 1;
	char *component_new_host = strchr(new_url, '.') + 1;
//	printf("lama= %s\nbru = %s\n", component_cur_host, component_new_host);
	int size_host_compare = strlen(component_cur_host);
	
	if (strncmp(component_cur_host, component_new_host, size_host_compare) == 0)
	{
		free(current_host);
		return 1;
	}
	free(current_host);
	return 0;
}	

int add_hyperlink_from_url(char *url_list[MAX_NUM_URL], int *url_count, char* url){
	char *additional_header = calloc(1, sizeof(char));
	char html_response[MAX_SIZE_RESPONSE+1];
	printf("%s\n", url);
	http_get_html(html_response, url, additional_header);
	
	int response_num = status_response(html_response);


	if (response_num == 503 || response_num == 504){
		http_get_html(html_response, url, additional_header);
	} else if ( response_num == 401) {
		char auth[] = "Authorization: Basic d2ludGFuOnBhc3N3b3Jk\r\n";
		http_get_html(html_response, url, auth);
	} else if  ( response_num == 301) {
		moved_site(html_response, &additional_header);
		char *moved_url = strchr(additional_header, ' ')+1;
		http_get_html(html_response, moved_url, "");
	}
	if (check_content_type(html_response) == 0){
		return 0;
	}	
	free(additional_header);
	if(*url_count < MAX_NUM_URL){
		find_url(html_response, url, url_list, url_count);
	}
	return 1;
//	printf("\nselese\n");
}
int status_response(char*response){
	char *end_first_line = strchr(response, '\n');
	int size_first_line = end_first_line - response;
	
	char*status = malloc(sizeof(char)*size_first_line);
	assert(status);
	sscanf(response, "HTTP/1.1%[^\n]\n", status);
	
	char *status_number_char = calloc(4, sizeof(char));
	assert(status_number_char);
	strncpy(status_number_char,strchr(status, ' ')+1, 3);
	int status_number_int = atoi(status_number_char);
	
	free(status_number_char);
	free(status);
	
	return status_number_int;
}

void moved_site(char*response, char**moved_url){
	char *location_str = strstr(response, "Location:");
	char *location_str_end = strchr(location_str, '\r');
	int location_header_size = location_str_end - location_str;
    
    int moved_url_size = location_header_size + 1;
    
	*moved_url = realloc(*moved_url, (sizeof(char)*moved_url_size));
    bzero(*moved_url, moved_url_size);
	strncpy(*moved_url, location_str, location_header_size);

}
int check_content_type(char*response){
	char comparison[] = "Content-Type: text/html";
	int comparisen_len = strlen(comparison);
	char *content_type_str = strstr(response, "Content-Type:");
	if(strncmp(comparison,content_type_str, comparisen_len) == 0){
		return 1;
	}
	return 0;
			
}
