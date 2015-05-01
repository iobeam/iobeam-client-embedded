#ifndef http_h
#define http_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PROTOCOL "HTTP/1.1"
#define PROTOCOL_LEN (sizeof(PROTOCOL) - 1)
#define HEADER_END "\r\n"

#define HTTP_HEADER_HOST           "Host"
#define HTTP_HEADER_CONTENT_LENGTH "content-length"
#define HTTP_HEADER_CONTENT_TYPE   "Content-Type"
#define HTTP_HEADER_CONNECTION     "Connection"
#define HTTP_HEADER_TOKEN          "Authorization"

#define HTTP_CONTENT_TYPE_JSON     "application/json"

#define HTTP_CONNECTION_CLOSE      "close"
#define HTTP_CONNECTION_KEEP_ALIVE "keep-alive"

#define HTTP_METHOD_GET    "GET"
#define HTTP_METHOD_POST   "POST"

#ifdef __cplusplus
extern "C" {
#endif

int makeRequest(char *buf, size_t bufLen, const char *method, size_t methodLen,
	char* rsource, size_t resourceLen);
int makeGetRequest(char *buf, size_t bufLen, char *resource,
	size_t resourceLen);
int makePostRequest(char *buf, size_t bufLen, char *resource,
	size_t resourceLen);

int makeHeader(char *buf, size_t bufLen, const char* key, size_t keyLen,
	const char* val, size_t valLen);
int makeHeaderInFormat(char *buf, size_t bufLen, const char *key, size_t keyLen,
		const char *valFmt, va_list vArgs);

int readResponseLine(char *buf, size_t bufLen);

int parseResponseCode(char *line);
int parseContentLength(char *line);

#ifdef __cplusplus
}
#endif

#endif /* http_h */
