#pragma once

class CoverAttach : public threaded_process_callback
{
public:
	CoverAttach(metadb_handle_list_cref handles, const album_art_data_ptr& data, const GUID& art_guid);

	void on_done(HWND, bool was_aborted) override;
	void run(threaded_process_status& status, abort_callback& abort) override;

private:
	GUID m_art_guid{};
	album_art_data_ptr m_data;
	metadb_handle_list m_handles;
};
