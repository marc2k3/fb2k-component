#pragma once

class CoverResizer : public threaded_process_callback
{
public:
	CoverResizer(metadb_handle_list_cref handles, const GUID& what, bool convert_only = false);

	static MimeCLSID get_clsid(const std::string& str);
	static album_art_data_ptr istream_to_data(IStream* stream);
	static bool resize(double max_size, const std::unique_ptr<Gdiplus::Image>& source, std::unique_ptr<Gdiplus::Bitmap>& out);

	void run(threaded_process_status& status, abort_callback& abort) override;

private:
	inline static std::map<std::string, CLSID> s_encoder_map;

	GUID m_what{};
	MimeCLSID m_clsid_jpeg{}, m_clsid_png{};
	bool m_convert_only = false;
	metadb_handle_list m_handles;
};
