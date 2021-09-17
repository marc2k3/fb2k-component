#include "stdafx.h"

using namespace resizer;

class ContextMenuUtils : public contextmenu_item_simple
{
public:
	GUID get_item_guid(uint32_t index) override
	{
		if (index == 0) return guid_context_command_convert;
		else if (index == 1) return guid_context_command_remove_all_except_front;
		else uBugCheck();
	}

	GUID get_parent() override
	{
		return guid_context_group_utils;
	}

	bool context_get_display(uint32_t index, metadb_handle_list_cref handles, pfc::string_base& out, uint32_t& displayflags, const GUID& caller) override
	{
		get_item_name(index, out);
		return true;
	}

	bool get_item_description(uint32_t index, pfc::string_base& out) override
	{
		get_item_name(index, out);
		return true;
	}

	uint32_t get_num_items() override
	{
		return 2;
	}

	void context_command(uint32_t index, metadb_handle_list_cref handles, const GUID& caller) override
	{
		const HWND hwnd = core_api::get_main_window();

		if (index == 0)
		{
			if (MessageBoxW(hwnd, L"This option will ignore any images that are already JPG. Continue?", string_wide_from_utf8_fast(group_utils), MB_ICONWARNING | MB_SETFOREGROUND | MB_YESNO) == IDYES)
			{
				auto cb = fb2k::service_new<CoverResizer>(handles, album_art_ids::cover_front, true);
				threaded_process::get()->run_modeless(cb, threaded_process_flags, hwnd, "Converting front covers to JPG...");
			}
		} 
		else if (index == 1)
		{
			auto cb = fb2k::service_new<CoverRemover>(handles);
			threaded_process::get()->run_modeless(cb, threaded_process_flags, hwnd, "Removing covers...");
		}
	}

	void get_item_name(uint32_t index, pfc::string_base& out) override
	{
		if (index == 0) out = "Convert front covers to JPG without resizng";
		else if (index == 1) out = "Remove all except front";
	}
};

static contextmenu_group_popup_factory g_context_group_utils(guid_context_group_utils, contextmenu_groups::root, group_utils, 1);
FB2K_SERVICE_FACTORY(ContextMenuUtils);
