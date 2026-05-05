#ifndef BKK_UDS_CLIENT_H
#define BKK_UDS_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "bkk_uds_protocol.h"

int send_bkk_uds_query(const char * stop_id, bkk_uds_response_t * response); 

#ifdef __cplusplus
} // extern "C"
#endif

#endif // BKK_UDS_CLIENT_H