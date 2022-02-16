#include "stdafx.hpp"

using namespace resizer;

CoverResizer::CoverResizer(metadb_handle_list_cref handles, bool convert_only) : m_handles(handles), m_convert_only(convert_only) {}

MimeCLSID CoverResizer::get_clsid(const char* str)
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
					s_encoder_map.emplace(string_utf8_from_wide(codec.MimeType).get_ptr(), codec.Clsid);
				}
			}
		}
	}

	const auto it = s_encoder_map.find(str);
	if (it != s_encoder_map.end())
	{
		return it->second;
	}
	return std::nullopt;
}

album_art_data_ptr CoverResizer::istream_to_data(IStream* stream)
{
	album_art_data_ptr data;
	HGLOBAL hg = nullptr;
	if (FAILED(GetHGlobalFromStream(stream, &hg))) return data;
	const ULONG new_size = GlobalSize(hg);
	LPVOID pimage = GlobalLock(hg);
	std::vector<uint8_t> buffer(new_size);
	memcpy(buffer.data(), pimage, buffer.size());
	GlobalUnlock(hg);

	data = album_art_data_impl::g_create(buffer.data(), buffer.size());
	return data;
}

bool CoverResizer::resize(double max_size, const std::unique_ptr<Gdiplus::Image>& source, std::unique_ptr<Gdiplus::Bitmap>& out)
{
	const double dw = static_cast<double>(source->GetWidth());
	const double dh = static_cast<double>(source->GetHeight());
	if (dw <= max_size && dh <= max_size) return false; // bail if source image is already smaller than specified max_size

	const double s = std::min(max_size / dw, max_size / dh);
	const int new_width = static_cast<int>(dw * s);
	const int new_height = static_cast<int>(dh * s);

	out = std::make_unique<Gdiplus::Bitmap>(new_width, new_height, PixelFormat32bppPARGB);
	Gdiplus::Graphics g(out.get());
	g.SetInterpolationMode(Gdiplus::InterpolationMode::InterpolationModeHighQuality);
	g.DrawImage(source.get(), 0, 0, new_width, new_height);
	return true;
}

void CoverResizer::run(threaded_process_status& status, abort_callback& abort)
{
	auto image_api = fb2k::imageLoaderLite::tryGet();
	if (image_api.is_empty())
	{
		popup_message::g_show(image_loader_error, group_resize);
		return;
	}

	m_clsid_jpeg = get_clsid(mime_jpeg);
	m_clsid_png = get_clsid(mime_png);

	if (!m_clsid_jpeg.has_value() || !m_clsid_png.has_value()) // even if JPG/PNG were not requested, these failing would indicate something very wrong that would affect others too
	{
		popup_message::g_show(image_clsid_error, group_resize);
		return;
	}

	album_art_editor::ptr editor_ptr;
	album_art_extractor::ptr extractor_ptr;
	auto lock_api = file_lock_manager::get();

	const GUID what = m_convert_only ? album_art_ids::cover_front : settings::get_guid();
	const uint32_t count = m_handles.get_count();
	const double dmax = static_cast<double>(settings::size.get_value());
	const int format = settings::format.get_value();
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
			album_art_data_ptr data = extractor_ptr->open(nullptr, path, abort)->query(what, abort);
			if (data.is_empty()) continue;

			fb2k::imageInfo_t info;
			std::unique_ptr<Gdiplus::Image> image(image_api->load(data, &info, abort));
			pfc::com_ptr_t<IStream> stream;
			if (FAILED(CreateStreamOnHGlobal(nullptr, TRUE, stream.receive_ptr()))) continue;

			if (m_convert_only)
			{
				if (info.formatName == nullptr || _stricmp(info.formatName, "jpeg") == 0 || _stricmp(info.formatName, "jpg") == 0) continue;
				if (image->Save(stream.get_ptr(), &m_clsid_jpeg.value()) != Gdiplus::Ok) continue;
			}
			else
			{
				MimeCLSID clsid{};
				switch (format)
				{
				case 0: // maintain original
					clsid = get_clsid(info.mime);
					if (!clsid.has_value())
					{
						FB2K_console_formatter() << "Cannot resize image found in " << path << ".";
						FB2K_console_formatter() << "Type not supported: " << info.mime;
						continue;
					}
					break;
				case 1:
					clsid = m_clsid_jpeg;
					break;
				case 2:
					clsid = m_clsid_png;
					break;
				}

				std::unique_ptr<Gdiplus::Bitmap> resized;
				if (!resize(dmax, image, resized)) continue;
				if (resized->GetLastStatus() != Gdiplus::Ok) continue;
				if (resized->Save(stream.get_ptr(), &clsid.value()) != Gdiplus::Ok) continue;
			}

			data = istream_to_data(stream.get_ptr());
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
	FB2K_console_formatter() << group_resize << ": " << success << " files were updated.";
}
