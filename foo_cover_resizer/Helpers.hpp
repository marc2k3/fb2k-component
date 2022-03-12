#pragma once

#ifdef NDEBUG
#include <libwebp/encode.h>
#pragma comment(lib, "../foobar2000-sdk/lib/libwebp/libwebp.lib")
#endif

namespace resizer
{
	static HRESULT decode(IStream* stream, wil::com_ptr_t<IWICBitmapSource>& source)
	{
		wil::com_ptr_t<IWICBitmapDecoder> decoder;
		wil::com_ptr_t<IWICBitmapFrameDecode> frame_decode;

		RETURN_IF_FAILED(g_imaging_factory->CreateDecoderFromStream(stream, nullptr, WICDecodeMetadataCacheOnDemand, &decoder));
		RETURN_IF_FAILED(decoder->GetFrame(0, &frame_decode));
		RETURN_IF_FAILED(WICConvertBitmapSource(GUID_WICPixelFormat32bppPBGRA, frame_decode.get(), &source));
		return S_OK;
	}

	static HRESULT encode_webp(IWICBitmapSource* source, const WICRect& rect, album_art_data_ptr& data)
	{
#ifdef NDEBUG
		uint8_t* ptr = nullptr;
		uint8_t* output = nullptr;
		uint32_t size{}, stride{};
		wil::com_ptr_t<IWICBitmap> bitmap;
		wil::com_ptr_t<IWICBitmapLock> lock;

		RETURN_IF_FAILED(g_imaging_factory->CreateBitmapFromSource(source, WICBitmapCacheOnDemand, &bitmap));
		RETURN_IF_FAILED(bitmap->Lock(&rect, WICBitmapLockWrite, &lock));
		RETURN_IF_FAILED(lock->GetDataPointer(&size, &ptr));
		RETURN_IF_FAILED(lock->GetStride(&stride));

		const size_t new_size = WebPEncodeBGRA(ptr, rect.Width, rect.Height, static_cast<int>(stride), 95.f, &output);
		if (new_size > 0)
		{
			data = album_art_data_impl::g_create(output, new_size);
			WebPFree(output);
			return S_OK;
		}
#endif
		return E_FAIL;
	}


	static HRESULT encode(Format format, IWICBitmapSource* source, album_art_data_ptr& data)
	{
		data.reset();

		uint32_t width{}, height{};
		RETURN_IF_FAILED(source->GetSize(&width, &height));
		WICRect rect(0, 0, static_cast<int>(width), static_cast<int>(height));

		if (format == Format::WEBP)
		{
			return encode_webp(source, rect, data);
		}

		wil::com_ptr_t<IStream> stream;
		wil::com_ptr_t<IWICBitmapEncoder> encoder;
		wil::com_ptr_t<IWICBitmapFrameEncode> frame_encode;
		wil::com_ptr_t<IWICStream> wic_stream;

		RETURN_IF_FAILED(CreateStreamOnHGlobal(nullptr, TRUE, &stream));
		RETURN_IF_FAILED(g_imaging_factory->CreateStream(&wic_stream));
		RETURN_IF_FAILED(wic_stream->InitializeFromIStream(stream.get()));
		RETURN_IF_FAILED(g_imaging_factory->CreateEncoder(format == Format::JPG ? GUID_ContainerFormatJpeg : GUID_ContainerFormatPng, nullptr, &encoder));
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

	static HRESULT resize(IWICBitmapSource* source, wil::com_ptr_t<IWICBitmapScaler>& scaler)
	{
		uint32_t old_width{}, old_height{};
		RETURN_IF_FAILED(source->GetSize(&old_width, &old_height));

		const double dmax = static_cast<double>(settings::size);
		const double dw = static_cast<double>(old_width);
		const double dh = static_cast<double>(old_height);
		if (dw <= dmax && dh <= dmax) return E_FAIL; // nothing to do

		const double s = std::min(dmax / dw, dmax / dh);
		const uint32_t new_width = static_cast<uint32_t>(dw * s);
		const uint32_t new_height = static_cast<uint32_t>(dh * s);

		RETURN_IF_FAILED(g_imaging_factory->CreateBitmapScaler(&scaler));
		RETURN_IF_FAILED(scaler->Initialize(source, new_width, new_height, WICBitmapInterpolationModeFant));
		return S_OK;
	}

	static bool api_check()
	{
		if (g_imaging_factory)
			return true;

		popup_message::g_show(wic_error, component_name);
		return false;
	}

	static bool browse_for_image(HWND parent, const char* folder, wil::com_ptr_t<IWICBitmapSource>& source)
	{
		pfc::string8 path;
		if (uGetOpenFileName(parent, "Picture files|*.jpg;*.jpeg;*.png;*.bmp;*.gif;*.tiff;*.webp", 0, nullptr, "Browse for image", folder, path, FALSE) == FALSE) return false;
		auto wpath = string_wide_from_utf8_fast(path);

		wil::com_ptr_t<IStream> stream;
		if (FAILED(SHCreateStreamOnFileEx(wpath, STGM_READ | STGM_SHARE_DENY_WRITE, GENERIC_READ, FALSE, nullptr, &stream))) return false;
		if (FAILED(decode(stream.get(), source)))
		{
			popup_message::g_show("Unable to decode selected file.", component_name);
			return false;
		}
		return true;
	}

	static bool choose_settings(HWND parent, bool show_size)
	{
		modal_dialog_scope scope;
		if (scope.can_create())
		{
			scope.initialize(parent);
			CDialogSettings dlg(show_size);
			return dlg.DoModal(parent) == IDOK;
		}
		return false;
	}
}
