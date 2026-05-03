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

ArrivalsParseStatus parse_arrivals_response(
	const std::string& response_body,
	std::vector<Arrival>* const output_arrivals);
