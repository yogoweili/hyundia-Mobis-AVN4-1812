#ifndef __DAUDIO_IE_TW8836__
#define __DAUDIO_IE_TW8836__

#include <mach/daudio_settings.h>

int tw8836_check_overflow_arg(int mode, char level);
int tw8836_set_ie(int cmd, unsigned char level);
int tw8836_get_ie(int cmd, char* level);
int tw8836_hue_valid_level(char level);

#endif
