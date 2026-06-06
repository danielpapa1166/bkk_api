#include <string>
#include <cstdlib>
#include <cctype>
#include <cerrno>
#include <climits>


#include "bkk_api_types.h"
#include "bkk_uds_protocol.h"
#include "bkk_cache.hpp"
#include "bkk_thread_pool.hpp"
#include "bkk_arg_parser.hpp"

#include <curl/curl.h>
#include <rbuflogd/logger.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <thread>
#include <unistd.h>


static const int MAX_EVENTS = 10;

static int init_server(int * const event_fd, int * const server_fd);
static void print_usage(const char * prog_name);

void test_fun() {
  printf("This is a test function to keep the thread pool worker threads busy\n");
}

int main(int argc, char* argv[]) {

  const int log_init_status = rbuflogd_logger_init("bkk_srv");
  if (log_init_status != 0) {
    printf("Failed to initialize logger\n");
  }

  arg_config_t config {};
  const parse_status_t parse_status = parse_arguments(
    argc, argv, &config);

  if(parse_status != parse_status::ok) {
    print_usage(argv[0]);
    return parse_status == parse_status::help ? 0 : 1;
  }


  const CURLcode curl_init_status = curl_global_init(CURL_GLOBAL_DEFAULT);
  if (curl_init_status != CURLE_OK) {
    log_error("Init", ("Failed to initialize libcurl: " 
      + std::string(curl_easy_strerror(curl_init_status))).c_str());
    return 1;
  }

  const auto cleanup_and_return = [](int code) {
    curl_global_cleanup();
    return code;
  };


  const size_t num_threads = std::thread::hardware_concurrency();
  ThreadPool thread_pool(num_threads);
  
  cache_config_t cache_config {
    config.freshness_seconds,
    config.staleness_seconds,
    config.max_cache_size
  };

  int event_fd, server_fd;
  if(init_server(&event_fd, &server_fd) != 0) {
    log_error("Init", "Failed to initialize server");
    return cleanup_and_return(1);
  }

  UdsCache cache(&cache_config);
  epoll_event events[MAX_EVENTS];

  log_info("Init", "Server initialized successfully");

  while (1) {
    int num_events = epoll_wait(
      event_fd, events, 
      MAX_EVENTS, 
      -1); 

    if(num_events < 0) {
      printf("Failed to wait for events\n");
      log_error("Runtime", "Failed to wait for events");
      return cleanup_and_return(1);
    }

    for (int i = 0; i < num_events; ++i) {
      if(events[i].data.fd == server_fd) {
        int client_fd = accept(server_fd, nullptr, nullptr);
        if(client_fd < 0) {
          log_error("Runtime", "Failed to accept client connection");
          continue;
        }

        // submit a task to handle the client request in the thread pool
        thread_pool.submit(client_fd, cache);

      }
    }
  }

  close(server_fd); 
  unlink(BKK_UDS_SOCKET_PATH); 
  curl_global_cleanup();
  return 0; 
}

static void print_usage(const char * prog_name) {
  printf("Usage: %s [-k api_key_file] "
    "[-f freshness_seconds] [-s staleness_seconds] [-l max_cache_size]\n",
    prog_name);
}


static int init_server(int * const event_fd, int * const server_fd) {
    unlink(BKK_UDS_SOCKET_PATH); // Remove existing socket file if it exists
  
  *server_fd = socket(AF_UNIX, SOCK_SEQPACKET, 0); 
  if(*server_fd < 0) {
    printf("Failed to create socket\n");
    log_error("Init", "Failed to create socket");
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
    log_error("Init", "Failed to bind socket");
    return 1;
  }

  const int listen_res = listen(*server_fd, 32);
  if(listen_res < 0) {
    printf("Failed to listen on socket\n");
    log_error("Init", "Failed to listen on socket");
    return 1;
  }

  printf("Server is listening on %s\n", BKK_UDS_SOCKET_PATH);

  *event_fd = epoll_create1(0);
  if(*event_fd < 0) {
    printf("Failed to create epoll instance\n");
    log_error("Init", "Failed to create epoll instance");
    return 1;
  }

  epoll_event event {};
  event.events = EPOLLIN;
  event.data.fd = *server_fd;

  const int ctl_res = epoll_ctl(*event_fd, EPOLL_CTL_ADD, *server_fd, &event);
  if(ctl_res < 0) {
    printf("Failed to add server socket to epoll\n");
    log_error("Init", "Failed to add server socket to epoll");
    return 1;
  }

  return 0; 
}
