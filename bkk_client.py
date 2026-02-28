"""
Budapest Public Transport (BKK/FUTAR) API Client
Fetches real-time transit information from FUTAR API

special thanks to: https://github.com/mefiblogger/KoviBusz
"""

import requests
import json
from typing import Optional, List, Dict, Any
from datetime import datetime
from dataclasses import dataclass
import logging

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)



class BKKClient:
    """
    Client for BKK Budapest Public Transport API (FUTAR)
    Uses the correct FUTAR OTP endpoint that actually works
    """
    
    # Correct FUTAR API endpoint (from KoviBusz project)
    BASE_URL = "https://futar.bkk.hu/api/query/v1/ws/otp/api/where"
    
    def __init__(self, api_key: Optional[str] = None, timeout: int = 10):
        """
        Initialize BKK API Client
        
        Args:
            api_key: Your BKK/FUTAR API key
            timeout: Request timeout in seconds
        """
        self.api_key = api_key
        self.timeout = timeout
        self.session = requests.Session()
        
        headers = {
            "User-Agent": "BKKClientPython/1.0",
            "Accept": "application/json"
        }
        
        self.session.headers.update(headers)
    
    
    def _make_request(
        self, 
        endpoint: str, 
        params: Optional[Dict[str, Any]] = None
    ) -> Dict[str, Any]:
        """
        Make HTTP request to FUTAR API
        
        Args:
            endpoint: API endpoint path
            params: Query parameters
            
        Returns:
            Response JSON data
        """
        url = f"{self.BASE_URL}/{endpoint}"
        
        # Add API key to params if provided
        if params is None:
            params = {}
        if self.api_key:
            params["key"] = self.api_key
        
        try:
            logger.info(f"Making request to: {url}")
            response = self.session.get(
                url,
                params=params,
                timeout=self.timeout
            )
            response.raise_for_status()
            return response.json()
        
        except requests.RequestException as e:
            logger.error(f"API request failed: {e}")
            raise
    
    def get_arrivals_for_stop(
        self,
        stop_id: str):
        """
        Get arrivals and departures for a specific stop
        Args:
            stop_id: The stop identifier (without BKK_ prefix)
        Returns:
            List of Arrival objects
        """
        try:
            # Format the stop ID with BKK prefix if not already present
            if not stop_id.startswith("BKK_"):
                stop_id = f"BKK_{stop_id}"
            
            params = {
                "stopId": stop_id,
                "onlyDepartures": int(True),
                "minutesBefore": 0, 
                "minutesAfter": 30
            }
            
            response = self._make_request(
                "arrivals-and-departures-for-stop.json",
                params=params
            )
        except Exception as e:
            logger.error(f"Failed to fetch arrivals for stop {stop_id}: {e}")
            return []

        return response
        
    
 
    
    def close(self):
        """Close the session"""
        self.session.close()