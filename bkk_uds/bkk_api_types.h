#ifndef BKK_API_TYPES_H
#define BKK_API_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum bkk_api_status{
  Ok = 0,
  MissingApiKey,
  CurlInitFailed,
  CurlPerformFailed,
  InvalidApiKey, 
  HttpError,
  FetchArrivalsFailed,
  ArrivalsParseFailed,
  UnexpectedException
} bkk_api_status_t;

static inline const char* error_code_to_string(bkk_api_status_t code) {
  switch (code) {
    case Ok:
      return "Ok";
    case MissingApiKey:
      return "MissingApiKey";
    case CurlInitFailed:
      return "CurlInitFailed";
    case CurlPerformFailed:
      return "CurlPerformFailed";
    case InvalidApiKey:
      return "InvalidApiKey";
    case HttpError:
      return "HttpError";
    case FetchArrivalsFailed:
      return "FetchArrivalsFailed";
    case ArrivalsParseFailed:
      return "ArrivalsParseFailed";
    case UnexpectedException:
      return "UnexpectedException";
    default:
      return "UnknownBkkApiStatus";    
  }; 
}

#ifdef __cplusplus
}
#endif

#endif // BKK_API_TYPES_H