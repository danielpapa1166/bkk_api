#!/bin/bash
#
# stress_test.sh — Stress test for bkk_uds_server unbounded thread spawning.
#
# The server detaches a new std::thread for every accepted connection.
# This script fires a configurable number of concurrent UDS connections
# to expose runaway thread/process growth.
#
# Usage:
#   ./stress_test.sh [CONCURRENCY] [TOTAL_REQUESTS] [STOP_ID]
#
# Defaults:
#   CONCURRENCY     = 50   (parallel connections at a time)
#   TOTAL_REQUESTS  = 500  (total requests to send)
#   STOP_ID         = F02932
#
# Requirements: python3
#
# The API key is read from a key file (same convention as the server/client).
# Default key file path: ../key.txt (relative to this script).
#

SOCKET_PATH="/tmp/bkk_uds.sock"
SERVER_BIN="$(dirname "$0")/../build/bin/bkk_uds_server"

CONCURRENCY="${1:-50}"
TOTAL_REQUESTS="${2:-500}"
STOP_ID="${3:-F02615}"
KEY_FILE="${4:-$(dirname "$0")/../key.txt}"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# ── helpers ──────────────────────────────────────────────────────────────────

require_cmd() {
  if ! command -v "$1" &>/dev/null; then
    echo -e "${RED}ERROR: '$1' is required but not found.${NC}"
    exit 1
  fi
}

server_pid() {
  pgrep -x bkk_uds_server 2>/dev/null | head -1
}

thread_count() {
  local pid="$1"
  # /proc/<pid>/status Threads field
  awk '/^Threads:/{print $2}' /proc/"$pid"/status 2>/dev/null || echo "?"
}

# ── preflight ────────────────────────────────────────────────────────────────

require_cmd python3

SRV_PID=$(server_pid)
if [[ -z "$SRV_PID" ]]; then
  echo -e "${YELLOW}WARNING: bkk_uds_server does not appear to be running.${NC}"
  echo "Start it first, e.g.:"
  echo "  $SERVER_BIN -k key.txt &"
  exit 1
fi

if [[ ! -f "$KEY_FILE" ]]; then
  echo -e "${RED}ERROR: API key file not found: $KEY_FILE${NC}"
  echo "Pass the key file as the 4th argument: ./stress_test.sh 50 500 F02932 /path/to/key.txt"
  exit 1
fi
API_KEY=$(tr -d '[:space:]' < "$KEY_FILE")

echo "======================================================"
echo " bkk_uds_server stress test"
echo "======================================================"
echo " Server PID   : $SRV_PID"
echo " Socket       : $SOCKET_PATH"
echo " Concurrency  : $CONCURRENCY"
echo " Total reqs   : $TOTAL_REQUESTS"
echo " Stop ID      : $STOP_ID"
echo " Key file     : $KEY_FILE"
echo "======================================================"
echo ""

THREADS_BEFORE=$(thread_count "$SRV_PID")
echo "Threads before: $THREADS_BEFORE"
echo ""

# ── build binary request payload ─────────────────────────────────────────────
# bkk_uds_request_t layout (from bkk_uds_protocol.h):
#   char stop_id[64]   – offset 0
#   char api_key[256]  – offset 64
# Total: 320 bytes

REQUEST_BIN=$(mktemp)
python3 - "$STOP_ID" "$API_KEY" "$REQUEST_BIN" <<'PYEOF'
import sys, struct

STOP_ID  = sys.argv[1].encode()
KEY      = sys.argv[2].encode()
STOP_BUF = STOP_ID[:63].ljust(64,  b'\x00')
KEY_BUF  = KEY[:255].ljust(256, b'\x00')

with open(sys.argv[3], 'wb') as f:
    f.write(STOP_BUF + KEY_BUF)
PYEOF

REQUEST_SIZE=$(wc -c < "$REQUEST_BIN")
echo "Request payload: $REQUEST_SIZE bytes"

# ── fire requests ─────────────────────────────────────────────────────────────
#
# Strategy: open all CONCURRENCY connections simultaneously and hold them open
# (do NOT wait for a response) so server threads are alive during sampling.
# After measuring the peak, release all connections and collect the totals.
#

echo "Sending $TOTAL_REQUESTS requests ($CONCURRENCY in parallel)..."
START_TIME=$(date +%s%3N)

RESULT_FILE=$(mktemp)

python3 - "$SOCKET_PATH" "$REQUEST_BIN" "$CONCURRENCY" "$TOTAL_REQUESTS" "$SRV_PID" "$RESULT_FILE" <<'PYEOF'
import socket, sys, threading, time, os

SOCK_PATH    = sys.argv[1]
REQ_FILE     = sys.argv[2]
CONCURRENCY  = int(sys.argv[3])
TOTAL        = int(sys.argv[4])
SERVER_PID   = sys.argv[5]
RESULT_FILE  = sys.argv[6]

payload = open(REQ_FILE, 'rb').read()

passed = 0
failed = 0
lock   = threading.Lock()
peak_threads = 0

def thread_count():
    try:
        with open(f'/proc/{SERVER_PID}/status') as f:
            for line in f:
                if line.startswith('Threads:'):
                    return int(line.split()[1])
    except Exception:
        pass
    return 0

def send_and_hold(release_event):
    global passed, failed
    s = socket.socket(socket.AF_UNIX, socket.SOCK_SEQPACKET)
    s.settimeout(3)
    try:
        s.connect(SOCK_PATH)
        s.sendall(payload)
        # hold the connection open until signalled — keeps server thread alive
        release_event.wait(timeout=5)
        s.recv(4096)
        s.close()
        with lock:
            passed += 1
    except Exception:
        try: s.close()
        except: pass
        with lock:
            failed += 1

batches = TOTAL // CONCURRENCY
remainder = TOTAL % CONCURRENCY

for batch_num in range(batches + (1 if remainder else 0)):
    count = CONCURRENCY if batch_num < batches else remainder
    if count == 0:
        break

    release = threading.Event()
    threads = []
    for _ in range(count):
        t = threading.Thread(target=send_and_hold, args=(release,), daemon=True)
        t.start()
        threads.append(t)

    # give threads time to all connect before sampling
    time.sleep(0.3)

    tc = thread_count()
    with lock:
        if tc > peak_threads:
            peak_threads = tc

    done_so_far = batch_num * CONCURRENCY + count
    print(f"  Batch {batch_num+1}: {done_so_far}/{TOTAL}  |  server threads: {tc}", flush=True)

    # release all held connections
    release.set()
    for t in threads:
        t.join(timeout=6)

with open(RESULT_FILE, 'w') as f:
    f.write(f'{passed}\n{failed}\n{peak_threads}\n')
PYEOF

END_TIME=$(date +%s%3N)
ELAPSED=$(( END_TIME - START_TIME ))

PASS=$(sed -n '1p' "$RESULT_FILE")
FAIL=$(sed -n '2p' "$RESULT_FILE")
THREADS_PEAK=$(sed -n '3p' "$RESULT_FILE")
rm -f "$RESULT_FILE"

echo ""
echo ""

# ── results ───────────────────────────────────────────────────────────────────

echo "======================================================"
echo " Results"
echo "======================================================"
printf " Requests sent    : %d\n" "$TOTAL_REQUESTS"
printf " Connections OK   : %d\n" "$PASS"
printf " Connections FAIL : %d\n" "$FAIL"
printf " Elapsed          : %d ms\n" "$ELAPSED"
echo ""
printf " Threads before   : %s\n" "$THREADS_BEFORE"
printf " Threads peak     : %s\n" "$THREADS_PEAK"

if [[ "$THREADS_BEFORE" =~ ^[0-9]+$ ]] && [[ "$THREADS_PEAK" =~ ^[0-9]+$ ]]; then
  DELTA=$(( THREADS_PEAK - THREADS_BEFORE ))
  printf " Thread delta     : %+d\n" "$DELTA"
  echo ""
  if (( DELTA > CONCURRENCY )); then
    echo -e "${RED}FAIL: Thread count grew by $DELTA — server is not bounding thread creation.${NC}"
  elif (( DELTA > 0 )); then
    echo -e "${YELLOW}WARN: Thread count grew by $DELTA (some threads may still be finishing).${NC}"
  else
    echo -e "${GREEN}OK: Thread count did not grow beyond baseline.${NC}"
  fi
fi

echo "======================================================"
echo ""
printf " Threads before   : %s\n" "$THREADS_BEFORE"
printf " Threads after    : %s\n" "$THREADS_AFTER"

if [[ "$THREADS_BEFORE" =~ ^[0-9]+$ ]] && [[ "$THREADS_AFTER" =~ ^[0-9]+$ ]]; then
  DELTA=$(( THREADS_AFTER - THREADS_BEFORE ))
  printf " Thread delta     : %+d\n" "$DELTA"
  echo ""
  if (( DELTA > CONCURRENCY )); then
    echo -e "${RED}FAIL: Thread count grew by $DELTA — server is not bounding thread creation.${NC}"
  elif (( DELTA > 0 )); then
    echo -e "${YELLOW}WARN: Thread count grew by $DELTA (some threads may still be finishing).${NC}"
  else
    echo -e "${GREEN}OK: Thread count did not grow beyond baseline.${NC}"
  fi
fi

echo "======================================================"

rm -f "$REQUEST_BIN"
