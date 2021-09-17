#pragma once

class CoverAttach : public threaded_process_callback
{
public:
	CoverAttach(metadb_handle_list_cref handles, const album_art_data_ptr& data) : m_handles(handles), m_data(data) {}

	void run(threaded_process_status& status, abort_callback& abort) override
	{
		auto api = file_lock_manager::get();
		const GUID what = resizer::prefs::get_guid();
		const uint32_t count = m_handles.get_count();
		std::set<pfc::string8> paths;

		for (const uint32_t i : std::views::iota(0U, count))
		{
			abort.check();

			const pfc::string8 path = m_handles[i]->get_path();
			if (!paths.emplace(path).second) continue;

			status.set_progress(i + 1, count);
			status.set_item_path(path);

			album_art_editor::ptr ptr;
			if (album_art_editor::g_get_interface(ptr, path))
			{
				try
				{
					auto lock = api->acquire_write(path, abort);
					album_art_editor_instance_ptr aaep = ptr->open(nullptr, path, abort);
					aaep->set(what, m_data, abort);
					aaep->commit(abort);
				}
				catch (...) {}
			}
		}
	}

private:
	album_art_data_ptr m_data;
	metadb_handle_list m_handles;
};
