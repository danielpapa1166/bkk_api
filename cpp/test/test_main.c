#include "bkk_uds_client.h"
#include <stdio.h>

int main() {
  bkk_uds_response_t response; 
  const int res = send_bkk_uds_query("1234", &response); 
  if(res == 0) {
    printf("Received response for stop_id 1234:\n");
    printf("Stop ID: %s\n", response.dummy_data);
  } else {
    printf("Failed to get response for stop_id 1234\n");
  }
  
  return 0; 
}