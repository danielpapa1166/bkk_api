# bkk_uds_server

UNIX domain socket server for BKK arrivals.

## CLI options

- `-k <path>`: read API key from file (first line). If omitted, `BKK_API_KEY` env var is used.
- `-f <seconds>`: cache freshness window.
- `-s <seconds>`: cache staleness window.
- `-l <count>`: max cache size.
- `-h`: print usage.

Rules:
- All numeric values must be positive integers.
- `staleness_seconds` must be greater than or equal to `freshness_seconds`.

Default values:
- freshness: 10
- staleness: 20
- max cache size: 100

## Example

```bash
./build/bin/bkk_uds_server -k /path/to/api_key.txt -f 15 -s 45 -l 500
```

## Notes

- If `-k` is provided, the key file is used.
- If `-k` is not provided, `BKK_API_KEY` is read from the environment.
- The server exits on invalid CLI values or missing API key.

## Known Limitations

**Cache keyed on `stop_id` only**

The cache does not account for the API key. If two clients query the same stop with different API keys, the server will serve the cached result from whichever key was used first. This is generally harmless in a single-user local setup but means a stale or invalid key will not trigger a fresh fetch as long as a valid cache entry exists.

**API key exposure over the socket**

The API key is sent from the client on every request as part of `bkk_uds_request_t`. Although UNIX domain sockets are local-only, any local process that has access to `/tmp/bkk_uds.sock` can observe the key. Restrict socket permissions after `bind` (e.g. `chmod 0600` / `fchmod`) to limit access to the owning user only.
