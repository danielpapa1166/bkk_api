#include "bkk_arg_parser.hpp"
#include <cerrno>
#include <cstdlib>
#include <rbuflogd/logger.h>
#include <climits>
#include <getopt.h>




static bool parse_positive_int(const char * arg, int * out_value) {
  if(arg == nullptr || out_value == nullptr) {
    return false;
  }

  errno = 0;
  char * endptr = nullptr;
  long value = strtol(arg, &endptr, 10);

  if(errno != 0 || endptr == arg || *endptr != '\0') {
    return false;
  }
  if(value <= 0 || value > INT_MAX) {
    return false;
  }

  *out_value = (int)value;
  return true;
}


parse_status_t parse_arguments(int argc, char* argv[], arg_config_t * config_out) {
  int freshness_seconds = 10;
  int staleness_seconds = 20;
  int max_cache_size = 100;

  int opt;
  while((opt = getopt(argc, argv, "f:s:l:h")) != -1) {
    switch(opt) {
      case 'f':
        if(!parse_positive_int(optarg, &freshness_seconds)) {
          log_error("Init", "Invalid freshness seconds");
          return parse_status::error;
        }
        break;
      case 's':
        if(!parse_positive_int(optarg, &staleness_seconds)) {
          log_error("Init", "Invalid staleness seconds");
          return parse_status::error;
        }
        break;
      case 'l':
        if(!parse_positive_int(optarg, &max_cache_size)) {
          log_error("Init", "Invalid max cache size");
          return parse_status::error;
        }
        break;
      case 'h':
        return parse_status::help;
      default:
        return parse_status::error;
    }
  }

  if(staleness_seconds < freshness_seconds) {
    log_error("Init", "Invalid cache config: staleness must be >= freshness");
    return parse_status::error;
  }

  config_out->freshness_seconds = freshness_seconds;
  config_out->staleness_seconds = staleness_seconds;
  config_out->max_cache_size = max_cache_size;

  return parse_status::ok;
}
