#include "bkk_uds_client.h"
#include "bkk_stop_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

const char * test_stop_id = "F02122";

static void print_usage(const char *prog_name) {
  printf("Usage: %s [-s stop_name_substring]\n", prog_name);
}

int main(int argc, char **argv) {

  const char * substring = "Kossuth";
  int opt;
  while((opt = getopt(argc, argv, "s:h")) != -1) {
    switch(opt) {
      case 's':
        if(optarg != NULL && optarg[0] != '\0') {
          substring = optarg;
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

  printf("Stop list size: %d\n", get_stop_list_size());
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

  for (size_t i = 0; i < count; i++) {
    bkk_stop_t stop;
    const bkk_stop_stat_t stat = find_stop_by_index(indices[i], &stop);
    if(stat != BKK_STOP_FOUND) {
      printf("Failed to retrieve stop at index %zu\n", indices[i]);
      continue;
    }

    const int res = send_bkk_uds_query(stop.stop_id, &response);

    if(res == 0) {
      printf("Received %d arrivals for stop_id %s\n", 
        response.number_of_arrivals, stop.stop_id);

      for(int i = 0; i < response.number_of_arrivals; i++) {
        printf("Arrival %d: Line: %s, Destination: %s, "
          "Departure Time: %s, Departs in: %d min\n", 
          i + 1, 
          response.arrivals[i].line_id, 
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
  
  free(indices);
  return 0; 
}