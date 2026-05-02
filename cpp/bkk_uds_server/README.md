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
