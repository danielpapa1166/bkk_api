#ifndef BKK_API_ARRIVAL_H
#define BKK_API_ARRIVAL_H

#define ARRIVAL_LINE_MAX_LEN                 8
#define ARRIVAL_DESTINATION_MAX_LEN          64
#define ARRIVAL_DEPARTURE_TIME_MAX_LEN       16

typedef struct {
  char line_id[ARRIVAL_LINE_MAX_LEN];
  char destination[ARRIVAL_DESTINATION_MAX_LEN];
  char departure_time[ARRIVAL_DEPARTURE_TIME_MAX_LEN];
  int departs_in_min;
  long long timestamp;
} Arrival;


#endif // BKK_API_ARRIVAL_H