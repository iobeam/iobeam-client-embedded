#ifndef IOBEAM_ARDUINO_LOG_H_
#define IOBEAM_ARDUINO_LOG_H_

#if DEBUG_LEVEL > 0
#	define IOBEAM_ERR(msg) Serial.print(msg);
#	define IOBEAM_ERR_W(msg, len) Serial.write(msg, len);
#else
#	define IOBEAM_ERR(msg)
#	define IOBEAM_ERR_W(msg, len)
#endif

#if DEBUG_LEVEL > 1
#	define IOBEAM_LOG(msg) Serial.print(msg);
#	define IOBEAM_LOG_W(msg, len) Serial.write(msg, len);
#else
#	define IOBEAM_LOG(msg)
#	define IOBEAM_LOG_W(msg, len)
#endif

#if DEBUG_LEVEL > 2
#	define IOBEAM_DEBUG(msg) Serial.print(msg);
#	define IOBEAM_DEBUG_W(msg, len) Serial.write(msg, len);
#else
#	define IOBEAM_DEBUG(msg)
#	define IOBEAM_DEBUG_W(msg, len)
#endif

#if DEBUG_LEVEL > 3
#	define IOBEAM_VERBOSE(msg) Serial.print(msg);
#	define IOBEAM_VERBOSE_W(msg, len) Serial.write(msg, len);
#else
#	define IOBEAM_VERBOSE(msg)
#	define IOBEAM_VERBOSE_W(msg, len)
#endif

#endif // IOBEAM_ARDUINO_LOG_H
