#ifndef BKK_API_ARRIVAL_H
#define BKK_API_ARRIVAL_H

#ifdef __cplusplus
extern "C" {
#endif

#define ARRIVAL_LINE_MAX_LEN                 8
#define ARRIVAL_DESTINATION_MAX_LEN          64
#define ARRIVAL_DEPARTURE_TIME_MAX_LEN       16

typedef enum {
  VEHICLE_TYPE_UNKNOWN,
  VEHICLE_TYPE_BUS,
  VEHICLE_TYPE_TRAM,
  VEHICLE_TYPE_TROLLEYBUS,
  VEHICLE_TYPE_METRO,
  VEHICLE_TYPE_SUBURB_RAIL,
  VEHICLE_TYPE_RAIL,
  VEHICLE_TYPE_FERRY,
  VEHICLE_TYPE_CABLE_CAR,
  VEHICLE_TYPE_FUNICULAR,
  VEHICLE_TYPE_GONDOLA,
  VEHICLE_TYPE_COACH,
  VEHICLE_TYPE_BICYCLE,
  VEHICLE_TYPE_CAR,
  VEHICLE_TYPE_WALK,
  VEHICLE_TYPE_LOCAL,
  VEHICLE_TYPE_TRANSIT
} vehicle_type_t;

typedef struct {
  char line_id[ARRIVAL_LINE_MAX_LEN];
  vehicle_type_t vehicle_type;
  char destination[ARRIVAL_DESTINATION_MAX_LEN];
  char departure_time[ARRIVAL_DEPARTURE_TIME_MAX_LEN];
  int departs_in_min;
  long long timestamp;
} Arrival;

static inline const char* vehicle_type_to_string(vehicle_type_t type) {
  switch (type) {
    case VEHICLE_TYPE_BUS:          return "BUS";
    case VEHICLE_TYPE_TRAM:         return "TRAM";
    case VEHICLE_TYPE_TROLLEYBUS:   return "TROLLEYBUS";
    case VEHICLE_TYPE_METRO:        return "SUBWAY";
    case VEHICLE_TYPE_SUBURB_RAIL:  return "SUBURBAN_RAILWAY";
    case VEHICLE_TYPE_RAIL:         return "RAIL";
    case VEHICLE_TYPE_FERRY:        return "FERRY";
    case VEHICLE_TYPE_CABLE_CAR:    return "CABLE_CAR";
    case VEHICLE_TYPE_FUNICULAR:    return "FUNICULAR";
    case VEHICLE_TYPE_GONDOLA:      return "GONDOLA";
    case VEHICLE_TYPE_COACH:        return "COACH";
    case VEHICLE_TYPE_BICYCLE:      return "BICYCLE";
    case VEHICLE_TYPE_CAR:          return "CAR";
    case VEHICLE_TYPE_WALK:         return "WALK";
    case VEHICLE_TYPE_LOCAL:        return "LOCAL";
    case VEHICLE_TYPE_TRANSIT:      return "TRANSIT";
    case VEHICLE_TYPE_UNKNOWN:
    default:                        return "UNKNOWN";
  }
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif // BKK_API_ARRIVAL_H