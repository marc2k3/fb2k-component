#include "stdafx.hpp"

extern wil::com_ptr_nothrow<IWICImagingFactory> g_imaging_factory;

using namespace resizer;

CoverResizer::CoverResizer(metadb_handle_list_cref handles, bool convert_only) : m_handles(handles), m_convert_only(convert_only) {}

HRESULT CoverResizer::decode(IStream* stream, wil::com_ptr_t<IWICBitmapSource>& source)
{
	wil::com_ptr_t<IWICBitmapDecoder> decoder;
	wil::com_ptr_t<IWICBitmapFrameDecode> frame_decode;

	RETURN_IF_FAILED(g_imaging_factory->CreateDecoderFromStream(stream, nullptr, WICDecodeMetadataCacheOnDemand, &decoder));
	RETURN_IF_FAILED(decoder->GetFrame(0, &frame_decode));
	RETURN_IF_FAILED(WICConvertBitmapSource(GUID_WICPixelFormat32bppPBGRA, frame_decode.get(), &source));
	return S_OK;
}

HRESULT CoverResizer::encode(const GUID& container, IWICBitmapSource* source, album_art_data_ptr& data)
{
	data.reset();
	uint32_t width{}, height{};
	RETURN_IF_FAILED(source->GetSize(&width, &height));
	WICRect rect(0, 0, width, height);

	wil::com_ptr_t<IStream> stream;
	wil::com_ptr_t<IWICBitmapEncoder> encoder;
	wil::com_ptr_t<IWICBitmapFrameEncode> frame_encode;
	wil::com_ptr_t<IWICStream> wic_stream;

	RETURN_IF_FAILED(CreateStreamOnHGlobal(nullptr, TRUE, &stream));
	RETURN_IF_FAILED(g_imaging_factory->CreateStream(&wic_stream));
	RETURN_IF_FAILED(wic_stream->InitializeFromIStream(stream.get()));
	RETURN_IF_FAILED(g_imaging_factory->CreateEncoder(container, nullptr, &encoder));
	RETURN_IF_FAILED(encoder->Initialize(wic_stream.get(), WICBitmapEncoderNoCache));
	RETURN_IF_FAILED(encoder->CreateNewFrame(&frame_encode, nullptr));
	RETURN_IF_FAILED(frame_encode->Initialize(nullptr));
	RETURN_IF_FAILED(frame_encode->SetSize(width, height));
	RETURN_IF_FAILED(frame_encode->WriteSource(source, &rect));
	RETURN_IF_FAILED(frame_encode->Commit());
	RETURN_IF_FAILED(encoder->Commit());

	HGLOBAL hg = nullptr;
	RETURN_IF_FAILED(GetHGlobalFromStream(stream.get(), &hg));
	auto image = wil::unique_hglobal_locked(hg);
	auto size = GlobalSize(image.get());
	std::vector<uint8_t> buffer(size);
	memcpy(buffer.data(), image.get(), buffer.size());

	data = album_art_data_impl::g_create(buffer.data(), buffer.size());
	return S_OK;
}

HRESULT CoverResizer::resize(IStream* stream, wil::com_ptr_t<IWICBitmapScaler>& scaler)
{
	uint32_t old_width{}, old_height{};
	wil::com_ptr_t<IWICBitmapSource> source;
	RETURN_IF_FAILED(decode(stream, source));
	RETURN_IF_FAILED(source->GetSize(&old_width, &old_height));

	const double dmax = static_cast<double>(settings::size.get_value());
	const double dw = static_cast<double>(old_width);
	const double dh = static_cast<double>(old_height);
	if (dw <= dmax && dh <= dmax) return E_FAIL; // nothing to do

	const double s = std::min(dmax / dw, dmax / dh);
	const uint32_t new_width = static_cast<uint32_t>(dw * s);
	const uint32_t new_height = static_cast<uint32_t>(dh * s);

	RETURN_IF_FAILED(g_imaging_factory->CreateBitmapScaler(&scaler));
	RETURN_IF_FAILED(scaler->Initialize(source.get(), new_width, new_height, WICBitmapInterpolationModeFant));
	return S_OK;
}

void CoverResizer::run(threaded_process_status& status, abort_callback& abort)
{
	album_art_editor::ptr editor_ptr;
	album_art_extractor::ptr extractor_ptr;
	auto lock_api = file_lock_manager::get();

	const GUID what = m_convert_only ? album_art_ids::cover_front : settings::get_guid();
	const size_t count = m_handles.get_count();
	std::set<pfc::string8> paths;
	uint32_t success{};

	const int format = settings::format.get_value();
	GUID container{};
	if (m_convert_only || format == 0)
	{
		container = GUID_ContainerFormatJpeg;
	}
	else
	{
		container = GUID_ContainerFormatPng;
	}

	for (const size_t i : std::views::iota(0U, count))
	{
		abort.check();

		const pfc::string8 path = m_handles[i]->get_path();
		if (!paths.emplace(path).second) continue;

		status.set_progress(i + 1, count);
		status.set_item_path(path);

		if (!album_art_extractor::g_get_interface(extractor_ptr, path)) continue;

		try
		{
			album_art_data_ptr data = extractor_ptr->open(nullptr, path, abort)->query(what, abort);
			if (data.is_empty()) continue;

			const uint8_t* ptr = static_cast<const uint8_t*>(data->get_ptr());
			const uint32_t bytes = pfc::downcast_guarded<uint32_t>(data->get_size());
			wil::com_ptr_t<IStream> stream;
			stream.attach(SHCreateMemStream(ptr, bytes));

			if (m_convert_only)
			{
				wil::com_ptr_t<IWICBitmapSource> source;
				if (FAILED(decode(stream.get(), source))) continue;
				if (FAILED(encode(container, source.get(), data))) continue;
			}
			else
			{
				wil::com_ptr_t<IWICBitmapScaler> scaler;
				if (FAILED(resize(stream.get(), scaler))) continue;
				if (FAILED(encode(container, scaler.get(), data))) continue;
			}

			if (data.is_empty()) continue;
			auto lock = lock_api->acquire_write(path, abort);
			album_art_editor::g_get_interface(editor_ptr, path);
			album_art_editor_instance_ptr aaep = editor_ptr->open(nullptr, path, abort);
			aaep->set(what, data, abort);
			aaep->commit(abort);
			success++;
		}
		catch (...) {}
	}
	FB2K_console_formatter() << component_name << ": " << success << " files were updated.";
}
