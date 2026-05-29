#ifndef BKK_STOP_UTILS_H
#define BKK_STOP_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include "bkk_stop_list.h"

typedef enum {
  BKK_STOP_NOT_FOUND = -1,
  BKK_STOP_FOUND = 0,
} bkk_stop_stat_t; 

typedef enum {
  API_KEY_READ_OK,
  API_KEY_READ_ERROR,
} api_key_read_stat_t;

api_key_read_stat_t read_api_key_from_file(const char * path, char ** key_out);
int get_stop_list_size(void); 
bkk_stop_stat_t find_stop_by_id(
  const char * stop_id, bkk_stop_t * stop_out);
bkk_stop_stat_t find_stop_by_name(
  const char * stop_name, bkk_stop_t * stop_out);
bkk_stop_stat_t find_stop_by_index(
  size_t index, bkk_stop_t * stop_out);
bkk_stop_stat_t find_stops_by_name_substring(
  const char * substring, size_t ** indices_out, size_t * count_out);

bkk_stop_stat_t display_stop_list(size_t * indices, size_t count);

#ifdef __cplusplus
} // extern "C" 
#endif // __cplusplus

#endif // BKK_STOP_UTILS_H