# SSFHS — Simple Static File HTTP Server

A lightweight HTTP server written from scratch in C. Built as a learning project to understand how web servers work at the socket level — no frameworks, no dependencies beyond the C standard library and pthreads.

> Not quite nginx yet, but give it time.

---

## Features

- **Written in C** — raw POSIX sockets, manual HTTP parsing, zero external dependencies
- **Multi-threaded** — spawns a thread per connection to handle concurrent requests
- **Path traversal protection** — resolves paths with `realpath()` and rejects anything that escapes the server root
- **Dynamic content** — files can embed `<ssfhs-dyn>shell command</ssfhs-dyn>` tags; the server executes them and injects the output at response time
- **Protected file list** — specified files are never served or shown in directory listings
- **Custom error pages** — configurable 400, 403, 404, and 500 pages
- **Auto directory listing** — generates an index page when no index file is present
- **Request timeout** — uses `poll()` to enforce a configurable timeout on slow/incomplete requests
- **Structured logging** — logs timestamps, client IP, and User-Agent to a dedicated log file
- **Flexible configuration** — both a CLI and a plain-text config file

---

## Building

```bash
make
```

The binary is output to `build/ssfhs`. Three build modes are available:

| Mode | Description |
|------|-------------|
| `RELEASE` (default) | Optimized (`-O2`) |
| `DEBUG` | Debug symbols, AddressSanitizer + UBSan enabled |
| `NOASAN` | Debug symbols, sanitizers disabled |

```bash
make BUILD_MODE=DEBUG
```

---

## Usage

```
Usage: ssfhs [options]
  -p, --port [PORT]        Port to listen on (default: 8080)
  -d, --root-dir [DIR]     Root directory to serve files from (required)
  -l, --log-file [FILE]    Log output file (default: ssfhs.log)
  -c, --config-file [FILE] Config file path (required)
      --debug              Enable verbose debug output
```

---

## Configuration

SSFHS is configured through a plain-text file:

```
# Protect files from being served or listed
PROTECTED=ssfhs.conf
PROTECTED=README.md

# Custom error pages
400_PAGE=./400.html
403_PAGE=./403.html
404_PAGE=./404.html
500_PAGE=./500.html

# Custom index page
INDEX_PAGE=./index.html

# Files processed as dynamic (shell tag substitution)
DYNAMIC=index.html
DYNAMIC=api/server-status.json
```

### Dynamic Content

Files listed under `DYNAMIC` support server-side shell tag substitution. Any `<ssfhs-dyn>` block in the file is replaced with the stdout of the embedded command:

```html
<p>Server time: <ssfhs-dyn>date</ssfhs-dyn></p>
<p>Uptime: <ssfhs-dyn>uptime -p</ssfhs-dyn></p>
```

The request string and a connection ID are passed to the subprocess via environment variables.

---

## Examples

Ready-to-run example setups are in the [`examples/`](./examples/) directory:

- **`basic/`** — static site with custom error pages and a JS test suite for malformed requests
- **`minimal/`** — the smallest possible config to get started
- **`dynamic/`** — demonstrates dynamic tag substitution, a server status page, and API endpoints

## License

MIT — see [LICENSE](LICENSE).
