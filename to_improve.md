### Design Flaw: Connection Management
The `http_client_t` struct contains a connection handle `HINTERNET connection`, but it is never used. The `http_request` function creates, uses, and closes a new TCP connection for every single request.
This approach has two main problems:
- **Inefficiency**: Establishing a new TCP and TLS (for HTTPS) connection for each request is slow and resource-intensive. Reusing connections is a major performance optimization (HTTP Keep-Alive).
- **Confusing Design**: The presence of `client->connection` suggests that the client manages a persistent connection, which is not the case.

**Recommendation:**
You have two options to fix this:
1. **Simple Fix**: Remove the `HINTERNET connection;` field from the `http_client_t` struct to accurately reflect that connections are managed per-request.
2. **Better Fix (More Complex)**: Refactor the logic to reuse connections. This would involve checking if an active connection for the target host/port already exists in the client, using it if available, and creating one otherwise (http_client.c:193).
