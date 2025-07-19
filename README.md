# httpClient

A lightweight HTTP client library for Windows built on the WinHTTP API. It supports GET, POST, PUT, DELETE, and PATCH requests and provides a simple interface for making HTTP requests in C.

## Features

- HTTP(S) requests: GET, POST, PUT, DELETE, PATCH
- Custom headers and request bodies
- Response parsing (status code, headers, body)
- URL encoding utility
- Configurable request timeouts
- Memory management utilities

## Requirements

- Windows platform
- WinHTTP library

## Installation

Clone the repository and include `http_client.c` and `http_client.h` in your project. Link against `winhttp.lib`.

```shell
git clone https://github.com/MohaIfk/httpClient.git
```

## Usage

### Basic GET Request

```c
#include "http_client.h"

int main() {
    http_client_t* client = http_client_create(NULL); // Default UA
    http_response_t* response = http_get(client, "https://example.com");

    printf("Status: %d\n", response->status_code);
    printf("Headers:\n%s\n", response->headers);
    printf("Body:\n%s\n", response->body);

    http_response_destroy(response);
    http_client_destroy(client);
    return 0;
}
```

### POST Request with Data

```c
http_response_t* response = http_post(
    client,
    "https://example.com/api",
    "{\"key\":\"value\"}",
    "application/json"
);
```

### Setting Timeout

```c
http_client_set_timeout(client, 10000); // 10 seconds
```

### URL Encoding

```c
char* encoded = http_url_encode("hello world!");
printf("%s\n", encoded);
free(encoded);
```

## API Reference

- `http_client_create(const char* user_agent)`
- `http_client_destroy(http_client_t*)`
- `http_get(http_client_t*, const char* url)`
- `http_post(http_client_t*, const char* url, const char* data, const char* content_type)`
- `http_put(...)`, `http_delete(...)`, etc.
- `http_response_destroy(http_response_t*)`
- `http_client_set_timeout(http_client_t*, int timeout_ms)`
- `http_url_encode(const char*)`

## License