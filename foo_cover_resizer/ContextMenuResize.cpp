#include "stdafx.h"
#include "PopupDialog.h"

using namespace resizer;

class ContextMenuResize : public contextmenu_item_simple
{
public:
	GUID get_item_guid(uint32_t index) override
	{
		if (index == 0) return guid_context_command_resize;
		else uBugCheck();
	}

	GUID get_parent() override
	{
		return guid_context_group_resize;
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
		return 1;
	}

	void context_command(uint32_t index, metadb_handle_list_cref handles, const GUID& caller) override
	{
		HWND hwnd = core_api::get_main_window();
		modal_dialog_scope scope;
		if (scope.can_create())
		{
			scope.initialize(hwnd);
			PopupDialog dlg;
			if (dlg.DoModal(hwnd) == IDOK)
			{
				auto cb = fb2k::service_new<CoverResizer>(handles, album_art_ids::query_type(prefs::type.get_value()), prefs::format.get_value(), prefs::size.get_value());
				threaded_process::get()->run_modeless(cb, threaded_process_flags, hwnd, "Resizing covers...");
			}
		}
	}

	void get_item_name(uint32_t index, pfc::string_base& out) override
	{
		out = "Resize";
	}
};

static contextmenu_group_popup_factory g_context_group_resize(guid_context_group_resize, contextmenu_groups::root, group_resize, 0);
FB2K_SERVICE_FACTORY(ContextMenuResize);
