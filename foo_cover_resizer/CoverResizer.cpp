#include "stdafx.h"

using namespace resizer;

CoverResizer::CoverResizer(metadb_handle_list_cref handles, const GUID& what, int format, int size) : m_handles(handles), m_what(what), m_format(format), m_size(size) {}
CoverResizer::CoverResizer(metadb_handle_list_cref handles, const GUID& what) : m_handles(handles), m_what(what) {}

std::unique_ptr<Gdiplus::Bitmap> CoverResizer::resize(const std::unique_ptr<Gdiplus::Image>& source, int width, int height)
{
	auto bitmap = std::make_unique<Gdiplus::Bitmap>(width, height, PixelFormat32bppPARGB);
	Gdiplus::Graphics g(bitmap.get());
	g.SetInterpolationMode(Gdiplus::InterpolationMode::InterpolationModeHighQuality);
	g.DrawImage(source.get(), 0, 0, width, height);
	return bitmap;
}

void CoverResizer::run(threaded_process_status& status, abort_callback& abort)
{
	if (s_encoder_map.empty())
	{
		uint32_t num = 0, size = 0;
		if (Gdiplus::GetImageEncodersSize(&num, &size) == Gdiplus::Ok && size > 0)
		{
			std::vector<Gdiplus::ImageCodecInfo> codecs(size);
			if (Gdiplus::GetImageEncoders(num, size, codecs.data()) == Gdiplus::Ok)
			{
				for (const auto& codec : std::ranges::views::take(codecs, num))
				{
					s_encoder_map.emplace(pfc::stringcvt::string_utf8_from_wide(codec.MimeType).get_ptr(), codec.Clsid);
				}
			}
		}
	}

	auto image_api = fb2k::imageLoaderLite::tryGet();
	if (image_api.is_empty())
	{
		popup_message::g_show("This component requires foobar2000 v1.6 or later.", group_resize);
		return;
	}

	const bool convert_mode = m_size == 0; // not supplying a size means we're in convert to JPG mode

	if (convert_mode)
	{
		const auto& it = s_encoder_map.find(mime_jpeg);
		if (it == s_encoder_map.end()) // should never ever happen
		{
			popup_message::g_show("Internal error. Unable to determine CLSID required to save as JPG.", group_resize);
			return;
		}
		m_clsid_jpeg = it->second;
	}

	album_art_editor::ptr editor_ptr;
	album_art_extractor::ptr extractor_ptr;
	auto lock_api = file_lock_manager::get();

	const uint32_t count = m_handles.get_count();
	const double dmax = static_cast<double>(m_size);
	std::set<pfc::string8> paths;
	uint32_t success = 0;

	for (const uint32_t i : std::views::iota(0U, count))
	{
		abort.check();

		const pfc::string8 path = m_handles[i]->get_path();
		if (!paths.emplace(path).second) continue;

		status.set_progress(i + 1, count);
		status.set_item_path(path);

		if (!album_art_extractor::g_get_interface(extractor_ptr, path)) continue;

		try
		{
			album_art_data_ptr data = extractor_ptr->open(nullptr, path, abort)->query(m_what, abort);
			if (data.is_empty()) continue;

			fb2k::imageInfo_t info;
			std::unique_ptr<Gdiplus::Image> image(image_api->load(data, &info, abort));
			pfc::com_ptr_t<IStream> stream;
			if (FAILED(CreateStreamOnHGlobal(nullptr, TRUE, stream.receive_ptr()))) continue;

			if (convert_mode)
			{
				if (info.formatName == nullptr || _stricmp(info.formatName, "jpeg") == 0 || _stricmp(info.formatName, "jpg") == 0) continue;
				if (image->Save(stream.get_ptr(), &m_clsid_jpeg) != Gdiplus::Ok) continue;
			}
			else
			{
				const double dw = static_cast<double>(info.width);
				const double dh = static_cast<double>(info.height);
				if (dw <= dmax && dh <= dmax) continue;
				const double s = std::min(dmax / dw, dmax / dh);
				const int new_width = static_cast<int>(dw * s);
				const int new_height = static_cast<int>(dh * s);

				std::string mime;
				switch (m_format)
				{
				case 0:
					mime = info.mime; 
					break;
				case 1:
					mime = mime_jpeg;
					break;
				case 2:
					mime = mime_png;
					break;
				}

				if (!s_encoder_map.contains(mime)) // using gdiplus so writing webp is not supported
				{
					FB2K_console_formatter() << "Cannot resize image found in " << path << ".";
					FB2K_console_formatter() << "Type not supported: " << mime.c_str();
					continue;
				}

				auto resized = resize(image, new_width, new_height);
				if (!resized || resized->GetLastStatus() != Gdiplus::Ok) continue;
				if (resized->Save(stream.get_ptr(), &s_encoder_map.at(mime)) != Gdiplus::Ok) continue;
			}

			HGLOBAL hg = nullptr;
			if (FAILED(GetHGlobalFromStream(stream.get_ptr(), &hg))) continue;
			const ULONG new_size = GlobalSize(hg);
			LPVOID pimage = GlobalLock(hg);
			std::vector<uint8_t> buffer(new_size);
			memcpy(buffer.data(), pimage, buffer.size());
			GlobalUnlock(hg);

			data = album_art_data_impl::g_create(buffer.data(), buffer.size());
			if (data.is_empty()) continue;

			auto lock = lock_api->acquire_write(path, abort);
			album_art_editor::g_get_interface(editor_ptr, path);
			album_art_editor_instance_ptr aaep = editor_ptr->open(nullptr, path, abort);
			aaep->set(m_what, data, abort);
			aaep->commit(abort);
			success++;
		}
		catch (...) {}
	}
	FB2K_console_formatter() << group_resize << ": " << success << " files were updated.";
}
