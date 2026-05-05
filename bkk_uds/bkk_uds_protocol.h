#ifndef BKK_UDS_PROTOCOL_H
#define BKK_UDS_PROTOCOL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bkk_api_arrival.h"

#define BKK_UDS_SOCKET_PATH "/tmp/bkk_uds.sock"
#define BKK_UDS_MAX_STOP_ID_LEN 64
#define BKK_UDS_MAX_ARRIVALS 16


typedef struct {
  char stop_id[BKK_UDS_MAX_STOP_ID_LEN]; 
} bkk_uds_request_t;

typedef struct {
  int number_of_arrivals;
  Arrival arrivals[BKK_UDS_MAX_ARRIVALS]; 
} bkk_uds_response_t;

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // BKK_UDS_PROTOCOL_H