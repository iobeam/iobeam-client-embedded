#ifndef IOBEAM_COMMON_H_
#define IOBEAM_COMMON_H_

#define __STDC_LIMIT_MACROS
#include <inttypes.h>

#ifndef API_DEFAULT_SERVER
    #define API_DEFAULT_SERVER "api.iobeam.com"
#endif

#define API_DEFAULT_PORT 80
#define API_MAX_DEVICE_ID_LEN 49
#define API_DEVICE_ID_KEY "device_id\":"
#define API_SERVER_TIME_KEY "server_timestamp\":"

#define RESOURCE_IMPORTS "/v1/imports"
#define RESOURCE_ADD_DEVICE "/v1/devices"
#define RESOURCE_GET_TIME "/v1/devices/timestamp"

#define ADD_DEVICE_JSON "{\"project_id\":%" PRIu32 "}"

#include "http.h"

// Function pointer typedefs for C and C++, C++ includes extra arg for the obj
typedef int (*netSendFunc)(char *, size_t);
typedef int (*netSendFunc_cpp)(void *, char *, size_t);

// Determines whether to treat `f` as a C or C++ function pointer based on
// whether the object has been set.
static int _call_send_func(void *obj, void *f, char *buf, size_t len)
{
	if (!obj) {
		netSendFunc fn = (netSendFunc) f;
		return fn(buf, len);
	}
	else {
		netSendFunc_cpp fn = (netSendFunc_cpp) f;
		return fn(obj, buf, len);
	}
}

//
// Generic version of common functions that work for either C or C++
//

static void _iobeam_generic_StartGet(void *obj, void *func, char *dst,
		size_t dstLen, char *resource, size_t resourceLen)
{
	int ret = makeGetRequest(dst, dstLen, resource, resourceLen);
	if (ret < 0)
		return;
	_call_send_func(obj, func, dst, ret);
}

static void _iobeam_generic_StartPost(void *obj, void *func, char* dst,
		size_t dstLen, char *resource, size_t resourceLen)
{
	int ret = makePostRequest(dst, dstLen, resource, resourceLen);
	if (ret < 0) {
	    //UART_PRINT("Could not prepare HTTP request\r\n");
	    return;
	}
	_call_send_func(obj, func, dst, ret);
}

static void _iobeam_generic_WriteHeader(void *obj, void *func, char* dst,
		size_t dstLen, const char *key, size_t keyLen, const char* val,
		size_t valLen)
{
	int ret = makeHeader(dst, dstLen, key, keyLen, val, valLen);
    if (ret < 0) {
    	//UART_PRINT("Could not prepare header\r\n");
        return;
    }
    _call_send_func(obj, func, dst, ret);
}

static void _iobeam_generic_WriteFormattedHeader(void *obj, void *func,
		char *dst, size_t dstLen, const char *key, size_t keyLen,
		const char *fmt, ...)
{
	va_list vArgs;
	va_start(vArgs, fmt);
	int ret = makeHeaderInFormat(dst, dstLen, key, keyLen, fmt, vArgs);
	va_end(vArgs);
	if (ret < 0) {
		return;
	}
	_call_send_func(obj, func, dst, ret);
}

static inline void _iobeam_generic_WriteCommonHeaders(void *obj, void *func,
		char *dst, size_t dstLen)
{
	_iobeam_generic_WriteHeader(obj, func, dst, dstLen,
			HTTP_HEADER_HOST,
			sizeof(HTTP_HEADER_HOST),
			API_DEFAULT_SERVER,
			sizeof(API_DEFAULT_SERVER));

	_iobeam_generic_WriteHeader(obj, func, dst, dstLen,
			HTTP_HEADER_CONNECTION,
			sizeof(HTTP_HEADER_CONNECTION),
			HTTP_CONNECTION_CLOSE,
			sizeof(HTTP_CONNECTION_CLOSE));

	_iobeam_generic_WriteHeader(obj, func, dst, dstLen,
			HTTP_HEADER_CONTENT_TYPE,
			sizeof(HTTP_HEADER_CONTENT_TYPE),
			HTTP_CONTENT_TYPE_JSON,
			sizeof(HTTP_CONTENT_TYPE_JSON));
}

static void _iobeam_generic_WriteContentLengthHeader(void *obj, void *func,
        char *dst, size_t dstLen, uint32_t len)
{
    _iobeam_generic_WriteFormattedHeader(obj, func, dst, dstLen,
            HTTP_HEADER_CONTENT_LENGTH, sizeof(HTTP_HEADER_CONTENT_LENGTH),
            "%" PRIu32 HEADER_END, len);
}

static void _iobeam_generic_WriteTokenHeader(void *obj, void *func, char *dst,
    size_t dstLen, const char *token)
{
    _iobeam_generic_WriteFormattedHeader(obj, func, dst, dstLen,
            HTTP_HEADER_TOKEN, sizeof(HTTP_HEADER_TOKEN),
            "Bearer %s" HEADER_END, token);
}

static void _iobeam_generic_EndHeaders(void *obj, void *func)
{
    _call_send_func(obj, func, HEADER_END, sizeof(HEADER_END) - 1);
}

static inline void _iobeam_generic_WriteBody(void *obj, void *func, char *body,
        size_t bodyLen)
{
    if (bodyLen > 0 && body != NULL) {
        _call_send_func(obj, func, body, bodyLen);
    }
}


//
// Language specific interfaces for C++ and C
//
#ifdef __cplusplus
static void _iobeam_StartGet(void *obj, netSendFunc_cpp f, char *dst,
		size_t dstLen, char *resource, size_t resourceLen)
{
	_iobeam_generic_StartGet(obj, (void *) f, dst, dstLen, resource,
			resourceLen);
}

static void _iobeam_StartPost(void *obj, netSendFunc_cpp f, char* dst,
		size_t dstLen, char *resource, size_t resourceLen)
{
	_iobeam_generic_StartPost(obj, (void *) f, dst, dstLen, resource,
			resourceLen);
}

static void _iobeam_WriteHeader(void *obj, netSendFunc_cpp f,
		char* dst, size_t dstLen, const char *key, size_t keyLen,
		const char* val, size_t valLen)
{
	_iobeam_generic_WriteHeader(obj, (void *) f, dst, dstLen, key, keyLen,
			val, valLen);
}

static void _iobeam_WriteCommonHeaders(void *obj, netSendFunc_cpp f,
		char *dst, size_t dstLen)
{
	_iobeam_generic_WriteCommonHeaders(obj, (void *) f, dst, dstLen);
}

static void _iobeam_WriteContentLengthHeader(void *obj, netSendFunc_cpp f,
		char *dst, size_t dstLen, uint32_t len)
{
	_iobeam_generic_WriteContentLengthHeader(obj, (void *) f, dst, dstLen, len);
}

static void _iobeam_WriteTokenHeader(void *obj, netSendFunc_cpp f, char *dst,
		size_t dstLen, const char *token)
{
	_iobeam_generic_WriteTokenHeader(obj, (void *) f, dst, dstLen, token);
}

static void _iobeam_EndHeaders(void *obj, netSendFunc_cpp f)
{
	_iobeam_generic_EndHeaders(obj, (void *) f);
}

static inline void _iobeam_WriteBody(void *obj, netSendFunc_cpp f, char *body,
		size_t bodyLen)
{
	_iobeam_generic_WriteBody(obj, (void *) f, body, bodyLen);
}

#define netSendFunc netSendFunc_cpp

#else
static void _iobeam_StartGet(netSendFunc f, char *dst,
		size_t dstLen, char *resource, size_t resourceLen)
{
	_iobeam_generic_StartGet(NULL, (void *) f, dst, dstLen, resource,
			resourceLen);
}


static void _iobeam_StartPost(netSendFunc f, char* dst, size_t dstLen,
		char *resource, size_t resourceLen)
{
	_iobeam_generic_StartPost(NULL, (void *) f, dst, dstLen, resource,
			resourceLen);
}

static void _iobeam_WriteHeader(netSendFunc f, char* dst, size_t dstLen,
		const char *key, size_t keyLen, const char* val, size_t valLen)
{
	_iobeam_generic_WriteHeader(NULL, (void *) f, dst, dstLen, key, keyLen,
			val, valLen);
}

static void _iobeam_WriteCommonHeaders(netSendFunc f, char *dst,
		size_t dstLen)
{
	_iobeam_generic_WriteCommonHeaders(NULL, (void *) f, dst, dstLen);
}

static void _iobeam_WriteContentLengthHeader(netSendFunc f, char *dst,
		size_t dstLen, uint32_t len)
{
	_iobeam_generic_WriteContentLengthHeader(NULL, (void *) f, dst, dstLen,
			len);
}

static void _iobeam_WriteTokenHeader(netSendFunc f, char *dst, size_t dstLen,
		const char *token)
{
	_iobeam_generic_WriteTokenHeader(NULL, (void *) f, dst, dstLen, token);
}

static void _iobeam_EndHeaders(netSendFunc f)
{
	_iobeam_generic_EndHeaders(NULL, (void *) f);
}

static inline void _iobeam_WriteBody(netSendFunc f, char *body, size_t bodyLen)
{
	_iobeam_generic_WriteBody(NULL, (void *) f, body, bodyLen);
}
#endif

//
// Common library functions that do not need function pointer support
//

static int _iobeam_ParseDeviceId(char *dst, char *deviceAddRsp)
{
    // To find the id, we first find the field in JSON (device_id), then
    // search for the first quote (") after the colon (:). Once found, the
    // first character will be one byte after it.
	char *start = strstr(deviceAddRsp, API_DEVICE_ID_KEY);
	start += (sizeof(API_DEVICE_ID_KEY) - 1);
	start = strchr(start, '\"');
	start++;

    // To get the length, we find the position of the first quote (")
    // after the start and subtract.
    size_t len = (strchr(start, '\"') - start);
    if (len > API_MAX_DEVICE_ID_LEN)
        return -1;

    memcpy(dst, start, len);
    return len;
}


#endif /* COMMON_H_ */
