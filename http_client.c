#include "http_client.h"

// Convert char to wchar_t
wchar_t* char_to_wchar(const char* str) {
    if (!str) return NULL;

    const int len = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
    if (len == 0) return NULL;

    wchar_t* wstr = malloc(len * sizeof(wchar_t));
    if (!wstr) return NULL;

    MultiByteToWideChar(CP_UTF8, 0, str, -1, wstr, len);
    return wstr;
}

// Convert wchar_t to char
char* wchar_to_char(const wchar_t* wstr) {
    if (!wstr) return NULL;

    int len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    if (len == 0) return NULL;

    char* str = malloc(len);
    if (!str) return NULL;

    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str, len, NULL, NULL);
    return str;
}

// Create HTTP client
http_client_t* http_client_create(const char* user_agent) {
    http_client_t* client = malloc(sizeof(http_client_t));
    if (!client) return NULL;

    memset(client, 0, sizeof(http_client_t));

    // Set default user agent
    const char* ua = user_agent ? user_agent : "WinHTTP-Client/1.0";
    client->user_agent = char_to_wchar(ua);
    if (!client->user_agent) {
        free(client);
        return NULL;
    }

    // Initialize WinHTTP session
    client->session = WinHttpOpen(client->user_agent,
                                  WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                  WINHTTP_NO_PROXY_NAME,
                                  WINHTTP_NO_PROXY_BYPASS,
                                  0);

    if (!client->session) {
        free(client->user_agent);
        free(client);
        return NULL;
    }

    client->timeout_ms = 30000; // 30 seconds default

    // Set timeouts
    WinHttpSetTimeouts(client->session,
                       client->timeout_ms,  // resolve timeout
                       client->timeout_ms,  // connect timeout
                       client->timeout_ms,  // send timeout
                       client->timeout_ms); // receive timeout

    return client;
}

// Destroy HTTP client
void http_client_destroy(http_client_t* client) {
    if (!client) return;

    if (client->connection) {
        WinHttpCloseHandle(client->connection);
    }

    if (client->session) {
        WinHttpCloseHandle(client->session);
    }

    if (client->user_agent) {
        free(client->user_agent);
    }

    free(client);
}

// Set timeout
void http_client_set_timeout(http_client_t* client, int timeout_ms) {
    if (!client) return;

    client->timeout_ms = timeout_ms;

    if (client->session) {
        WinHttpSetTimeouts(client->session,
                           timeout_ms,
                           timeout_ms,
                           timeout_ms,
                           timeout_ms);
    }
}

// Parse URL components
typedef struct {
    wchar_t* scheme;
    wchar_t* host;
    int port;
    wchar_t* path;
} url_components_t;

void free_url_components(url_components_t* components) {
    if (!components) return;

    free(components->scheme);
    free(components->host);
    free(components->path);
    free(components);
}

url_components_t *parse_url(const char *url) {
    wchar_t* wurl = char_to_wchar(url);
    if (!wurl) return NULL;

    URL_COMPONENTS urlComp;
    memset(&urlComp, 0, sizeof(urlComp));
    urlComp.dwStructSize = sizeof(urlComp);

    // Initial dummy lengths to trigger correct size filling
    urlComp.dwSchemeLength = 1;
    urlComp.dwHostNameLength = 1;
    urlComp.dwUrlPathLength = 1;

    if (!WinHttpCrackUrl(wurl, 0, 0, &urlComp)) {
        free(wurl);
        return NULL;
    }

    url_components_t* components = malloc(sizeof(url_components_t));
    if (!components) {
        free(wurl);
        return NULL;
    }

    // Allocate dynamic buffers based on reported sizes
    components->scheme = malloc((urlComp.dwSchemeLength + 1) * sizeof(wchar_t));
    components->host   = malloc((urlComp.dwHostNameLength + 1) * sizeof(wchar_t));
    components->path   = malloc((urlComp.dwUrlPathLength + 1) * sizeof(wchar_t));

    if (!components->scheme || !components->host || !components->path) {
        free_url_components(components);
        free(wurl);
        return NULL;
    }

    // Second pass
    URL_COMPONENTS urlComp2;
    memset(&urlComp2, 0, sizeof(urlComp2));
    urlComp2.dwStructSize = sizeof(urlComp2);

    urlComp2.lpszScheme = components->scheme;
    urlComp2.dwSchemeLength = urlComp.dwSchemeLength + 1;
    urlComp2.lpszHostName = components->host;
    urlComp2.dwHostNameLength = urlComp.dwHostNameLength + 1;
    urlComp2.lpszUrlPath = components->path;
    urlComp2.dwUrlPathLength = urlComp.dwUrlPathLength + 1;

    if (!WinHttpCrackUrl(wurl, 0, 0, &urlComp2)) {
        free_url_components(components);
        free(wurl);
        return NULL;
    }

    components->port = urlComp2.nPort;
    free(wurl);
    return components;
}

// Get method name as wide string
const wchar_t* get_method_name(http_method_t method) {
    switch (method) {
        case HTTP_GET: return L"GET";
        case HTTP_POST: return L"POST";
        case HTTP_PUT: return L"PUT";
        case HTTP_DELETE: return L"DELETE";
        case HTTP_PATCH: return L"PATCH";
        default: return L"GET";
    }
}

// Main HTTP request function
http_response_t* http_request(http_client_t* client, http_request_t* request) {
    if (!client || !request || !request->url) return NULL;

    // Parse URL
    url_components_t* url_comp = parse_url(request->url);
    if (!url_comp) return NULL;

    // Create connection
    HINTERNET connection = WinHttpConnect(client->session,
                                          url_comp->host,
                                          url_comp->port,
                                          0);

    if (!connection) {
        free_url_components(url_comp);
        return NULL;
    }

    // Determine if HTTPS
    DWORD flags = 0;
    if (_wcsicmp(url_comp->scheme, L"https") == 0) {
        flags = WINHTTP_FLAG_SECURE;
    }

    // Create request
    HINTERNET http_request = WinHttpOpenRequest(connection,
                                                get_method_name(request->method),
                                                url_comp->path,
                                                NULL,
                                                WINHTTP_NO_REFERER,
                                                WINHTTP_DEFAULT_ACCEPT_TYPES,
                                                flags);

    free_url_components(url_comp);

    if (!http_request) {
        WinHttpCloseHandle(connection);
        return NULL;
    }

    // Add headers if provided
    wchar_t* wheaders = NULL;
    if (request->headers) {
        wheaders = char_to_wchar(request->headers);
        if (wheaders) {
            WinHttpAddRequestHeaders(http_request,
                                     wheaders,
                                     -1,
                                     WINHTTP_ADDREQ_FLAG_ADD);
        }
    }

    // Send request
    BOOL result = WinHttpSendRequest(http_request,
                                     WINHTTP_NO_ADDITIONAL_HEADERS,
                                     0,
                                     request->body,
                                     request->body_length,
                                     request->body_length,
                                     0);

    if (wheaders) free(wheaders);

    if (!result) {
        WinHttpCloseHandle(http_request);
        WinHttpCloseHandle(connection);
        return NULL;
    }

    // Receive response
    if (!WinHttpReceiveResponse(http_request, NULL)) {
        WinHttpCloseHandle(http_request);
        WinHttpCloseHandle(connection);
        return NULL;
    }

    // Create response structure
    http_response_t* response = malloc(sizeof(http_response_t));
    if (!response) {
        WinHttpCloseHandle(http_request);
        WinHttpCloseHandle(connection);
        return NULL;
    }

    memset(response, 0, sizeof(http_response_t));

    // Get status code
    DWORD status_code_size = sizeof(response->status_code);
    WinHttpQueryHeaders(http_request,
                        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX,
                        &response->status_code,
                        &status_code_size,
                        WINHTTP_NO_HEADER_INDEX);

    // Get headers
    DWORD headers_size = 0;
    WinHttpQueryHeaders(http_request,
                        WINHTTP_QUERY_RAW_HEADERS_CRLF,
                        WINHTTP_HEADER_NAME_BY_INDEX,
                        NULL,
                        &headers_size,
                        WINHTTP_NO_HEADER_INDEX);

    if (headers_size > 0) {
        wchar_t* wheaders_raw = malloc(headers_size);
        if (wheaders_raw) {
            if (WinHttpQueryHeaders(http_request,
                                    WINHTTP_QUERY_RAW_HEADERS_CRLF,
                                    WINHTTP_HEADER_NAME_BY_INDEX,
                                    wheaders_raw,
                                    &headers_size,
                                    WINHTTP_NO_HEADER_INDEX)) {
                response->headers = wchar_to_char(wheaders_raw);
                response->headers_length = strlen(response->headers);
            }
            free(wheaders_raw);
        }
    }

    // Read response body
    DWORD bytes_available = 0;
    char* body_buffer = NULL;
    size_t total_size = 0;

    do {
        // Check how much data is available
        if (!WinHttpQueryDataAvailable(http_request, &bytes_available)) {
            break;
        }

        if (bytes_available == 0) break;

        // Allocate space for the data
        char* temp_buffer = realloc(body_buffer, total_size + bytes_available + 1);
        if (!temp_buffer) {
            free(body_buffer);
            body_buffer = NULL;
            break;
        }
        body_buffer = temp_buffer;

        // Read the data
        DWORD bytes_read = 0;
        if (!WinHttpReadData(http_request,
                             body_buffer + total_size,
                             bytes_available,
                             &bytes_read)) {
            break;
        }

        total_size += bytes_read;

    } while (bytes_available > 0);

    if (body_buffer) {
        body_buffer[total_size] = '\0';
        response->body = body_buffer;
        response->body_length = total_size;
    }

    // Cleanup
    WinHttpCloseHandle(http_request);
    WinHttpCloseHandle(connection);

    return response;
}

// Convenience function for GET requests
http_response_t* http_get(http_client_t* client, const char* url) {
    http_request_t request = {0};
    request.method = HTTP_GET;
    request.url = url;

    return http_request(client, &request);
}

// Convenience function for POST requests
http_response_t* http_post(http_client_t* client, const char* url, const char* data, const char* content_type) {
    http_request_t request = {0};
    request.method = HTTP_POST;
    request.url = url;

    if (data) {
        request.body = (char*)data;
        request.body_length = strlen(data);
    }

    // Build headers with content type
    if (content_type) {
        int header_len = strlen("Content-Type: ") + strlen(content_type) + 3; // +3 for \r\n\0
        char* headers = malloc(header_len);
        if (headers) {
            snprintf(headers, header_len, "Content-Type: %s\r\n", content_type);
            request.headers = headers;
        }
    }

    http_response_t* response = http_request(client, &request);

    if (request.headers) {
        free(request.headers);
    }

    return response;
}

// Convenience function for PUT requests
http_response_t* http_put(http_client_t* client, const char* url, const char* data, const char* content_type) {
    http_request_t request = {0};
    request.method = HTTP_PUT;
    request.url = (char*)url;

    if (data) {
        request.body = (char*)data;
        request.body_length = strlen(data);
    }

    // Build headers with content type
    if (content_type) {
        int header_len = strlen("Content-Type: ") + strlen(content_type) + 3;
        char* headers = malloc(header_len);
        if (headers) {
            snprintf(headers, header_len, "Content-Type: %s\r\n", content_type);
            request.headers = headers;
        }
    }

    http_response_t* response = http_request(client, &request);

    if (request.headers) {
        free(request.headers);
    }

    return response;
}

// Convenience function for DELETE requests
http_response_t* http_delete(http_client_t* client, const char* url) {
    http_request_t request = {0};
    request.method = HTTP_DELETE;
    request.url = (char*)url;

    return http_request(client, &request);
}

// Destroy response
void http_response_destroy(http_response_t* response) {
    if (!response) return;

    if (response->headers) {
        free(response->headers);
    }

    if (response->body) {
        free(response->body);
    }

    free(response);
}

// URL encode utility function
char* http_url_encode(const char* str) {
    if (!str) return NULL;

    const char* hex_chars = "0123456789ABCDEF";
    size_t len = strlen(str);
    char* encoded = malloc(len * 3 + 1); // worst case: every char needs encoding
    if (!encoded) return NULL;

    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)str[i];

        if ((c >= 'A' && c <= 'Z') ||
            (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') ||
            c == '-' || c == '_' || c == '.' || c == '~') {
            encoded[j++] = c;
        } else {
            encoded[j++] = '%';
            encoded[j++] = hex_chars[c >> 4];
            encoded[j++] = hex_chars[c & 0x0F];
        }
    }

    encoded[j] = '\0';
    return encoded; // Negligible Waste: The worst-case buffer size is already quite small in absolute terms (at most 3x the input length) no need for realloc(encoded, j + 1)
}