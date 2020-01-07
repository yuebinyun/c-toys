#ifndef _JANUS_DEBUG_H
#define _JANUS_DEBUG_H

#include <glib.h>


#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

/*! \brief No debugging */
#define LOG_NONE     (0u)
/*! \brief Fatal error */
#define LOG_FATAL    (1u)
/*! \brief Non-fatal error */
#define LOG_ERR      (2u)
/*! \brief Warning */
#define LOG_WARN     (3u)
/*! \brief Informational message */
#define LOG_INFO     (4u)
/*! \brief Verbose message */
#define LOG_VERB     (5)
/*! \brief Overly verbose message */
#define LOG_HUGE     (6)
/*! \brief Debug message (includes .c filename, function and line number) */
#define LOG_DBG      (7u)
/*! \brief Maximum level of debugging */
#define LOG_MAX LOG_DBG

/*! \brief Coloured prefixes for errors and warnings logging. */
static const char *janus_log_prefix[] = {
/* no colors */
        "",
        "[FATA] ",
        "[ERR ] ",
        "[WARN] ",
        "[INFO] ",
        "",
        "",
        "",
/* with colors */
        "",
        ANSI_COLOR_MAGENTA "[FATA]" ANSI_COLOR_RESET " ",
        ANSI_COLOR_RED "[ERR ]" ANSI_COLOR_RESET " ",
        ANSI_COLOR_YELLOW "[WARN]" ANSI_COLOR_RESET " ",
        ANSI_COLOR_CYAN "[INFO]" ANSI_COLOR_RESET " ",
        "",
        "",
        ""
};

#define JANUS_LOG(level, format, ...) \
do { \
    if (level > LOG_NONE && level <= LOG_MAX && level <= LOG_INFO) { \
        char janus_log_ts[64] = ""; \
        char janus_log_src[128] = ""; \
        struct tm janustmresult; \
        time_t janusltime = time(NULL); \
        localtime_r(&janusltime, &janustmresult); \
        strftime(janus_log_ts, sizeof(janus_log_ts), \
                 "[%Y-%m-%d %X] ", &janustmresult); \
        snprintf(janus_log_src, sizeof(janus_log_src), \
                 "[%s:%d:%s] ", __FILE__, __LINE__, __FUNCTION__); \
        printf("%s%s[%ld] %s" format, \
            janus_log_ts, \
            janus_log_prefix[level | ((int)TRUE << 3)], \
            pthread_self(), \
            janus_log_src, \
            ##__VA_ARGS__); \
    } \
} while (0)
///@}

#endif
