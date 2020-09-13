#include <mach/daudio.h>
#include <mach/daudio_debug.h>
#include <mach/daudio_info.h>
#include <mach/daudio_ie.h>

#include "daudio_ie_twxxxx.h"

CLOG_INIT(CLOG_TAG_CLEINSOFT, CLOG_TAG_IE_TWXXXX, CLOG_LEVEL_IE_TWXXXX);

#define DEBUG_IE_LEVEL	get_ie_debug_level()
#define sCLogLevel		(CLOG_LEVEL_IE_TWXXXX <= DEBUG_IE_LEVEL ? DEBUG_IE_LEVEL : CLOG_LEVEL_IE_TWXXXX)

extern int TW8836_BRIGHTNESS_LIMITS[];
extern int TW8836_CONTRAST_LIMITS[];
extern int TW8836_HUE_LIMITS[];
extern int TW8836_SATURATION_LIMITS[];

extern int tw8836_write_ie(int cmd, unsigned char level);
extern int tw8836_read_ie(int cmd, char* level);

extern int tw9912_write_ie(int cmd, unsigned char level);
extern int tw9912_read_ie(int cmd, char* level);

int is_twxxxx_ie_invalid_arg(int mode, char level)
{
	if (level < 1 || level > 255)
		return 1;
	else
		return 0;
}

int twxxxx_set_ie(int cmd, unsigned char level)
{
	CLOG(CLL_INFO, "%s\n", __func__);
	
	switch (daudio_main_version())
	{
		case DAUDIO_BOARD_6TH:
		case DAUDIO_BOARD_5TH:
		default:
			return tw9912_write_ie(cmd, level);
	}

	return FAIL;
}

int twxxxx_get_ie(int cmd, char *level)
{
	CLOG(CLL_INFO, "%s\n", __func__);

	switch (daudio_main_version())
	{
		case DAUDIO_BOARD_6TH:
		case DAUDIO_BOARD_5TH:
		default:
			return tw9912_read_ie(cmd, level);
	}

	return FAIL;
}

int twxxxx_hue_valid_level(char level)
{
	return level <= TW8836_BRIGHTNESS_LIMITS[1] ? 1 : 0;
}
