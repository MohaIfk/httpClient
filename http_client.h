#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <windows.h>
#include <winhttp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "winhttp.lib")

// HTTP Methods
typedef enum {
    HTTP_GET,
    HTTP_POST,
    HTTP_PUT,
    HTTP_DELETE,
    HTTP_PATCH
} http_method_t;

// HTTP Response structure
typedef struct {
    int status_code;
    char* headers;
    char* body;
    size_t body_length;
    size_t headers_length;
} http_response_t;

// HTTP Client structure
typedef struct {
    HINTERNET session;
    HINTERNET connection;
    wchar_t* user_agent;
    int timeout_ms;
} http_client_t;

// HTTP Request structure
typedef struct {
    http_method_t method;
    const char* url;
    char* headers;
    char* body;
    size_t body_length;
} http_request_t;

// Function declarations
http_client_t* http_client_create(const char* user_agent);
void http_client_destroy(http_client_t* client);
void http_client_set_timeout(http_client_t* client, int timeout_ms);

http_response_t* http_request(http_client_t* client, http_request_t* request);
http_response_t* http_get(http_client_t* client, const char* url);
http_response_t* http_post(http_client_t* client, const char* url, const char* data, const char* content_type);
http_response_t* http_put(http_client_t* client, const char* url, const char* data, const char* content_type);
http_response_t* http_delete(http_client_t* client, const char* url);

void http_response_destroy(http_response_t* response);

// Utility functions
char* http_url_encode(const char* str);
wchar_t* char_to_wchar(const char* str);
char* wchar_to_char(const wchar_t* wstr);

#endif // HTTP_CLIENT_H