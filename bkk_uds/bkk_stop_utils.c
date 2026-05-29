#include "bkk_stop_utils.h"
#include "bkk_stop_list.h"
#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


api_key_read_stat_t read_api_key_from_file(const char * path, 
    char ** key_out, size_t * key_out_size) {

  if(path == NULL || key_out == NULL || key_out_size == NULL) {
    return API_KEY_READ_ERROR;
  }


  FILE * infile = fopen(path, "r");
  if(infile == NULL) {
    return API_KEY_READ_ERROR;
  }

  fseek(infile, 0, SEEK_END);
  long file_size = ftell(infile);
  if(file_size < 0) {
    fclose(infile);
    return API_KEY_READ_ERROR;
  }

  char * buffer = (char *)malloc(file_size + 1);
  if(buffer == NULL) {
    fclose(infile);
    return API_KEY_READ_ERROR;
  }

  fseek(infile, 0, SEEK_SET);

  const size_t read_size = fread(buffer, 1, 
    file_size, infile);

  if(read_size != (size_t)file_size) {
    free(buffer);
    fclose(infile);
    return API_KEY_READ_ERROR;
  }
  buffer[file_size] = '\0'; // null-terminate the buffer
  fclose(infile);


  // trim leading/trailing whitespace so newline-terminated files work out of the box
  size_t begin = 0;

  while(begin < file_size && isspace((unsigned char)buffer[begin])) {
    begin++;
  }

  size_t end = file_size;
  while(end > begin && isspace((unsigned char)buffer[end - 1])) {
    end--;
  }

  size_t key_length = end - begin;

  char * key_no_whitspace = (char *)malloc(key_length + 1);
  if(key_no_whitspace == NULL) {
    free(buffer);
    return API_KEY_READ_ERROR;
  }
  memcpy(key_no_whitspace, buffer + begin, key_length);
  free(buffer);
  key_no_whitspace[key_length] = '\0';
  *key_out = key_no_whitspace;
  *key_out_size = key_length;
    
  return API_KEY_READ_OK;
}


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