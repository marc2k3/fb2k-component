#pragma once

class CoverResizer : public threaded_process_callback
{
public:
	CoverResizer(metadb_handle_list_cref handles, const GUID& what, int format, int size);
	CoverResizer(metadb_handle_list_cref handles, const GUID& what);

	void run(threaded_process_status& status, abort_callback& abort) override;

private:
	std::unique_ptr<Gdiplus::Bitmap> resize(const std::unique_ptr<Gdiplus::Image>& source, int width, int height);

	inline static std::map<std::string, CLSID> s_encoder_map;

	CLSID m_clsid_jpeg{}, m_clsid_png{};
	GUID m_what{};
	metadb_handle_list m_handles;
	int m_format = 0, m_size = 0;
};
