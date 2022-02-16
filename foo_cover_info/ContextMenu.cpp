#include "stdafx.hpp"

namespace cinfo
{
	struct ContextItem
	{
		const GUID* guid;
		const pfc::string8 name;
	};

	static const std::vector<ContextItem> context_items =
	{
		{ &guid_context_scan, "Scan for Cover Info" },
		{ &guid_context_clear, "Clear Cover Info" },
	};

	class ContextMenu : public contextmenu_item_simple
	{
	public:
		GUID get_item_guid(uint32_t index) override
		{
			return *context_items[index].guid;
		}

		GUID get_parent() override
		{
			return guid_context_group;
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
			return context_items.size();
		}

		void context_command(uint32_t index, metadb_handle_list_cref handles, const GUID& caller) override
		{
			if (index == 0)
			{
				auto cb = fb2k::service_new<CoverInfo>(handles);
				static constexpr uint32_t flags = threaded_process::flag_show_progress | threaded_process::flag_show_delayed | threaded_process::flag_show_item | threaded_process::flag_show_abort;
				threaded_process::get()->run_modeless(cb, flags, core_api::get_main_window(), "Scanning for covers...");
			}
			else if (index == 1)
			{
				reset(handles);
			}
		}

		void get_item_name(uint32_t index, pfc::string_base& out) override
		{
			out = context_items[index].name;
		}
	};

	static contextmenu_group_popup_factory g_context_group(guid_context_group, contextmenu_groups::root, component_name, 0);
	FB2K_SERVICE_FACTORY(ContextMenu);
}
