/*
	COMP30023 Project 1 2020
	by William Putra Intan
*/

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