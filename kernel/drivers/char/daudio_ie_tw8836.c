#include "daudio_ie_tw8836.h"
 
extern int TW8836_BRIGHTNESS_LIMITS[];
extern int TW8836_CONTRAST_LIMITS[];
extern int TW8836_HUE_LIMITS[];
extern int TW8836_SATURATION_LIMITS[];

extern int tw8836_write_ie(int cmd, unsigned char level);
extern int tw8836_read_ie(int cmd, char* level);

/**
 * @return 1 is argument overflow.
 */
int tw8836_check_overflow_arg(int mode, char level)
{
	if (level < 0 || level > 255)
		return 1;
	else
		return 0;
}

int tw8836_set_ie(int cmd, unsigned char level)
{
	return tw8836_write_ie(cmd, level);
}

int tw8836_get_ie(int cmd, char *level)
{
	return tw8836_read_ie(cmd, level);
}

int tw8836_hue_valid_level(char level)
{
	return level <= TW8836_BRIGHTNESS_LIMITS[1] ? 1 : 0;
}
