#ifndef DEBUG_H
#define DEBUG_H

/* Colorized output, baby! */
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#define DEBUG_SHORT_NAME "UTIL"

// These prints wrappers are designed to make it easy to add pretty messages
// var, min are meant to allow for silencing of the messages, or by setting
// them to a useful level.
#define P_DBG(var, min, msg...)\
    if(var > min) {\
        fprintf(stderr, ANSI_COLOR_CYAN    "<dbg> " msg); \
        fprintf(stderr, ANSI_COLOR_RESET);\
    }

#define P_ERR(msg...) \
    fprintf(stderr, ANSI_COLOR_RED     "<err> " msg); \
    fprintf(stderr, ANSI_COLOR_RESET);

#define P_WRN(var, msg...) \
    if(var >= 0) {\
        fprintf(stdout, ANSI_COLOR_YELLOW  "<warn> " msg); \
        fprintf(stdout, ANSI_COLOR_RESET);\
    }

#define P_NFO(var, msg...) \
    if(var >= 0) {\
        fprintf(stdout, ANSI_COLOR_GREEN "[" DEBUG_SHORT_NAME "] " msg); \
        fprintf(stdout, ANSI_COLOR_RESET);\
    }

#define P_DFL(var, msg...) \
    if(var >= 0)\
    fprintf(stdout, "[" DEBUG_SHORT_NAME "] " msg);

// These wrappers are to ease condition passing
#define C_ERR(flag, msg...) \
    if(flag) {\
        fprintf(stderr, ANSI_COLOR_RED     "<err> " msg); \
        fprintf(stderr, ANSI_COLOR_RESET);\
    }

#define C_WRN(flag, msg...) \
    if(flag) {\
        fprintf(stdout, ANSI_COLOR_YELLOW  "<warn> " msg); \
        fprintf(stdout, ANSI_COLOR_RESET);\
    }

#endif
