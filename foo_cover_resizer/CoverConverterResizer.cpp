#include "stdafx.hpp"

using namespace resizer;

CoverConverterResizer::CoverConverterResizer(Action action, metadb_handle_list_cref handles, Format format, const GUID& art_guid) : m_action(action), m_handles(handles), m_format(format), m_art_guid(art_guid) {}

void CoverConverterResizer::on_done(HWND, bool was_aborted)
{
	if (!was_aborted && m_art_guid == album_art_ids::cover_front)
	{
		standard_commands::run_context(guid_foo_cover_info_command, m_handles);
	}
}

void CoverConverterResizer::run(threaded_process_status& status, abort_callback& abort)
{
	album_art_editor::ptr editor_ptr;
	album_art_extractor::ptr extractor_ptr;
	auto lock_api = file_lock_manager::get();

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
			album_art_data_ptr data = extractor_ptr->open(nullptr, path, abort)->query(m_art_guid, abort);
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
			aaep->set(m_art_guid, data, abort);
			aaep->commit(abort);
			success++;
		}
		catch (...) {}
	}
	FB2K_console_formatter() << component_name << ": " << success << " files were updated.";
}
