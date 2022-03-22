#pragma once

using resizer::Format;

class CoverConverterResizer : public threaded_process_callback
{
public:
	enum class Action
	{
		resize,
		convert,
	};

	CoverConverterResizer(Action action, metadb_handle_list_cref handles, Format format, const GUID& art_guid);

	void on_done(HWND, bool was_aborted) override;
	void run(threaded_process_status& status, abort_callback& abort) override;

private:
	Action m_action{};
	Format m_format{};
	GUID m_art_guid{};
	metadb_handle_list m_handles;
};
