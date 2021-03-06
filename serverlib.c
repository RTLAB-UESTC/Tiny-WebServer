#include "serverlib.h"


void *doit(void *connfd)
{
	int is_static;
	int fd = *((int*)connfd);
	struct stat sbuf;
	char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
	char filename[MAXLINE], cgiargs[MAXLINE];
	rio_t rio;

	/* Read request line and headers */
	Rio_readinitb(&rio, fd);
	Rio_readlineb(&rio, buf, MAXLINE);                   //line:netp:doit:readrequest
	sscanf(buf, "%s %s %s", method, uri, version);       //line:netp:doit:parserequest
	if (strcasecmp(method, "GET")) {                     //line:netp:doit:beginrequesterr
	   clienterror(fd, method, "501", "Not Implemented",
				"Tiny does not implement this method");
		goto end;
	}                                                    //line:netp:doit:endrequesterr
	read_requesthdrs(&rio);                              //line:netp:doit:readrequesthdrs

	/* Parse URI from GET request */
	is_static = parse_uri(uri, filename, cgiargs);       //line:netp:doit:staticcheck
	if (stat(filename, &sbuf) < 0) {                     //line:netp:doit:beginnotfound
		clienterror(fd, filename, "404", "Not found",
			"Tiny couldn't find this file");
		goto end;
	}                                                    //line:netp:doit:endnotfound

	if (is_static) { /* Serve static content */
		if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) { //line:netp:doit:readable
			clienterror(fd, filename, "403", "Forbidden",
				"Tiny couldn't read the file");
			goto end;
		}
		serve_static(fd, filename, sbuf.st_size);        //line:netp:doit:servestatic
	}
	else { /* Serve dynamic content */
		if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) { //line:netp:doit:executable
			clienterror(fd, filename, "403", "Forbidden",
				"Tiny couldn't run the CGI program");
			goto end;
		}
		serve_dynamic(fd, filename, cgiargs);            //line:netp:doit:servedynamic
	}
end:
	fprintf(stderr, "client %d is disconnecting\n", fd);
	Close(fd);
	fprintf(stderr, "client %d disconnected\n", fd);
	return NULL;
}

void read_requesthdrs(rio_t *rp)
{
	char buf[MAXLINE];

	Rio_readlineb(rp, buf, MAXLINE);
	while(strcmp(buf, "\r\n")) {          //line:netp:readhdrs:checkterm
		if(Rio_readlineb(rp, buf, MAXLINE) < 0)
			break;
		//printf("%s", buf);
	}
	return;
}

int parse_uri(char *uri, char *filename, char *cgiargs)
{
	char *ptr;
	int ishome;

	if(strstr(uri, "content_dynamic/")){
		ptr = index(uri, '?');
		if(ptr){
			strcpy(cgiargs, ptr+1);
			*ptr = '\0';
		}
		else{
			strcpy(cgiargs, "");
		}
		strcpy(filename, ".");
		strcat(filename, uri);
		return 0;
	}
	else {
		ishome = !strcmp(uri, "/");
		strcpy(cgiargs, "");
		strcpy(filename, ".");
		strcat(filename, uri);
		if(ishome){
			strcat(filename, "content_static/home.html");
		}
		return 1;
	}

	if (!strstr(uri, "cgi-bin")) {  /* Static content */ //line:netp:parseuri:isstatic
		strcpy(cgiargs, "");                             //line:netp:parseuri:clearcgi
		strcpy(filename, ".");                           //line:netp:parseuri:beginconvert1
		strcat(filename, uri);                           //line:netp:parseuri:endconvert1
		if (uri[strlen(uri)-1] == '/')                   //line:netp:parseuri:slashcheck
			strcat(filename, "content_static/home.html");               //line:netp:parseuri:appenddefault
		return 1;
	}
}



void serve_static(int fd, char *filename, int filesize)
{
	int srcfd;
	char *srcp, filetype[MAXLINE], buf[MAXBUF];

	/* Send response headers to client */
	get_filetype(filename, filetype);       //line:netp:servestatic:getfiletype
	sprintf(buf, "HTTP/1.0 200 OK\r\n");    //line:netp:servestatic:beginserve
	sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
	sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
	sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
	Rio_writen(fd, buf, strlen(buf));       //line:netp:servestatic:endserve

	/* Send response body to client */
	srcfd = Open(filename, O_RDONLY, 0);    //line:netp:servestatic:open
	fprintf(stderr, "file %s is opened in fd %d\n", filename, fd);
//	srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);//line:netp:servestatic:mmap
	srcp = Malloc(filesize);
	Read(srcfd, srcp, filesize);
	Close(srcfd);                           //line:netp:servestatic:close
	fprintf(stderr, "file %s is closed in fd %d\n", filename, fd);
	Rio_writen(fd, srcp, filesize);         //line:netp:servestatic:write
//	Munmap(srcp, filesize);                 //line:netp:servestatic:munmap
	Free(srcp);
}

void get_filetype(char *filename, char *filetype)
{
	if (strstr(filename, ".htm") || strstr(filename, ".html"))
		strcpy(filetype, "text/html");
	else if (strstr(filename, ".gif"))
		strcpy(filetype, "image/gif");
	else if (strstr(filename, ".jpg"))
		strcpy(filetype, "image/jpeg");
	else if (strstr(filename, ".mp3"))
		strcpy(filetype, "audio/x-mpeg");
	else if (strstr(filename, ".mp4"))
		strcpy(filetype, "video/mp4");
	else if (strstr(filename, ".flv"))
		strcpy(filetype, "flv-application/octet-stream");
	else
		strcpy(filetype, "text/plain");
}


void serve_dynamic(int fd, char *filename, char *cgiargs)
{
	char buf[MAXLINE], *emptylist[] = { NULL };

	/* Return first part of HTTP response */
	sprintf(buf, "HTTP/1.0 200 OK\r\n");
	Rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Server: Tiny Web Server\r\n");
	Rio_writen(fd, buf, strlen(buf));

	if (Fork() == 0) { /* child */ //line:netp:servedynamic:fork
	/* Real server would set all CGI vars here */
	setenv("QUERY_STRING", cgiargs, 1); //line:netp:servedynamic:setenv
	Dup2(fd, STDOUT_FILENO);         /* Redirect stdout to client */ //line:netp:servedynamic:dup2
	Execve(filename, emptylist, environ); /* Run CGI program */ //line:netp:servedynamic:execve
	}
	Wait(NULL); /* Parent waits for and reaps child */ //line:netp:servedynamic:wait
}


void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
{
	char buf[MAXLINE], body[MAXBUF];

	/* Build the HTTP response body */
	sprintf(body, "<html><title>Tiny Error</title>");
	sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
	sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
	sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
	sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

	/* Print the HTTP response */
	sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
	Rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Content-type: text/html\r\n");
	Rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
	Rio_writen(fd, buf, strlen(buf));
	Rio_writen(fd, body, strlen(body));
}



