#pragma once

class CoverRemover : public threaded_process_callback
{
public:
	CoverRemover(metadb_handle_list_cref handles);

	void run(threaded_process_status& status, abort_callback& abort) override;

private:
	metadb_handle_list m_handles;
};
