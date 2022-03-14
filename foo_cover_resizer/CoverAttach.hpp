#pragma once

class CoverAttach : public threaded_process_callback
{
public:
	CoverAttach(metadb_handle_list_cref handles, const album_art_data_ptr& data, const GUID& art_guid) : m_handles(handles), m_data(data), m_art_guid(art_guid) {}

	void on_done(HWND, bool was_aborted) override
	{
		if (was_aborted) return;

		standard_commands::run_context(resizer::guid_foo_cover_info_command, m_handles);
	}

	void run(threaded_process_status& status, abort_callback& abort) override
	{
		auto api = file_lock_manager::get();
		const size_t count = m_handles.get_count();
		std::set<pfc::string8> paths;

		for (const size_t i : std::views::iota(0U, count))
		{
			abort.check();

			const pfc::string8 path = m_handles[i]->get_path();
			if (!paths.emplace(path).second) continue;

			status.set_progress(i + 1, count);
			status.set_item_path(path);

			album_art_editor::ptr ptr;
			if (!album_art_editor::g_get_interface(ptr, path)) continue;

			try
			{
				auto lock = api->acquire_write(path, abort);
				album_art_editor_instance_ptr aaep = ptr->open(nullptr, path, abort);
				aaep->set(m_art_guid, m_data, abort);
				aaep->commit(abort);
			}
			catch (...) {}
		}
	}

private:
	GUID m_art_guid{};
	album_art_data_ptr m_data;
	metadb_handle_list m_handles;
};
