#include "stdafx.hpp"

using namespace resizer;

CoverConverterResizer::CoverConverterResizer(Action action, metadb_handle_list_cref handles, Format format) : m_action(action), m_handles(handles), m_format(format) {}

void CoverConverterResizer::run(threaded_process_status& status, abort_callback& abort)
{
	album_art_editor::ptr editor_ptr;
	album_art_extractor::ptr extractor_ptr;
	auto lock_api = file_lock_manager::get();

	const GUID what = m_action == Action::convert ? album_art_ids::cover_front : settings::get_guid();
	const size_t count = m_handles.get_count();
	std::set<pfc::string8> paths;
	uint32_t success{};

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
			wil::com_ptr_t<IWICBitmapSource> source;
			stream.attach(SHCreateMemStream(ptr, bytes));

			if (FAILED(decode(stream.get(), source)))
			{
				FB2K_console_formatter() << component_name << ": Unable to decode image data found in: " << path;
				continue;
			}

			if (m_action == Action::convert)
			{
				if (FAILED(encode(m_format, source.get(), data))) continue;
			}
			else
			{
				wil::com_ptr_t<IWICBitmapScaler> scaler;
				if (FAILED(resize(source.get(), scaler))) continue;
				if (FAILED(encode(m_format, scaler.get(), data))) continue;
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
