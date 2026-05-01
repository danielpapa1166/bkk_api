#ifndef BKK_UDS_PROTOCOL_H
#define BKK_UDS_PROTOCOL_H

#define BKK_UDS_SOCKET_PATH "/tmp/bkk_uds.sock"
#define BKK_UDS_MAX_STOP_ID_LEN 64


typedef struct {
  char stop_id[BKK_UDS_MAX_STOP_ID_LEN]; 
} bkk_uds_request_t;

typedef struct {
  char dummy_data[16]; 
} bkk_uds_response_t;


#endif // BKK_UDS_PROTOCOL_H