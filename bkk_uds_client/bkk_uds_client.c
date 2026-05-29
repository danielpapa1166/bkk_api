#include "bkk_uds_client.h"
#include "bkk_api_types.h"
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>


bkk_client_status_t send_bkk_uds_query(const bkk_uds_request_t * request, bkk_uds_response_t * response) {
  if(!request || !response) {
    printf("Invalid arguments to send_bkk_uds_query\n");
    return client_InvalidArguments;
  }
  
  int client_fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
  if(client_fd < 0) {
    printf("Failed to create socket\n");
    return client_SocketError;
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
    return client_ConnectionFailed;
  }

  const ssize_t sent_size = send(client_fd, request, sizeof(*request), 0);
  if(sent_size != sizeof(*request)) {
    printf("Failed to send request to server\n");
    close(client_fd);
    return client_SendFailed;
  }

  // Receive response from server
  const ssize_t recv_size = recv(client_fd, response, sizeof(*response), 0);
  close(client_fd);
  if(recv_size != sizeof(*response)) {
    printf("Failed to receive response from server\n");
    return client_ReceiveFailed;
  }

  return client_OK;
}
