#ifndef __LOG_H
#define __LOG_H

int log_init();

void log_debug(const char *format, ...);
void log_error(const char *format, ...);

#endif /* _LOG_H */
