#include "bkk_stop_utils.h"
#include "bkk_stop_list.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


int get_stop_list_size(void) {
  // test function 
  return sizeof(bkk_stop_list) / sizeof(bkk_stop_list[0]);
}

bkk_stop_stat_t find_stop_by_id(const char * stop_id, bkk_stop_t * stop_out) {
  if(stop_id == NULL) {
    return BKK_STOP_NOT_FOUND;
  }

  for(size_t i = 0; i < BKK_STOP_COUNT; i++) {
    if(strcmp(bkk_stop_list[i].stop_id, stop_id) == 0) {
      if(stop_out != NULL) {
        *stop_out = bkk_stop_list[i];
      }
      return BKK_STOP_FOUND;
    }
  }
  return BKK_STOP_NOT_FOUND;
}

bkk_stop_stat_t find_stop_by_name(const char * stop_name, bkk_stop_t * stop_out) {
  if(stop_name == NULL) {
    return BKK_STOP_NOT_FOUND;
  }

  for(size_t i = 0; i < BKK_STOP_COUNT; i++) {
    if(strcmp(bkk_stop_list[i].stop_name, stop_name) == 0) {
      if(stop_out != NULL) {
        *stop_out = bkk_stop_list[i];
      }
      return BKK_STOP_FOUND;
    }
  }
  return BKK_STOP_NOT_FOUND;
}

bkk_stop_stat_t find_stop_by_index(size_t index, bkk_stop_t * stop_out) {
  if(index < BKK_STOP_COUNT) {
    if(stop_out != NULL) {
      *stop_out = bkk_stop_list[index];
    }
    return BKK_STOP_FOUND;
  }
  return BKK_STOP_NOT_FOUND;
}



// the caller is responsible for freeing the allocated memory for indices_out
bkk_stop_stat_t find_stops_by_name_substring(
    const char * substring, size_t ** indices_out, size_t * count_out) {

  // double for loop for allocation
  if(substring == NULL || indices_out == NULL || count_out == NULL) {
    if(indices_out != NULL) {
      *indices_out = NULL;
    }
    if(count_out != NULL) {
      *count_out = 0;
    }
    return BKK_STOP_NOT_FOUND;
  }

  size_t count = 0; 

  // count mathing stops: 
  for(size_t i = 0; i < BKK_STOP_COUNT; i++) {
    if(strstr(bkk_stop_list[i].stop_name, substring) != NULL) {
      count++;
    }
  }

  if(count == 0) {
    *indices_out = NULL;
    *count_out = 0;
    return BKK_STOP_NOT_FOUND;
  }

  // alloc memory (to be free'd by the caller)
  size_t * indices = (size_t *)malloc(count * sizeof(size_t));
  if(indices == NULL) {
    *indices_out = NULL;
    *count_out = 0;
    return BKK_STOP_NOT_FOUND;
  }

  // collect indices of matching stops:
  size_t idx = 0;
  for(size_t i = 0; i < BKK_STOP_COUNT; i++) {
    if(strstr(bkk_stop_list[i].stop_name, substring) != NULL) {
      indices[idx++] = i;
    }
  }

  *indices_out = indices;
  *count_out = count;
  return BKK_STOP_FOUND;
}


bkk_stop_stat_t display_stop_list(size_t * indices, size_t count) {
  if(indices == NULL || count == 0) {
    return BKK_STOP_NOT_FOUND;
  }

  for(size_t i = 0; i < count; i++) {
    size_t idx = indices[i];
    if(idx < BKK_STOP_COUNT) {
      printf("Stop %zu: ID: %s, Name: %s\n", 
        idx, 
        bkk_stop_list[idx].stop_id, 
        bkk_stop_list[idx].stop_name);
    }
  }
  return BKK_STOP_FOUND;
}