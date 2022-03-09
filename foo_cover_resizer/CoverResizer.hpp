#pragma once

using resizer::Format;

class CoverResizer : public threaded_process_callback
{
public:
	CoverResizer(metadb_handle_list_cref handles, Format format, bool convert_only = false);

	void run(threaded_process_status& status, abort_callback& abort) override;

private:
	Format m_format = Format::JPG;
	bool m_convert_only{};
	metadb_handle_list m_handles;
};
