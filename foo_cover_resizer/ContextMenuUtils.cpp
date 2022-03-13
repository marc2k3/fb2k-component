#include "stdafx.hpp"

using namespace resizer;

static const std::vector<ContextItem> context_items =
{
	{ &guid_context_command_convert, "Convert without resizng" },
	{ &guid_context_command_convert_and_attach, "Browse for file, convert and attach" },
	{ &guid_context_command_remove_all_except_front, "Remove all except front" },
};

class ContextMenuUtils : public contextmenu_item_simple
{
public:
	GUID get_item_guid(uint32_t index) override
	{
		return *context_items[index].guid;
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
		return pfc::downcast_guarded<uint32_t>(context_items.size());
	}

	void context_command(uint32_t index, metadb_handle_list_cref handles, const GUID& caller) override
	{
		if (!api_check()) return;

		const HWND hwnd = core_api::get_main_window();

		if (index == 0)
		{
			if (!choose_settings(hwnd, false)) return;

			const Format format = formats[settings::format].first;
			const GUID art_guid = album_art_ids::query_type(settings::type);
			auto cb = fb2k::service_new<CoverConverterResizer>(CoverConverterResizer::Action::convert, handles, format, art_guid);
			threaded_process::get()->run_modeless(cb, threaded_process_flags, hwnd, "Converting covers...");
		}
		else if (index == 1)
		{
			album_art_data_ptr data;
			pfc::string8 folder = pfc::string_directory(handles[0]->get_path());
			wil::com_ptr_t<IWICBitmapSource> source;

			if (!browse_for_image(hwnd, folder, source)) return;
			if (!choose_settings(hwnd, false)) return;

			const Format format = formats[settings::format].first;
			if (FAILED(encode(format, source.get(), data))) return;

			if (data.is_valid())
			{
				const GUID art_guid = album_art_ids::query_type(settings::type);
				auto cb = fb2k::service_new<CoverAttach>(handles, data, art_guid);
				threaded_process::get()->run_modeless(cb, threaded_process_flags, hwnd, "Attaching cover...");
			}
		}
		else if (index == 2)
		{
			auto cb = fb2k::service_new<CoverRemover>(handles);
			threaded_process::get()->run_modeless(cb, threaded_process_flags, hwnd, "Removing covers...");
		}
	}

	void get_item_name(uint32_t index, pfc::string_base& out) override
	{
		out = context_items[index].name;
	}
};

static contextmenu_group_popup_factory g_context_group_utils(guid_context_group_utils, contextmenu_groups::root, "Cover Utils", 1);
FB2K_SERVICE_FACTORY(ContextMenuUtils);
