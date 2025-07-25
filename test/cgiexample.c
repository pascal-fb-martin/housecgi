// A minimal CGI application to test and debug the CGI mechanism.

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
// #include <errno.h>
 
 
static void showEnv (const char *name) {
   const char *value = getenv(name);
   if (!value) value = "(undefined)";
   printf ("<li><strong>%s</strong> = %s</li>\n", name, value);
}

void main()
{
  printf("Content-type: text/html\r\n\n");
 
  printf("<html>\n");
  printf("<head>\n");
  printf("<head><title>CGI Test Application</title></head>\n");
  printf("<body>\n");
  printf("<h1>CGI Test example.</h1>\n");
  printf("<h2>CGI Environment Variables:</h2><ul>\n");
 
  static char *names[] = {
      "CONTENT_LENGTH",
      "CONTENT_TYPE",
      "GATEWAY_INTERFACE",
      "HTTP_COOKIE",
      "HTTP_HOST",
      "HTTP_REFERER",
      "HTTP_USER_AGENT",
      "PATH_INFO",
      "PATH_TRANSLATED",
      "QUERY_STRING",
      "REDIRECT_STATUS",
      "REMOTE_ADDR",
      "REMOTE_HOST",
      "REQUEST_METHOD",
      "SCRIPT_NAME",
      "SERVER_NAME",
      "SERVER_PORT",
      "SERVER_PROTOCOL",
      "SERVER_SOFTWARE",
      0};

   int i;
   for (i = 0; names[i]; ++i) {
      showEnv (names[i]);
   }
   int length = 0;
   const char *contentLength = getenv ("CONTENT_LENGTH");
   if (contentLength) length = atoi (contentLength);

   if (length > 0) {
       char *data = malloc (length + 1); // Need space for null terminator.
       int read = fread(data, 1, length, stdin);
 
       printf ("<h2>Input Data</h2>\n");
       if (read == length) {
          data[length] = 0;
          printf ("%s\n", data);
       } else {
          printf("(Standard input error)\n");
       }
       free (data);
   }
   printf("</body></html>\n");
}

