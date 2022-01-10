#define _WIN32_WINNT _WIN32_WINNT_WIN7
#define WINVER _WIN32_WINNT_WIN7

#include <algorithm>
#include <ranges>
#include <string>
#include <thread>

#include <foobar2000/SDK/foobar2000.h>
#include <Shlwapi.h>

#pragma comment(lib, "Shlwapi.lib")

namespace
{
	static constexpr const char* component_name = "Run Main";

	DECLARE_COMPONENT_VERSION(
		component_name,
		"0.0.4",
		"Copyright (C) 2022 marc2003\n\n"
		"Build: " __TIME__ ", " __DATE__
	);

	VALIDATE_COMPONENT_FILENAME("foo_run_main.dll");

	class MainMenuCommand
	{
	public:
		MainMenuCommand(const char* command) : m_command(command)
		{
			if (s_group_guid_map.empty())
			{
				for (auto e = service_enum_t<mainmenu_group>(); !e.finished(); ++e)
				{
					auto ptr = *e;
					s_group_guid_map.emplace(ptr->get_guid(), ptr);
				}
			}
		}

		bool execute()
		{
			// Ensure commands on the Edit menu are enabled
			ui_edit_context_manager::get()->set_context_active_playlist();

			for (auto e = service_enum_t<mainmenu_commands>(); !e.finished(); ++e)
			{
				auto ptr = *e;
				mainmenu_commands_v2::ptr v2_ptr;
				ptr->cast(v2_ptr);

				const pfc::string8 parent_path = build_parent_path(ptr->get_parent());
				const uint32_t count = ptr->get_command_count();

				for (const uint32_t i : std::views::iota(0U, count))
				{
					if (v2_ptr.is_valid() && v2_ptr->is_command_dynamic(i))
					{
						mainmenu_node::ptr node = v2_ptr->dynamic_instantiate(i);
						if (execute_recur(node, parent_path))
						{
							return true;
						}
					}
					else
					{
						pfc::string8 name;
						ptr->get_name(i, name);

						pfc::string8 path = parent_path;
						path.add_string(name);
						if (match_command(path))
						{
							ptr->execute(i, nullptr);
							return true;
						}
					}
				}
			}
			return false;
		}

	private:
		struct GuidHasher
		{
			uint64_t operator()(const GUID& g) const
			{
				return hasher_md5::get()->process_single_string(pfc::print_guid(g)).xorHalve();
			}
		};

		bool execute_recur(mainmenu_node::ptr node, const char* parent_path)
		{
			pfc::string8 text;
			uint32_t flags;
			node->get_display(text, flags);

			pfc::string8 path = parent_path;
			path.add_string(text);

			switch (node->get_type())
			{
			case mainmenu_node::type_group:
				{
					path.end_with('/');
					const uint32_t count = node->get_children_count();

					for (const uint32_t i : std::views::iota(0U, count))
					{
						mainmenu_node::ptr child = node->get_child(i);
						if (execute_recur(child, path))
						{
							return true;
						}
					}
					break;
				}
			case mainmenu_node::type_command:
				if (match_command(path))
				{
					node->execute(nullptr);
					return true;
				}
				break;
			}
			return false;
		}

		bool match_command(const char* what)
		{
			return _stricmp(m_command, what) == 0;
		}

		pfc::string8 build_parent_path(GUID parent)
		{
			pfc::string8 path;
			while (parent != pfc::guid_null)
			{
				mainmenu_group::ptr group_ptr = s_group_guid_map.at(parent);
				mainmenu_group_popup::ptr group_popup_ptr;

				if (group_ptr->cast(group_popup_ptr))
				{
					pfc::string8 str;
					group_popup_ptr->get_display_string(str);
					str.add_char('/');
					str.add_string(path);
					path = str;
				}
				parent = group_ptr->get_parent();
			}
			return path;
		};

		inline static std::unordered_map<GUID, mainmenu_group::ptr, GuidHasher> s_group_guid_map;
		pfc::string8 m_command;
	};

	class CommandLineHandler : public commandline_handler
	{
	public:
		result on_token(const char* token) override
		{
			pfc::string8 s = token;

			if (s.startsWith("/run_main:"))
			{
				pfc::string8 command = s.subString(10);
				if (!MainMenuCommand(command).execute())
				{
					FB2K_console_formatter() << component_name << ": Command not found: " << command;
				}

				return RESULT_PROCESSED;
			}
			else if (s.startsWith("/select_item:") || s.startsWith("/select_item_and_play:"))
			{
				const uint32_t first_colon = s.find_first(':');
				const uint32_t second_colon = s.find_last(':');
				const bool play = first_colon == 21;

				pfc::string8 item_text = s.subString(first_colon + 1, second_colon - first_colon - 1);
				if (pfc::string_is_numeric(item_text))
				{
					const uint32_t item_num = std::stoul(item_text.get_ptr());
					uint32_t delay_num = 0;

					if (second_colon < SIZE_MAX && second_colon != first_colon)
					{
						pfc::string8 delay_text = s.subString(second_colon + 1);
						if (pfc::string_is_numeric(delay_text))
						{
							delay_num = std::stoul(delay_text.get_ptr());
						}
					}
					
					if (delay_num == 0)
					{
						select_item(item_num, play);
					}
					else
					{
						auto t = std::thread([=]()
							{
								std::this_thread::sleep_for(std::chrono::milliseconds(delay_num));
								fb2k::inMainThread([=]()
									{
										select_item(item_num, play);
									});
							});

						t.detach();
					}
				}

				return RESULT_PROCESSED;
			}
			return RESULT_NOT_OURS;
		}

	private:
		void select_item(uint32_t item_num, bool play)
		{
			auto api = playlist_manager::get();
			const uint32_t playlistIndex = api->get_active_playlist();
			if (playlistIndex < api->get_playlist_count())
			{
				const uint32_t count = api->playlist_get_item_count(playlistIndex);
				if (count > 0)
				{
					const uint32_t playlistItemIndex = std::clamp<uint32_t>(item_num, 1, count) - 1;
					api->playlist_clear_selection(playlistIndex);
					api->playlist_set_selection_single(playlistIndex, playlistItemIndex, true);
					api->playlist_set_focus_item(playlistIndex, playlistItemIndex);
					if (play)
					{
						api->playlist_execute_default_action(playlistIndex, playlistItemIndex);
					}
				}
			}
		}
	};

	FB2K_SERVICE_FACTORY(CommandLineHandler)
};
