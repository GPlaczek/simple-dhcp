#ifndef log_h_INCLUDED
#define log_h_INCLUDED

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>

#define LOGLEVEL_DEBUG 4
#define LOGLEVEL_INFO  3
#define LOGLEVEL_WARN  2
#define LOGLEVEL_ERROR 1

extern const char *LOG_LABELS[];

#define LOG(LEVEL, fmt, ...) printf("[%s] " fmt, LOG_LABELS[LEVEL-1], ##__VA_ARGS__)
#define LOG_HADDR(LEVEL, HADDR, fmt, ...) LOG(LEVEL, "[%s] " fmt, HADDR, ##__VA_ARGS__)

int human_readable_haddr(char *restrict buf, const uint8_t *restrict haddr, uint8_t halen);
void vfslog(int level, const char *restrict *fields, int flen, const char *restrict fmt, va_list args);
void vslog(int level, const char *fmt, va_list args);
void slog(int level, const char *fmt, ...);

void vhslog(int level, const char *restrict haddr, const char *restrict fmt, va_list args);
void hslog(int level, const char *restrict haddr, const char *restrict fmt, ...);

#endif  // log_h_INCLUDED
