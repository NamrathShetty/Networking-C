#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>       
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#define DS_IO_IMPLEMENTATION
#define DS_SS_IMPLEMENTATION
#define DS_SB_IMPLEMENTATION
#include "ds.h"

#define MAX_LEN 1024

#define handle_error(msg) \
  do { perror(msg); exit(EXIT_FAILURE); } while (0)

int main(int argc, char *argv[])
{ 
  int fd, cfd, result;
  socklen_t client_addr_size;
  struct sockaddr_in addr, client_addr;

  fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd == -1) {
    DS_PANIC("socket");
  }

  addr.sin_family = AF_INET;
  addr.sin_port = htons(8001);
  inet_pton(AF_INET, "0.0.0.0", &addr.sin_addr);
  result = bind(fd, (struct sockaddr *) &addr, sizeof(addr));

  if (result == -1) {
    DS_PANIC("bind");
  }

  result = listen(fd, 10);
  if (result == -1) {
    DS_PANIC("listen");
  }

  while (1) {
    client_addr_size = sizeof(client_addr);
    cfd = accept(fd, (struct sockaddr *) &client_addr, &client_addr_size);
    if (cfd == -1) {
      DS_PANIC("accept");
    }
    
    char buffer[MAX_LEN] = {0};
    int result = read(cfd, buffer, MAX_LEN);
    if (result == -1) {
      DS_LOG_ERROR("read");
      continue;
    }
    unsigned int buffer_len = result;
    // printf("client (%u): '%s'\n", buffer_len, buffer);

    // client (82): 'GET /hello HTTP/1.1
    // Host: localhost:8000
    // User-Agent: curl/8.5.0
    // Accept: */*

    ds_string_slice request, token;
    ds_string_slice_init(&request, buffer, buffer_len);

    // VERB
    ds_string_slice_tokenize(&request, ' ', &token);
    
    char *verb = NULL;
    ds_string_slice_to_owned(&token, &verb);
    if (strcmp(verb, "GET")) {
      DS_LOG_ERROR("not a get request");
      continue;
    }

    ds_string_slice_tokenize(&request, ' ', &token);
    char *path = NULL;
    ds_string_slice_to_owned(&token, &path);

    struct stat path_stat;
    result = stat(path + 1, &path_stat);

    if (result != 0) {
      DS_LOG_ERROR("stat");
      continue;
    }

    char *content = NULL;
    int content_len = 0;
    if (S_ISREG(path_stat.st_mode)) { 
      content_len = ds_io_read_file(path + 1, &content);
    } else if (S_ISDIR(path_stat.st_mode)) {
      ds_string_builder directory_builder;
      ds_string_builder_init(&directory_builder);
      ds_string_builder_append(&directory_builder,
                               "<!DOCTYPE HTML>\n<html lang=\"en\">\n<head>\n<meta "
                               "charset=\"utf-8\">\n<title>Direcotory listing for "
                               "%s</title>\n<head>\n"
                               "<body>\n<h1>Direcotory listing for %s</h1>\n</hr>\n<ul>\n",
                                path, path);
      
      DIR *directory = opendir(path + 1);
      struct dirent *dir;
      if (directory) {
        while ((dir = readdir(directory)) != NULL) {
          printf("%s\n", dir->d_name);
          ds_string_builder_append(&directory_builder, "<li><a href=\"%s/%s\">%s</a></li>\n", path + 1, dir->d_name, dir->d_name);
        }
        closedir(directory);
      }
      ds_string_builder_append(&directory_builder, "</ul>\n</hr>\n</body>\n</html>\n");
      ds_string_builder_build(&directory_builder, &content);
      content_len = strlen(content);
    } else {
      DS_LOG_ERROR("mode not supported");
    }
    ds_string_builder response_builder;
    ds_string_builder_init(&response_builder);

    ds_string_builder_append(&response_builder, "HTTP/1.1 200 OK\nContent-Type: text/html\nContent-Length: %d\n\n%s", content_len, content);
    
    char *response = NULL;
    ds_string_builder_build(&response_builder, &response);
    int response_len = strlen(response);

    // send one message
    write(cfd, response, response_len);
  }
  result = close(fd);
  if (result == -1) {
    DS_PANIC("close");
  }

  return 0;
}
