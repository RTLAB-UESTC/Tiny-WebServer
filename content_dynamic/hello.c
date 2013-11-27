#include <stdio.h>
#include "../mylib.h"

int main()
{
	char *buf, *p;
	char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
	int n1 = 0, n2 = 0;

	if((buf = getenv("QUERY_STRING")) != NULL){
		p = strchr(buf, '&');
		*p = '\0';
		strcpy(arg1, buf);
		strcpy(arg2, p+1);
		n1 = atoi(arg1);
		n2 = atoi(arg2);
	}

	sprintf(content, "Welcome to Tiny Webserver\r\n<p>");
	sprintf(content, "%sThis is a \"Hello world\" example of a dynamic content\r\n<p>", content);
	sprintf(content, "%sThe answer of %d + %d is %d\r\n<p>", content, n1, n2, n1+n2);
	sprintf(content, "%sThanks for visiting!\r\n", content);

	printf("Conetnt-length: %d\r\n", (int)strlen(content));
	printf("Content-type: text/html\r\n\r\n");
	printf("%s", content);

	fflush(stdout);
	return 0;
}

