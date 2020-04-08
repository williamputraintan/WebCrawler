/*
	COMP30023 Project 1 2020
	by William Putra Intan
*/



#include <crawler.h>

/*Main Function of the program*/
int main(int argc, char **argv)
{	
	//error when no URL provided
	if (argc < 2) {
		fprintf(stderr,"ERROR, no URL provided\n");
		exit(1);
	}

	//intialize
	char *url_list[MAX_NUM_URL];
	int url_count = 0;
	char *curr_url = argv[1];
	
	//adding the first url to the array
	add_new_url(curr_url, url_list, &url_count);

	//Will go through the every url on the url_list array
	for(int i = 0; i < url_count; i++){
		curr_url = url_list[i];
		
		//Add hyperlink to the array list found in each url response
		add_hyperlink_from_url(url_list, &url_count, curr_url);
	}
	
	//free memory allocated on the pointers array
	for (int i = 0; i < url_count; i++){
		free(url_list[i]);
	}
	return 0;
}

