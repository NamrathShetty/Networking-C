#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>       
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
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
  addr.sin_port = htons(8000);
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
    ds_string_slice_to_owned(&request, &path);
    printf("path requested: %s\n", path);

    ds_string_slice_tokenize(&request, '\n', &token);
    char *protocol = NULL;
    ds_string_slice_to_owned(&token, &protocol);
    printf("protocol: %s\n", protocol);
    
    int response_len = snprintf(NULL, response, 0, "%s 200 OK\nContent-Type: text/html\nContent-Length: %d\n\n%s", protocol, 6, "Hello\n")
    char *response = calloc(response_len, sizeof(char));
    snprintf(response, 0, "%s 200 OK\nContent-Type: text/html\nContent-Length: %d\n\n%s", protocol, 6, "Hello\n")

    write(cfd, "%s 200 OK\nContent-Type: text/html\nContent-Length: %d\n\n%s", 7);
    }
  result = close(fd);
  if (result == -1) {
    DS_PANIC("close");
  }

  return 0;
}
