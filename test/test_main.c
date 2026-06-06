#include "bkk_uds_client.h"
#include "bkk_stop_utils.h"
#include <rbuflogd/logger.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

const char * test_stop_id = "F02122";

static void print_usage(const char *prog_name) {
  printf("Usage: %s [-s stop_name_substring]\n", prog_name);
}

int main(int argc, char **argv) {
  rbuflogd_logger_init("bkk_clnt");

  const char * substring = "Kossuth";
  const char * key_path = NULL; 
  int opt;
  while((opt = getopt(argc, argv, "s:h:k:")) != -1) {
    switch(opt) {
      case 's':
        if(optarg != NULL && optarg[0] != '\0') {
          substring = optarg;
        }
        break;
      case 'h':
        print_usage(argv[0]);
        return 0;
      case 'k':
        if(optarg != NULL && optarg[0] != '\0') {
          key_path = optarg;
        }
        else {
          printf("Invalid API key file path\n");
        }
        break;
      default:
        print_usage(argv[0]);
        return 1;
    }
  }

  if(key_path == NULL) {
    log_error("Init", "API key file path is required");
    print_usage(argv[0]);
    return 1;
  }

  char * key_val; 
  const int api_key_stat = read_api_key_from_file(key_path, &key_val);

  if(api_key_stat != API_KEY_READ_OK) {
    log_error("Init", "Failed to read API key from file");
    return 1;
  }

  struct timespec t_start, t_end;

  clock_gettime(CLOCK_MONOTONIC, &t_start);
  size_t * indices = NULL;
  size_t count = 0;
  bkk_stop_stat_t stat = find_stops_by_name_substring(
    substring, 
    &indices, 
    &count);

  if(stat == BKK_STOP_FOUND) {
    printf("Found %zu stops matching '%s':\n", count, substring);
    display_stop_list(indices, count);
  } 
  else {
    printf("No stops found matching '%s'\n", substring);
  }
  clock_gettime(CLOCK_MONOTONIC, &t_end);
  long elapsed_us = (t_end.tv_sec - t_start.tv_sec) * 1000000L
                  + (t_end.tv_nsec - t_start.tv_nsec) / 1000L;
  printf("Search time: %ld us\n", elapsed_us);

  bkk_uds_response_t response;


  clock_gettime(CLOCK_MONOTONIC, &t_start);

  rbuflogd_logger_info("init", "BKK API test client init succeeded");
  char msg[256];

  for (size_t i = 0; i < count; i++) {
    bkk_stop_t stop;
    const bkk_stop_stat_t stat = find_stop_by_index(indices[i], &stop);
    if(stat != BKK_STOP_FOUND) {
      printf("Failed to retrieve stop at index %zu\n", indices[i]);
      continue;
    }

    bkk_uds_request_t request = { 0 };
    strncpy(request.stop_id, stop.stop_id, sizeof(request.stop_id) - 1);
    strncpy(request.api_key, key_val, sizeof(request.api_key) - 1);

    const bkk_client_status_t res = send_bkk_uds_query(&request, &response);
    const bkk_api_status_t api_status = response.status;

    snprintf(msg, sizeof(msg), 
      "Query for stop_id %s returned status: %d, api fetch status: %s", 
      stop.stop_id, 
      (int)res, 
      error_code_to_string(api_status));
      
    rbuflogd_logger_info("querry", msg);


    if(res == client_OK) {
      printf("Received %d arrivals for stop_id %s\n", 
        response.number_of_arrivals, stop.stop_id);

      for(int i = 0; i < response.number_of_arrivals; i++) {
        printf("Arrival %d: Line: %s, Destination: %s, "
          "Departure Time: %s, Departs in: %d min\n", 
          i + 1, 
          response.arrivals[i].line_id, 
          vehicle_type_to_string(response.arrivals[i].vehicle_type),
          response.arrivals[i].destination, 
          response.arrivals[i].departure_time, 
          response.arrivals[i].departs_in_min);
      }
    } 
    else {
      printf("Failed to get response for stop_id %s\n", stop.stop_id);
    }
  }

  clock_gettime(CLOCK_MONOTONIC, &t_end);

  elapsed_us = (t_end.tv_sec - t_start.tv_sec) * 1000000L
                  + (t_end.tv_nsec - t_start.tv_nsec) / 1000L;
  printf("Total fetch time: %ld us\n", elapsed_us);
  
  free(key_val);
  free(indices);
  return 0; 
}