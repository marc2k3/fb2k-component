#pragma once

class CoverResizer : public threaded_process_callback
{
public:
	CoverResizer(metadb_handle_list_cref handles, bool convert_only = false);

	static HRESULT decode(IStream* stream, wil::com_ptr_t<IWICBitmapSource>& source);
	static HRESULT encode(const GUID& container, IWICBitmapSource* source, album_art_data_ptr& data);
	static HRESULT resize(IStream* stream, wil::com_ptr_t<IWICBitmapScaler>& scaler);

	void run(threaded_process_status& status, abort_callback& abort) override;

private:
	bool m_convert_only = false;
	metadb_handle_list m_handles;
};
