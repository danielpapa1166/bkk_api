#include <iostream>
#include <string>
#include <cstdlib>
#include "bkk_api.hpp"
#include "bkk_uds_protocol.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <thread>
#include <unistd.h>


static const int MAX_EVENTS = 10;


static int init_server(int * const event_fd, int * const server_fd);
static void handle_client(int client_fd, BkkApi & bkk_api); 


int main(int argc, char* argv[]) {
  (void) argc;
  (void) argv;

  BkkApi bkk_api;

  int event_fd, server_fd;
  if(init_server(&event_fd, &server_fd) != 0) {
    return 1;
  }

  
  epoll_event events[MAX_EVENTS];

  while (1) {
    int num_events = epoll_wait(
      event_fd, events, 
      MAX_EVENTS, 
      -1); 

    if(num_events < 0) {
      printf("Failed to wait for events\n");
      return 1;
    }

    for (int i = 0; i < num_events; ++i) {
      if(events[i].data.fd == server_fd) {
        printf("New client connection\n");
        int client_fd = accept(server_fd, nullptr, nullptr);
        if(client_fd < 0) {
          printf("Failed to accept client connection\n");
          continue;
        }

        std::thread(handle_client, client_fd, std::ref(bkk_api)).detach();

      }
    }
  }

  close(server_fd); 
  unlink(BKK_UDS_SOCKET_PATH); 
  return 0; 
}


static int init_server(int * const event_fd, int * const server_fd) {
    unlink(BKK_UDS_SOCKET_PATH); // Remove existing socket file if it exists
  
  *server_fd = socket(AF_UNIX, SOCK_SEQPACKET, 0); 
  if(*server_fd < 0) {
    printf("Failed to create socket\n");
    return 1;
  }

  sockaddr_un server_addr {};
  server_addr.sun_family = AF_UNIX;
  strncpy(
    server_addr.sun_path, 
    BKK_UDS_SOCKET_PATH, 
    sizeof(server_addr.sun_path) - 1);
 
  const int bind_res = bind(
    *server_fd, 
    (sockaddr*)&server_addr, 
    sizeof(server_addr));

  if(bind_res < 0) {
    printf("Failed to bind socket\n");
    return 1;
  }

  const int listen_res = listen(*server_fd, 32);
  if(listen_res < 0) {
    printf("Failed to listen on socket\n");
    return 1;
  }

  printf("Server is listening on %s\n", BKK_UDS_SOCKET_PATH);

  *event_fd = epoll_create1(0);
  if(*event_fd < 0) {
    printf("Failed to create epoll instance\n");
    return 1;
  }

  epoll_event event {};
  event.events = EPOLLIN;
  event.data.fd = *server_fd;

  const int ctl_res = epoll_ctl(*event_fd, EPOLL_CTL_ADD, *server_fd, &event);
  if(ctl_res < 0) {
    printf("Failed to add server socket to epoll\n");
    return 1;
  }

  return 0; 
}

static void handle_client(int client_fd, BkkApi & bkk_api) {
  printf("Handling client on fd %d\n", client_fd);
  bkk_uds_request_t request {};
  ssize_t n = recv(client_fd, &request, sizeof(request), 0);
  if (n != sizeof(request)) {
    printf("Failed to receive data from client\n");
    close(client_fd);
    return;
  }

  printf("Received request for stop_id: %s\n", request.stop_id);

  std::vector<Arrival> arrivals = bkk_api.get_arrivals_for_station(
    request.stop_id); 
  bkk_api.display_arrivals(arrivals); 

  int num_arrivals = std::min((int)arrivals.size(), BKK_UDS_MAX_ARRIVALS);


  bkk_uds_response_t response {};

  response.number_of_arrivals = num_arrivals;
  for (int i = 0; i < num_arrivals; i++) {
    response.arrivals[i] = arrivals[i];
  }


  ssize_t sent_size = send(client_fd, &response, sizeof(response), 0);
  if (sent_size != sizeof(response)) {
    printf("Failed to send response to client\n");
  }

  close(client_fd);

}