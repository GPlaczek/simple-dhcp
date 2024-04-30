#include "log.h"

#include <string.h>
#include <stdarg.h>
#include <time.h>

#include <unistd.h>

#define LOG_LINE_BUFFER_LEN 4096

char line_buffer[LOG_LINE_BUFFER_LEN];
const char *LOG_LABELS[] = {
	"ERROR",
	"WARN ",
	"INFO ",
	"DEBUG"
};
const size_t LOG_LABELS_LEN = sizeof(LOG_LABELS) / sizeof(LOG_LABELS[0]);

int log_level = LOGLEVEL_INFO;

void set_log_level(int level) {
	log_level = level;
}

int human_readable_haddr(char *restrict buf, const uint8_t *restrict haddr, uint8_t halen) {
	int written = 0, len = 0;
	for (int i = 0; i < halen; i++) {
		len = sprintf(buf+written, "%02x:", haddr[i]);
		if (len < 0)
			return -1;

		written+=len;
	}
	buf[written-1] = 0;
	return written;
}

void vfslog(int level, const char *restrict *fields, int flen, const char *restrict fmt, va_list args) {
	struct timespec tms;
	struct tm tmi;
	int written = 0, len = 0, i = 0;

	if (level > log_level)
		return;

	clock_gettime(CLOCK_REALTIME, &tms);
	localtime_r(&tms.tv_sec, &tmi);

	line_buffer[written++] = '[';
	len = strftime(line_buffer+written, sizeof(line_buffer) - written,
		"%Y-%m-%d %H:%M:%S", &tmi);
	written+=len;
	len = snprintf(line_buffer + written, LOG_LINE_BUFFER_LEN - written, "] [%s] ", LOG_LABELS[level-1]);
	written+= len;

	for (i = 0; i < flen; i++) {
		len = snprintf(line_buffer + written, LOG_LINE_BUFFER_LEN - written, "[%s] ", fields[i]);
		written+=len;
	}

	len = vsnprintf(line_buffer + written, sizeof(line_buffer) - written, fmt, args);
	written += len;

	write(STDOUT_FILENO, line_buffer, written);
}

void vslog(int level, const char *fmt, va_list args) {
	vfslog(level, NULL, 0, fmt, args);
}

void vhslog(int level, const char *restrict haddr, const char *restrict fmt, va_list args) {
	vfslog(level, &haddr, 1, fmt, args);
}

void slog(int level, const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	vslog(level, fmt, args);
	va_end(args);
}

void hslog(int level, const char *restrict haddr, const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	vhslog(level, haddr, fmt, args);
	va_end(args);
}
