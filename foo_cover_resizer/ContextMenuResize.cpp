#include "stdafx.hpp"

using namespace resizer;

static const std::vector<ContextItem> context_items =
{
	{ &guid_context_command_resize, "Resize" },
	{ &guid_context_command_resize_and_attach, "Browse for file, resize and attach" },
};

class ContextMenuResize : public contextmenu_item_simple
{
public:
	GUID get_item_guid(uint32_t index) override
	{
		return *context_items[index].guid;
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
		return pfc::downcast_guarded<uint32_t>(context_items.size());
	}

	void context_command(uint32_t index, metadb_handle_list_cref handles, const GUID& caller) override
	{
		if (!api_check()) return;

		const HWND hwnd = core_api::get_main_window();

		if (index == 0)
		{
			if (choose_settings(hwnd, true))
			{
				const Format format = formats[settings::format].first;
				auto cb = fb2k::service_new<CoverResizer>(handles, format);
				threaded_process::get()->run_modeless(cb, threaded_process_flags, hwnd, "Resizing covers...");
			}
		}
		else if (index == 1)
		{
			album_art_data_ptr data;
			pfc::string8 folder = pfc::string_directory(handles[0]->get_path());
			wil::com_ptr_t<IWICBitmapSource> source;
			wil::com_ptr_t<IWICBitmapScaler> scaler;

			if (!browse_for_image(hwnd, folder, source)) return;
			if (!choose_settings(hwnd, true)) return;
			if (FAILED(resize(source.get(), scaler))) return;

			const Format format = formats[settings::format].first;
			if (FAILED(encode(format, scaler.get(), data))) return;

			if (data.is_valid())
			{
				auto cb = fb2k::service_new<CoverAttach>(handles, data);
				threaded_process::get()->run_modeless(cb, threaded_process_flags, hwnd, "Attaching cover...");
			}
		}
	}

	void get_item_name(uint32_t index, pfc::string_base& out) override
	{
		out = context_items[index].name;
	}
};

static contextmenu_group_popup_factory g_context_group_resize(guid_context_group_resize, contextmenu_groups::root, component_name, 0);
FB2K_SERVICE_FACTORY(ContextMenuResize);
