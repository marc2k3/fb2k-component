#pragma once

namespace cinfo
{
	struct Fields
	{
		uint32_t front_cover_width = 0;
		uint32_t front_cover_height = 0;
		uint32_t front_cover_bytes = 0;
		pfc::string8 front_cover_format;
	};

	using HashList = pfc::list_t<metadb_index_hash>;
	using HashSet = std::set<metadb_index_hash>;

	Fields get(metadb_index_hash hash);
	bool hashHandle(const metadb_handle_ptr& handle, metadb_index_hash& hash);
	metadb_index_hash generate_hash(file_path_display path);
	metadb_index_manager::ptr theAPI();
	void refresh(const HashList& hashes);
	void reset(metadb_handle_list_cref handles);
	void set(metadb_index_hash hash, Fields f);
}
