#ifndef BKK_API_TYPES_H
#define BKK_API_TYPES_H


namespace bkk_api {

enum class ErrorCode {
  Ok = 0,
  MissingApiKey,
  CurlInitFailed,
  CurlPerformFailed,
  InvalidApiKey, 
  HttpError,
  FetchArrivalsFailed,
  ArrivalsParseFailed,
  UnexpectedException
};

static inline const char* error_code_to_string(ErrorCode code) {
  switch (code) {
    case ErrorCode::Ok:
      return "Ok";
    case ErrorCode::MissingApiKey:
      return "MissingApiKey";
    case ErrorCode::CurlInitFailed:
      return "CurlInitFailed";
    case ErrorCode::CurlPerformFailed:
      return "CurlPerformFailed";
    case ErrorCode::InvalidApiKey:
      return "InvalidApiKey";
    case ErrorCode::HttpError:          
      return "HttpError";
    case ErrorCode::FetchArrivalsFailed:
      return "FetchArrivalsFailed";
    case ErrorCode::ArrivalsParseFailed:
      return "ArrivalsParseFailed";
    case ErrorCode::UnexpectedException:
      return "UnexpectedException";
    default:
      return "UnknownErrorCode";    
  }; 
}


} // namespace bkk_api



#endif // BKK_API_TYPES_H