#include <stdio.h>

#include "http_client.h"

int main() {
    printf("HTTP Client Library Test\n");
    printf("========================\n\n");

    // Create HTTP client
    http_client_t* client = http_client_create("MyApp/1.0");
    if (!client) {
        printf("Failed to create HTTP client\n");
        return 1;
    }

    // Test GET request
    printf("Testing GET request...\n");
    http_response_t* response = http_get(client, "https://httpbin.org/get");
    if (response) {
        printf("Status Code: %d\n", response->status_code);
        printf("Body Length: %zu\n", response->body_length);
        if (response->body) {
            printf("Body: %s\n", response->body);
        }
        http_response_destroy(response);
    } else {
        printf("GET request failed\n");
    }

    printf("\n");

    // Test POST request
    printf("Testing POST request...\n");
    const char* post_data = "{\"test\": \"data\"}";
    response = http_post(client, "https://httpbin.org/post", post_data, "application/json");
    if (response) {
        printf("Status Code: %d\n", response->status_code);
        printf("Body Length: %zu\n", response->body_length);
        if (response->body) {
            printf("Body: %s\n", response->body);
        }
        http_response_destroy(response);
    } else {
        printf("POST request failed\n");
    }

    // Cleanup
    http_client_destroy(client);

    printf("\nTest completed.\n");
    return 0;
}
