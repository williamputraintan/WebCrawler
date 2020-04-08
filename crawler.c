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
void find_url(char *html_response, char *current_url, char *url_list[MAX_NUM_URL], int *url_count);
int add_new_url(char *new_url, char *url_list[MAX_NUM_URL], int*url_count);
int find_url_type(char *url);
int is_eligible_url(char * old_url, char * new_url);
void add_hyperlink_from_url(char *url_list[MAX_NUM_URL], int *url_count, char* url);
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

	//Will go through the every url on the url_list array
	for(int i = 0; i < url_count; i++){
		curr_url = url_list[i];
		add_hyperlink_from_url(url_list, &url_count, curr_url);
	}
	
	//free memory allocated on the pointers array
	for (int i = 0; i < url_count; i++){
		free(url_list[i]);
	}
	return 0;
}
/*
The function will add url found in the html response and add the url to the 
url array list pass through the function. 
The function will also handle 503, 504, 401, 301 status code response.
*/
void add_hyperlink_from_url(char *url_list[MAX_NUM_URL], int *url_count, char* url){
	//Getting the html response from the current url 
	char *additional_header = calloc(1, sizeof(char));
	char html_response[MAX_SIZE_RESPONSE+1];
	printf("%s\n", url);
	http_get_html(html_response, url, additional_header);
	
	//It will identify the response code and take the appropriate responses
	int response_num = status_response(html_response);
	
	//trying to refetch when receiving 503 ann 504 response
	if (response_num == 503 || response_num == 504){
		http_get_html(html_response, url, additional_header);
		
	//refetch the url with authentication
	} else if ( response_num == 401) {
		char auth[] = "Authorization: Basic d2ludGFuOnBhc3N3b3Jk\r\n";
		http_get_html(html_response, url, auth);
		
	//refetch the url with the new url
	} else if  ( response_num == 301) {
		moved_site(html_response, &additional_header);
		char *moved_url = strchr(additional_header, ' ')+1;
		http_get_html(html_response, moved_url, "");
	}
	
	//Will check if the response in in text/html format and will terminate the 
	//function if it is the wrong format
	if (check_content_type(html_response) == 0){
		return;
	}	
	free(additional_header);
	
	//Will search url's from the HTML response and add the URL to the array list
	//if the url found have not reach the MAX_NUM_URL
	if(*url_count < MAX_NUM_URL){
		find_url(html_response, url, url_list, url_count);
	}
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


	/* receive the response */
	
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
	
	free(host);
	free(path);
	free(request_message);
	
}

/*
The function will copy the all available valid hyperlink and copy it to the list
of array.
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

			//checking if it acceptable host
			int url_type = find_url_type(url);
			
			// Relative URL (implied protocol)
			if (url_type == 1) {
				
				//indicating where the hostname starts
				char *after_protocol = strstr(current_url, "//");
				int current_protcol_size = after_protocol - current_url;
				
				//resize the url according to its size
				int new_url_size = url_size + current_protcol_size + 1;
				char * temp = malloc(url_size*sizeof(char));
				assert(url);
				strcpy(temp, url);
				
				//Replacing the relative url to absolute url
				url = realloc(url, sizeof(char)*new_url_size);
				bzero(url, new_url_size);
				strncpy(url, current_url, current_protcol_size);
				strcat(url, temp);
				
				free(temp);
				
			//Relative URL (implied protocol + host)
			} else if (url_type == 2) {
				
				//indicate where the path starts
				char *after_protocol = strstr(current_url, "//")+2;
				char *after_host = strchr(after_protocol, '/');
				int current_host_size = after_host - current_url;
				
				//resizing the url char size
				int new_url_size = url_size + current_host_size + 1;
				char * temp = malloc(url_size*sizeof(char));
				assert(url);
				strcpy(temp, url);

				//Replacing the relative url to absolute url
				url = realloc(url, sizeof(char)*new_url_size);
				bzero(url, new_url_size);
				strncpy(url, current_url, current_host_size);
				strcat(url, temp);
				
				free(temp);
				
			// Relative URL (implied protocol + host + path)
			} else if (url_type == 3) {
				//indicate where the file path starts
				char *after_path = strrchr(current_url, '/');
				int current_path_size = after_path - current_url + 1;
				
				//resize the url char size
				int new_url_size = url_size + current_path_size + 1;
				char * temp = malloc(url_size*sizeof(char));
				assert(temp);
				strcpy(temp, url);
				
				//Replacing the relative url to absolute url
				url = realloc(url, sizeof(char)*new_url_size);
				assert(url);
				bzero(url, new_url_size);
				strncpy(url, current_url, current_path_size);
				strcat(url, temp);
				
				free(temp);
			}

			//Will check if the url found have the same second component hostname
			//if true will add the url to the url array
			if (is_eligible_url(current_url, url) == TRUE){
				add_new_url(url, url_list, url_count);
			}
			
			//if url found have reach MAX_NUM_URL it will break the loop
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
second component host. 
Return: 1 - True
		0 - False.
*/
int is_eligible_url(char * old_url, char * new_url){
	
	//will parse the hostname of the new url
	char *current_host = calloc(strlen(old_url), sizeof(char));
	assert(current_host);
	sscanf(old_url, "http://%[^/]/", current_host);

	//Will assign pointer variable after the first " . " at the hostname	
	char *component_cur_host = strchr(current_host, '.') + 1;
	char *component_new_host = strchr(new_url, '.') + 1;

	int size_host_compare = strlen(component_cur_host);
	
	//Will compare both url's at second component of the hostname if are the same
	if (strncmp(component_cur_host, component_new_host, size_host_compare) == 0)
	{
		free(current_host);
		return 1;
	}
	
	free(current_host);
	return 0;
}	

/*
The function will find the status response code. It return the status repsonse 
code number as an integer.
*/
int status_response(char*response){
	//taking the first line of the response
	char *end_first_line = strchr(response, '\n');
	int size_first_line = end_first_line - response;
	
	//copying the status a char
	char*status = malloc(sizeof(char)*size_first_line);
	assert(status);
	sscanf(response, "HTTP/1.1%[^\n]\n", status);
	
	//copying the response number to a string
	char *status_number_char = calloc(4, sizeof(char));
	assert(status_number_char);
	strncpy(status_number_char,strchr(status, ' ')+1, 3);
	
	//converting the string into integer value
	int status_number_int = atoi(status_number_char);
	
	free(status_number_char);
	free(status);
	
	return status_number_int;
}
/*
The function will deal with 301 where the url are moved to another site.
The function will locate the Location of the new site and copy to the moved_url char
*/
void moved_site(char*response, char**moved_url){
	char *location_str = strstr(response, "Location:");
	char *location_str_end = strchr(location_str, '\r');
	int location_header_size = location_str_end - location_str;
    
    int moved_url_size = location_header_size + 1;
    
	*moved_url = realloc(*moved_url, (sizeof(char)*moved_url_size));
    bzero(*moved_url, moved_url_size);
	strncpy(*moved_url, location_str, location_header_size);

}

/*
The function will check if the the response have "Content-Type: text/html"
Return: 1 - true 
		0 - false
*/
int check_content_type(char*response){
	char comparison[] = "Content-Type: text/html";
	int comparisen_len = strlen(comparison);
	char *content_type_str = strstr(response, "Content-Type:");
	if(strncmp(comparison,content_type_str, comparisen_len) == 0){
		return 1;
	}
	return 0;
			
}
