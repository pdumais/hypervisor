#include <stdarg.h>
#include <stdio.h>

void inline log(const char* str, ...)
{
    //TODO: re-enable this. Send to file instead
    va_list args;
    va_start(args, str);
    //vprintf(str, args);
    va_end(args);
}