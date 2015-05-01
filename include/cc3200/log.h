#ifndef IOBEAM_CC3200_LOG_H_
#define IOBEAM_CC3200_LOG_H_

#ifdef __COMMON__H__
#define IOBEAM_ERR(fmt, args...) if (DEBUG_LEVEL > 0) { UART_PRINT(fmt, ##args); }
#define IOBEAM_LOG(fmt, args...) if (DEBUG_LEVEL > 1) { UART_PRINT(fmt, ##args); }
#define IOBEAM_DEBUG(fmt, args...) if (DEBUG_LEVEL > 2) { UART_PRINT(fmt, ##args); }
#define IOBEAM_VERBOSE(fmt, args...) if (DEBUG_LEVEL > 3) { UART_PRINT(fmt, ##args); }
#else
#define IOBEAM_ERR(fmt, args...)
#define IOBEAM_LOG(fmt, args...)
#define IOBEAM_DEBUG(fmt, args...)
#define IOBEAM_VERBOSE(fmt, args...)
#endif

#endif // IOBEAM_CC3200_LOG_H
