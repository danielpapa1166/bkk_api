#ifndef BKK_UDS_CLIENT_H
#define BKK_UDS_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bkk_uds_protocol.h"
#include "bkk_api_types.h"

typedef enum {
  client_OK = 0,
  client_InvalidArguments,
  client_SocketError, 
  client_ConnectionFailed,
  client_SendFailed,
  client_ReceiveFailed,
} bkk_client_status_t;

bkk_client_status_t send_bkk_uds_query(const char * stop_id, bkk_uds_response_t * response); 

#ifdef __cplusplus
} // extern "C"
#endif

#endif // BKK_UDS_CLIENT_H