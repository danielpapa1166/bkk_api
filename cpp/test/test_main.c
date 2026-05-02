#include "bkk_uds_client.h"
#include <stdio.h>

const char * test_stop_id = "F02122";

int main() {
  bkk_uds_response_t response; 
  const int res = send_bkk_uds_query(test_stop_id, &response); 
  if(res == 0) {
    printf("Received %d arrivals for stop_id %s\n", response.number_of_arrivals, test_stop_id);
    for(int i = 0; i < response.number_of_arrivals; i++) {
      printf("Arrival %d: Line: %s, Destination: %s, Departure Time: %s, Departs in: %d min\n", 
        i + 1, 
        response.arrivals[i].line_id, 
        response.arrivals[i].destination, 
        response.arrivals[i].departure_time, 
        response.arrivals[i].departs_in_min);
    }
  } else {
    printf("Failed to get response for stop_id %s\n", test_stop_id);
  }
  
  return 0; 
}