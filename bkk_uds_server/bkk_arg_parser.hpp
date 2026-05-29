#ifndef BKK_ARG_PARSER_HPP
#define BKK_ARG_PARSER_HPP


#include <string>

typedef struct {
  std::string api_key_file_path;
  int freshness_seconds;
  int staleness_seconds;
  int max_cache_size;
} arg_config_t;

typedef enum class parse_status {
  ok, 
  error, 
  help
} parse_status_t; 

parse_status_t parse_arguments(int argc, char* argv[], 
    arg_config_t * config_out);

#endif // BKK_ARG_PARSER_HPP