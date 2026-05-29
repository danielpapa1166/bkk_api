#ifndef BKK_UDS_PROTOCOL_H
#define BKK_UDS_PROTOCOL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "bkk_api_arrival.h"
#include "bkk_api_types.h"

#define BKK_UDS_SOCKET_PATH "/tmp/bkk_uds.sock"
#define BKK_UDS_MAX_STOP_ID_LEN 64
#define BKK_UDS_MAX_ARRIVALS 16
#define BKK_UDS_MAX_KEY_LEN 256

typedef struct {
  char stop_id[BKK_UDS_MAX_STOP_ID_LEN]; 
  char api_key[BKK_UDS_MAX_KEY_LEN];
  size_t api_key_len;
} bkk_uds_request_t;

typedef struct {
  bkk_api_status_t status;
  int number_of_arrivals;
  Arrival arrivals[BKK_UDS_MAX_ARRIVALS]; 
} bkk_uds_response_t;

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // BKK_UDS_PROTOCOL_H