from typing import Optional, List, Dict, Any
from datetime import datetime
from dataclasses import dataclass
import logging

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)


@dataclass
class Arrival:
    """Data class for arrival information"""
    line: str
    destination: str
    departure_time: Optional[str] = None
    departs_in_min: Optional[int] = None
    timestamp: Optional[int] = None


def find_route_id_for_trip(trip_id: str, references: Dict[str, Any]) -> Optional[str]:
    """Helper function to find route ID for a given trip ID"""
    trips = references.get("trips", {})
    trip = trips.get(trip_id)
    if trip:
        return trip.get("routeId")
    return None

def find_route_name_for_route_id(route_id: str, references: Dict[str, Any]) -> Optional[str]:
    """Helper function to find route name for a given route ID"""
    routes = references.get("routes", {})
    route = routes.get(route_id)
    if route:
        return route.get("shortName") 
    return None


def parse_arrivals_response(response: Dict[str, Any]) -> List[Arrival]:
    # fetched successfully, now parse the response: 

    # print(json.dumps(response, indent=2))  # Debug: print raw response
    arrivals = []
    
    if not response.get("data") or not response["data"].get("entry"):
        logger.info(f"No schedule info for the given stop")
        return []

    apiTime_s = response["currentTime"] / 1000  # Convert ms to seconds
    
    try:
        entry = response["data"]["entry"]
        stop_times = entry.get("stopTimes", [])
        references = response["data"].get("references", {})
    except KeyError as e:
        logger.error(f"Failed to parse entry from response: {e}")
        return []


    
    # References are dictionaries keyed by ID (from OTP dialect)
    trips = references.get("trips", {})
    
    # Parse arrivals
    for stop_time in stop_times:
        trip_id = stop_time.get("tripId")
        route_id = find_route_id_for_trip(trip_id, references)
        route_name = find_route_name_for_route_id(route_id, references)

        try: 
            depertureTime = stop_time.get("predictedArrivalTime") or stop_time.get("arrivalTime")
        except Exception as e:
            print(f"Error parsing departure time for stop_time: {stop_time}")
            logger.error(f"Failed to parse departure time: {e}")
        
        trip = trips.get(trip_id, {})

        
        departs_in_min = int(round((depertureTime - apiTime_s) / 60)) \
            if depertureTime else None

        
        # Get destination
        destination = trip.get("tripHeadsign", "?")

        depertureTime_str = datetime.fromtimestamp(depertureTime).strftime("%H:%M") 
        
        arrival = Arrival(
            line=route_name or route_id or "n.a.",
            destination=destination,
            departure_time=depertureTime_str,
            departs_in_min=departs_in_min,
            timestamp=depertureTime
        )
        arrivals.append(arrival)
    
    return arrivals
