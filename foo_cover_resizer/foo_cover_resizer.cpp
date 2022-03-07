#include "stdafx.hpp"

#pragma comment(lib, "Windowscodecs.lib")

wil::com_ptr_nothrow<IWICImagingFactory> g_imaging_factory;

namespace resizer
{
	DECLARE_COMPONENT_VERSION(
		component_name,
		"0.1.1",
		"Copyright (C) 2022 marc2003\n\n"
		"Build: " __TIME__ ", " __DATE__
	);

	VALIDATE_COMPONENT_FILENAME("foo_cover_resizer.dll");

	bool api_check()
	{
		if (g_imaging_factory)
			return true;

		popup_message::g_show(wic_error, component_name);
		return false;
	}

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

	class InitQuit : public initquit
	{
	public:
		void on_init() override
		{
			g_imaging_factory = wil::CoCreateInstanceNoThrow<IWICImagingFactory>(CLSID_WICImagingFactory);
		}

		void on_quit() override
		{
			g_imaging_factory.reset();
		}
	};

	FB2K_SERVICE_FACTORY(InitQuit);
}
