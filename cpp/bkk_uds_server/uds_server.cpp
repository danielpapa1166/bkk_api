#include <iostream>
#include <string>
#include <cstdlib>
#include <fstream>
#include <cctype>
#include <cerrno>
#include <climits>
#include <chrono>

#include "bkk_api.hpp"
#include "bkk_uds_protocol.h"
#include "bkk_cache.hpp"
#include "bkk_stop_utils.h"

#include <rbuflogd/producer.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <thread>
#include <unistd.h>


static const int MAX_EVENTS = 10;
static std::string bkk_api_key; 
static rbuflogd_producer_t * logger = new rbuflogd_producer_t;


static int init_server(int * const event_fd, int * const server_fd);
static void print_usage(const char * prog_name);
static bool parse_positive_int(const char * arg, int * out_value);
static std::string read_api_key_from_file(const std::string & path);
static void handle_client(int client_fd, UdsCache & cache); 
static void fresh_fetch_and_update_cache(
  const std::string & stop_id, UdsCache & cache, std::vector<Arrival> * arrivals_out);
static void log_debug(const std::string & category, const std::string & message);
static void log_info(const std::string & category, const std::string & message); 
static void log_error(const std::string & category, const std::string & message);



int main(int argc, char* argv[]) {

  if (rbuflogd_producer_open(logger, "bkk_srv") != 0) {
    logger = nullptr;
    printf("Failed to initialize logger\n");
  }

  std::string api_key_file_path;
  int freshness_seconds = 10;
  int staleness_seconds = 20;
  int max_cache_size = 100;

  int opt;
  while((opt = getopt(argc, argv, "k:f:s:l:h")) != -1) {
    switch(opt) {
      case 'k':
        if(optarg != nullptr && optarg[0] != '\0') {
          api_key_file_path = optarg;
        }
        break;
      case 'f':
        if(!parse_positive_int(optarg, &freshness_seconds)) {
          printf("Invalid freshness seconds: %s\n", optarg);
          log_error("Init", "Invalid freshness seconds");
          print_usage(argv[0]);
          return 1;
        }
        break;
      case 's':
        if(!parse_positive_int(optarg, &staleness_seconds)) {
          printf("Invalid staleness seconds: %s\n", optarg);
          log_error("Init", "Invalid staleness seconds");
          print_usage(argv[0]);
          return 1;
        }
        break;
      case 'l':
        if(!parse_positive_int(optarg, &max_cache_size)) {
          printf("Invalid max cache size: %s\n", optarg);
          log_error("Init", "Invalid max cache size");
          print_usage(argv[0]);
          return 1;
        }
        break;
      case 'h':
        print_usage(argv[0]);
        return 0;
      default:
        print_usage(argv[0]);
        return 1;
    }
  }

  if(staleness_seconds < freshness_seconds) {
    printf("Invalid cache config: staleness (%d) must be >= freshness (%d)\n",
      staleness_seconds, freshness_seconds);
    log_error("Init", "Invalid cache config: staleness must be >= freshness");
    return 1;
  }

  if(!api_key_file_path.empty()) {
    try {
      bkk_api_key = read_api_key_from_file(api_key_file_path);
    } catch (const std::exception& e) {
      printf("Failed to read API key from file: %s\n", e.what());
      log_error("Init", "Failed to read API key from file");
      return 1;
    }
  } else {
    bkk_api_key = bkk_api::get_env_var("BKK_API_KEY");
  }

  if (bkk_api_key.empty()) {
    printf("API key is required (set BKK_API_KEY or pass -k <key_file_path>)\n");
    log_error("Init", "API key not provided");
    return 1;
  }

  cache_config_t cache_config {
    freshness_seconds,
    staleness_seconds,
    max_cache_size
  };

  int event_fd, server_fd;
  if(init_server(&event_fd, &server_fd) != 0) {
    log_error("Init", "Failed to initialize server");
    return 1;
  }

  UdsCache cache(&cache_config);
  epoll_event events[MAX_EVENTS];

  log_debug("Init", "Server initialized successfully");

  while (1) {
    int num_events = epoll_wait(
      event_fd, events, 
      MAX_EVENTS, 
      -1); 

    if(num_events < 0) {
      printf("Failed to wait for events\n");
      log_error("Runtime", "Failed to wait for events");
      return 1;
    }

    for (int i = 0; i < num_events; ++i) {
      if(events[i].data.fd == server_fd) {
        printf("New client connection\n");
        int client_fd = accept(server_fd, nullptr, nullptr);
        if(client_fd < 0) {
          printf("Failed to accept client connection\n");
          log_error("Runtime", "Failed to accept client connection");
          continue;
        }

        std::thread(handle_client, client_fd, std::ref(cache)).detach();

      }
    }
  }

  close(server_fd); 
  unlink(BKK_UDS_SOCKET_PATH); 
  return 0; 
}

static void print_usage(const char * prog_name) {
  printf("Usage: %s [-k api_key_file] [-f freshness_seconds] [-s staleness_seconds] [-l max_cache_size]\n",
    prog_name);
}

static bool parse_positive_int(const char * arg, int * out_value) {
  if(arg == nullptr || out_value == nullptr) {
    return false;
  }

  errno = 0;
  char * endptr = nullptr;
  long value = strtol(arg, &endptr, 10);

  if(errno != 0 || endptr == arg || *endptr != '\0') {
    return false;
  }
  if(value <= 0 || value > INT_MAX) {
    return false;
  }

  *out_value = (int)value;
  return true;
}

static std::string read_api_key_from_file(const std::string & path) {
  std::ifstream infile(path);
  if(!infile.is_open()) {
    throw std::runtime_error("cannot open file: " + path);
  }

  std::string key;
  std::getline(infile, key);

  // trim leading/trailing whitespace so newline-terminated files work out of the box
  size_t begin = 0;
  while(begin < key.size() && std::isspace((unsigned char)key[begin])) {
    begin++;
  }

  size_t end = key.size();
  while(end > begin && std::isspace((unsigned char)key[end - 1])) {
    end--;
  }

  return key.substr(begin, end - begin);
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

static void handle_client(int client_fd, UdsCache & cache) {
  const auto t_start = std::chrono::steady_clock::now();
  printf("Handling client on fd %d\n", client_fd);
  bkk_uds_request_t request {};
  ssize_t n = recv(client_fd, &request, sizeof(request), 0);
  if (n != sizeof(request)) {
    printf("Failed to receive data from client\n");
    log_error("Runtime", "Failed to receive data from client");
    close(client_fd);
    return;
  }

  printf("Received request for stop_id: %s\n", request.stop_id);

  // check cache first:
  std::vector<Arrival> arrivals; 
  cache_entry_t cache_entry;
  cache_state_t cache_state = cache.get_element(request.stop_id, &cache_entry);
  if (cache_state == CACHE_HIT_FRESH) {
    arrivals = cache_entry.arrivals;
    printf("Cache hit (fresh) for stop_id: %s\n", request.stop_id);
  } 
  else if (cache_state == CACHE_HIT_STALE) {
    printf("Cache hit (stale) for stop_id: %s\n", request.stop_id);

    // fetch fresh data in the background 
    std::thread([&cache, request](){
      std::vector<Arrival> fresh_arrivals;
      fresh_fetch_and_update_cache(
        request.stop_id, 
        cache, 
        &fresh_arrivals);
    }).detach();

    arrivals = cache_entry.arrivals;
  }
  else {
    printf("Cache miss for stop_id: %s\n", request.stop_id);
    fresh_fetch_and_update_cache(
      request.stop_id, 
      cache, 
      &arrivals); 
  }

  int num_arrivals = std::min((int)arrivals.size(), BKK_UDS_MAX_ARRIVALS);


  bkk_uds_response_t response {};

  response.number_of_arrivals = num_arrivals;
  for (int i = 0; i < num_arrivals; i++) {
    response.arrivals[i] = arrivals[i];
  }


  ssize_t sent_size = send(client_fd, &response, sizeof(response), 0);
  if (sent_size != sizeof(response)) {
    printf("Failed to send response to client\n");
    log_error("Runtime", "Failed to send response to client");
  }

  close(client_fd);

  // log message debug: 
  char log_msg[256];
  char cache_status_str[16];
  switch(cache_state) {
    case CACHE_HIT_FRESH:
      strncpy(cache_status_str, "HIT_FRESH", sizeof(cache_status_str) -
        1);
      break;
    case CACHE_HIT_STALE:
      strncpy(cache_status_str, "HIT_STALE", sizeof(cache_status_str) -
        1);
      break;
    case CACHE_MISS:
    default:  
      strncpy(cache_status_str, "MISS", sizeof(cache_status_str) - 1);
      break;
  }
  snprintf(
    log_msg, 
    sizeof(log_msg), 
    "Handled request for stop_id: %s, returned %d arrivals in %lld us, cache_status: %s",
    request.stop_id,
    num_arrivals,
    (long long)std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::steady_clock::now() - t_start).count(),
    cache_status_str);
  log_debug("Request", log_msg);

}


static void fresh_fetch_and_update_cache(
    const std::string & stop_id, UdsCache & cache, std::vector<Arrival> * arrivals_out) {
  *arrivals_out = bkk_api::get_arrivals_for_station(
    stop_id, bkk_api_key); 

  cache_entry_t new_cache_entry {
    *arrivals_out, 
    std::chrono::steady_clock::now()
  };
  cache.put_element(stop_id, new_cache_entry);
}


static void log_debug(const std::string & category, const std::string & message) {
  if(logger == nullptr) {
    return; 
  }
  rbuflogd_producer_log(
    logger, RBUF_LOG_LEVEL_DEBUG, 
    category.c_str(), message.c_str()); 
}
static void log_info(const std::string & category, const std::string & message) {
  if(logger == nullptr) {
    return; 
  }
  rbuflogd_producer_log(
    logger, RBUF_LOG_LEVEL_INFO, 
    category.c_str(), message.c_str()); 
}
static void log_warn(const std::string & category, const std::string & message) {
  if(logger == nullptr) {
    return; 
  }
  rbuflogd_producer_log(
    logger, RBUF_LOG_LEVEL_WARNING, 
    category.c_str(), message.c_str()); 
}
static void log_error(const std::string & category, const std::string & message) {
  if(logger == nullptr) {
    return; 
  }
  rbuflogd_producer_log(
    logger, RBUF_LOG_LEVEL_ERROR, 
    category.c_str(), message.c_str()); 
}