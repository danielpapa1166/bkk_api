"""
Example application demonstrating BKK/FUTAR API usage

"""

import os
import sys
from dotenv import load_dotenv
from bkk_client import BKKClient
from bkk_response_parser import parse_arrivals_response, Arrival
from bkk_station_list import stations

def print_section(title: str):
    """Print a formatted section header"""
    print(f"\n{'='*60}")
    print(f"  {title}")
    print(f"{'='*60}\n")


def main(station_name = None):
    load_dotenv()
    api_key = os.getenv("BKK_API_KEY")
    client = BKKClient(api_key=api_key)
    
    try:
        print_section("Station Test")
        if station_name:
            stop_id = None
            for sid, name in stations.items():
                if station_name.lower() in name.lower():
                    stop_id = sid
                    break
            
            if not stop_id:
                print(f"Station '{station_name}' not found. Available stations:")
                for sid, name in stations.items():
                    print(f"  {sid}: {name}")
                return

        server_response = client.get_arrivals_for_stop(stop_id)

        arrivals = parse_arrivals_response(server_response)

        if arrivals:
            for i, arr in enumerate(arrivals, 1):
                print(f"🚌 {arr.line}:\t{arr.departure_time} "
                    f"({arr.departs_in_min}) \t→ {arr.destination} \n")
                
        else:
            print("No arrivals found for this stop (might be outside operating hours)\n")
        
    
    except Exception as e:
        print(f"❌ Error: {e}")
        import traceback
        traceback.print_exc()
    finally:
        client.close()



if __name__ == "__main__":
    station_name = sys.argv[1] if len(sys.argv) > 1 else None
    main(station_name=station_name)
