#include "stdafx.h"

namespace resizer
{
	DECLARE_COMPONENT_VERSION(
		"Cover Resizer",
		"0.0.4",
		"Copyright (C) 2022 marc2003\n\n"
		"Build: " __TIME__ ", " __DATE__
	);

	VALIDATE_COMPONENT_FILENAME("foo_cover_resizer.dll");

	namespace settings
	{
		cfg_int type(guid_cfg_type, 0);
		cfg_int format(guid_cfg_format, 0);
		cfg_int size(guid_cfg_size, 500);

		GUID get_guid()
		{
			return album_art_ids::query_type(type.get_value());
		}
	}
}
