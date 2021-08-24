#pragma once

class CoverRemover : public threaded_process_callback
{
public:
	CoverRemover(metadb_handle_list_cref handles) : m_handles(handles) {}

	void run(threaded_process_status& status, abort_callback& abort) override
	{
		auto api = file_lock_manager::get();
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

					aaep->remove(album_art_ids::artist);
					aaep->remove(album_art_ids::cover_back);
					aaep->remove(album_art_ids::disc);
					aaep->remove(album_art_ids::icon);
					aaep->commit(abort);
				}
				catch (...) {}
			}
		}
	}

private:
	metadb_handle_list m_handles;
};
