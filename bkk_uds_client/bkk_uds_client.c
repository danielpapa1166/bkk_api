#include "bkk_uds_client.h"
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>


int send_bkk_uds_query(const char * stop_id, bkk_uds_response_t * response) {
  if(!stop_id || !response) {
    printf("Invalid arguments to send_bkk_uds_query\n");
    return 1;
  }
  
  int client_fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
  if(client_fd < 0) {
    printf("Failed to create socket\n");
    return 1;
  }

  struct sockaddr_un server_addr; 
  memset(&server_addr, 0, sizeof(server_addr)); 
  server_addr.sun_family = AF_UNIX;
  strncpy(server_addr.sun_path, BKK_UDS_SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

  const int connect_res = connect(
    client_fd, 
    (struct sockaddr*)&server_addr, 
    sizeof(server_addr));

  if(connect_res < 0) {
    printf("Failed to connect to server\n");
    close(client_fd);
    return 1;
  }

  bkk_uds_request_t request; 
  memset(&request, 0, sizeof(request)); 
  strncpy(request.stop_id, stop_id, sizeof(request.stop_id) - 1);

  const ssize_t sent_size = send(client_fd, &request, sizeof(request), 0);
  if(sent_size != sizeof(request)) {
    printf("Failed to send request to server\n");
    close(client_fd);
    return 1;
  }

  // Receive response from server
  const ssize_t recv_size = recv(client_fd, response, sizeof(*response), 0);
  close(client_fd);
  if(recv_size != sizeof(*response)) {
    printf("Failed to receive response from server\n");
    return 1;
  }

  return 0;
}
