#define _WIN32_WINNT _WIN32_WINNT_WIN7
#define WINVER _WIN32_WINNT_WIN7

#include <ranges>
#include <foobar2000/SDK/foobar2000.h>

namespace
{
	static constexpr const char* component_name = "Run Main";

	DECLARE_COMPONENT_VERSION(
		component_name,
		"0.0.1",
		"Copyright (C) 2022 marc2003\n\n"
		"Build: " __TIME__ ", " __DATE__
	);

	VALIDATE_COMPONENT_FILENAME("foo_run_main.dll");

	using pfc::string8;
	using pfc::stringp;

	class MainMenuCommand
	{
	public:
		MainMenuCommand(stringp command) : m_command(command)
		{
			if (s_group_guid_map.empty())
			{
				for (auto e = service_enum_t<mainmenu_group>(); !e.finished(); ++e)
				{
					auto ptr = *e;
					s_group_guid_map.emplace(hash_guid(ptr->get_guid()), ptr);
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

				const string8 parent_path = build_parent_path(ptr->get_parent());
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
						string8 name;
						ptr->get_name(i, name);

						string8 path = parent_path;
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
		bool execute_recur(mainmenu_node::ptr node, stringp parent_path)
		{
			string8 text;
			uint32_t flags;
			node->get_display(text, flags);

			string8 path = parent_path.get_ptr();
			path.add_string(text);

			switch (node->get_type())
			{
			case mainmenu_node::type_group:
				{
					const uint32_t count = node->get_children_count();
					path.end_with('/');

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

		bool match_command(stringp what)
		{
			return _stricmp(m_command, what) == 0;
		}

		string8 build_parent_path(GUID parent)
		{
			string8 path;
			while (parent != pfc::guid_null)
			{
				mainmenu_group::ptr group_ptr = s_group_guid_map.at(hash_guid(parent));
				mainmenu_group_popup::ptr group_popup_ptr;

				if (group_ptr->cast(group_popup_ptr))
				{
					string8 str;
					group_popup_ptr->get_display_string(str);
					str.add_char('/');
					str.add_string(path);
					path = str;
				}
				parent = group_ptr->get_parent();
			}
			return path;
		};

		uint64_t hash_guid(const GUID& g)
		{
			return hasher_md5::get()->process_single_string(pfc::print_guid(g)).xorHalve();
		}

		inline static std::unordered_map<uint64_t, mainmenu_group::ptr> s_group_guid_map;
		string8 m_command;
	};

	class CommandLineHandler : public commandline_handler
	{
	public:
		result on_token(const char* token) override
		{
			pfc::string s(token);

			if (s.startsWith("/run_main:"))
			{
				string8 command = s.subString(10).get_ptr();
				if (!MainMenuCommand(command).execute())
				{
					FB2K_console_formatter() << component_name << ": Command not found: " << command;
				}

				return RESULT_PROCESSED;
			}

			return RESULT_NOT_OURS;
		}
	};

	FB2K_SERVICE_FACTORY(CommandLineHandler)
};
