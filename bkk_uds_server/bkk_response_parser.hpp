#pragma once
#include <vector>
#include <string>
#include "bkk_api_arrival.h"

enum class ArrivalsParseStatus {
	Success = 0,
	SuccessWithWarnings,
	InvalidJson,
	RootNotObject,
	MissingDataObject,
	MissingEntryObject,
	StopTimesNotArray,
	EmptyStopTimes,
	NoValidStopTimes
};


static inline const char* parse_status_to_string(ArrivalsParseStatus status) {
  switch (status) {
	case ArrivalsParseStatus::Success:
	  return "Success";
	case ArrivalsParseStatus::SuccessWithWarnings:
	  return "SuccessWithWarnings";
	case ArrivalsParseStatus::InvalidJson:
	  return "InvalidJson";
	case ArrivalsParseStatus::RootNotObject:
	  return "RootNotObject";
	case ArrivalsParseStatus::MissingDataObject:
	  return "MissingDataObject";
	case ArrivalsParseStatus::MissingEntryObject:
	  return "MissingEntryObject";
	case ArrivalsParseStatus::StopTimesNotArray:
	  return "StopTimesNotArray";
	case ArrivalsParseStatus::EmptyStopTimes:
	  return "EmptyStopTimes";
	case ArrivalsParseStatus::NoValidStopTimes:
	  return "NoValidStopTimes";
	default:
	  return "UnknownStatus";    
  }
}


ArrivalsParseStatus parse_arrivals_response(
	const std::string& response_body,
	std::vector<Arrival>* const output_arrivals);
