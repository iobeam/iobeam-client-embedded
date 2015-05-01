#include "../include/http.h"

int makeRequest(char *buf, size_t bufLen, const char *method, size_t methodLen,
	char *resource, size_t resourceLen)
{
	int lineLen = methodLen + 1 + resourceLen + 1 + PROTOCOL_LEN + 2;
	if (lineLen > bufLen)  // buffer too small
		return -1;

	// Uses more storage space, could go with memcpy/memset.
	memset(buf, '\0', bufLen);
	int ret = snprintf(buf, bufLen, "%s %s %s" HEADER_END, method, resource, PROTOCOL);
	return ret;
}

int makeGetRequest(char *buf, size_t bufLen, char *resource,
	size_t resourceLen)
{
	return makeRequest(buf, bufLen, 
		HTTP_METHOD_GET, sizeof(HTTP_METHOD_GET) - 1, resource, resourceLen);
}

int makePostRequest(char *buf, size_t bufLen, char *resource,
	size_t resourceLen)
{
	return makeRequest(buf, bufLen, 
		HTTP_METHOD_POST, sizeof(HTTP_METHOD_POST) - 1, resource, resourceLen);
}

int makeHeader(char *buf, size_t bufLen, const char* key, size_t keyLen,
	const char* val, size_t valLen)
{
	int lineLen = keyLen + 2 + valLen + 2; // 2 = ": ", 2 = "\r\n"
	if (lineLen > bufLen)
		return -1;

	memset(buf, '\0', bufLen);
	int ret = snprintf(buf, bufLen, "%s: %s" HEADER_END, key, val);
	return ret;
}

int makeHeaderInFormat(char *buf, size_t bufLen, const char *key, size_t keyLen,
		const char *valFmt, va_list vArgs)
{
	memset(buf, '\0', bufLen);
	int keyOff = snprintf(buf, bufLen, "%s: ", key);
	int ret = vsnprintf(buf + keyOff, bufLen - keyOff, valFmt, vArgs);

	return ret + keyOff;
}

int parseContentLength(char *line) 
{
	int ret = -1;
	char *p = strstr(line, HTTP_HEADER_CONTENT_LENGTH);
	if (!p) {
		p = strstr(line, "Content-Length: ");
	}
	if (p) {
		p += sizeof(HTTP_HEADER_CONTENT_LENGTH) - 1;
		p += 2;  // moves past the ": "
		ret = strtol(p, NULL, 10);
	}

	return ret;
}

int parseResponseCode(char *line)
{
	char *spacePos = strchr(line, ' ');
    return (int) strtol(spacePos, NULL, 10);
}
