#include <string>
#include <cstdlib>
#include <fstream>
#include <cctype>
#include <cerrno>
#include <climits>
#include <chrono>

#include "bkk_api.hpp"
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

#if defined(__GNUC__) || defined(__clang__)
#define BKK_MAYBE_UNUSED __attribute__((unused))
#else
#define BKK_MAYBE_UNUSED
#endif


static const int MAX_EVENTS = 10;
static std::string bkk_api_key; 


static int init_server(int * const event_fd, int * const server_fd);
static void print_usage(const char * prog_name);
static std::string read_api_key_from_file(const std::string & path);
static void handle_client(int client_fd, UdsCache & cache); 
static bkk_api_status_t fresh_fetch_and_update_cache(
  const std::string & stop_id, UdsCache & cache, std::vector<Arrival> * arrivals_out);

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

  if(!config.api_key_file_path.empty()) {
    try {
      bkk_api_key = read_api_key_from_file(config.api_key_file_path);
    } catch (const std::exception& e) {
      log_error("Init", ("Failed to read API key from file: " + std::string(e.what())).c_str());
      return cleanup_and_return(1);
    }
  } else {
    bkk_api_key = bkk_api::get_env_var("BKK_API_KEY");
  }

  if (bkk_api_key.empty()) {
    printf("API key is required (set BKK_API_KEY or pass -k <key_file_path>)\n");
    log_error("Init", "API key not provided");
    return cleanup_and_return(1);
  }


  const size_t num_threads = std::thread::hardware_concurrency();
  ThreadPool thread_pool(num_threads);

  thread_pool.submit(test_fun);
  thread_pool.submit(test_fun);
  

  log_info("Init", ("Using " + std::to_string(num_threads) + " threads").c_str());

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

        std::thread(handle_client, client_fd, std::ref(cache)).detach();

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
  bkk_uds_request_t request {};
  ssize_t n = recv(client_fd, &request, sizeof(request), 0);
  if (n != sizeof(request)) {
    printf("Failed to receive data from client\n");
    log_error("Runtime", "Failed to receive data from client");
    close(client_fd);
    return;
  }

  // check cache first:
  std::vector<Arrival> arrivals; 
  bkk_api_status_t api_status; 
  cache_entry_t cache_entry;
  cache_state_t cache_state = cache.get_element(request.stop_id, &cache_entry);
  if (cache_state == CACHE_HIT_FRESH) {
    arrivals = cache_entry.arrivals;
    // no failed fetch entry gets into the cache
    api_status = bkk_api_status::Ok;
  } 
  else if (cache_state == CACHE_HIT_STALE) {
    // fetch fresh data in the background 
    std::thread([&cache, request](){
      std::vector<Arrival> fresh_arrivals;
      (void) fresh_fetch_and_update_cache(
        request.stop_id, 
        cache, 
        &fresh_arrivals);
    }).detach();

    arrivals = cache_entry.arrivals;

    // no failed fetch entry gets into the cache
    api_status = bkk_api_status::Ok;
  }
  else {
    api_status = fresh_fetch_and_update_cache(
      request.stop_id, 
      cache, 
      &arrivals); 
  }

  int num_arrivals = std::min((int)arrivals.size(), BKK_UDS_MAX_ARRIVALS);


  bkk_uds_response_t response {};
  response.status = api_status;
  response.number_of_arrivals = num_arrivals;
  for (int i = 0; i < num_arrivals; i++) {
    response.arrivals[i] = arrivals[i];
  }


  ssize_t sent_size = send(client_fd, &response, sizeof(response), 0);
  if (sent_size != sizeof(response)) {
    log_error("Runtime", "Failed to send response to client");
  }

  close(client_fd);

  auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(
    std::chrono::steady_clock::now() - t_start).count();

  log_info("Request", ("Handled request for stop_id: " 
      + std::string(request.stop_id) 
      + ", returned " + std::to_string(num_arrivals) + " arrivals," 
      + " cache_status: " + cache_state_to_string(cache_state)
      + " in " + std::to_string(duration_us) + " us").c_str());

}


static bkk_api_status_t fresh_fetch_and_update_cache(
    const std::string & stop_id, UdsCache & cache, std::vector<Arrival> * arrivals_out) {

  std::vector<Arrival> fresh_arrivals;
  bkk_api_status_t fetch_status = bkk_api::get_arrivals_for_station(
    stop_id, bkk_api_key, &fresh_arrivals);
  if (fetch_status != bkk_api_status::Ok) {
    log_error("Fetch", ("Failed to fetch arrivals for stop ID: " + stop_id 
        + ", error: " + error_code_to_string(fetch_status)).c_str()); 
    return fetch_status; 
  }
  *arrivals_out = fresh_arrivals;

  cache_entry_t new_cache_entry {
    *arrivals_out, 
    std::chrono::steady_clock::now()
  };
  cache.put_element(stop_id, new_cache_entry);
  return bkk_api_status::Ok;
}