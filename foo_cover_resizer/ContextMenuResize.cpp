#include "stdafx.h"

using namespace resizer;

class ContextMenuResize : public contextmenu_item_simple
{
public:
	GUID get_item_guid(uint32_t index) override
	{
		if (index == 0) return guid_context_command_resize;
		else if (index == 1) return guid_context_command_attach_and_resize;
		else uBugCheck();
	}

	GUID get_parent() override
	{
		return guid_context_group_resize;
	}

	bool context_get_display(uint32_t index, metadb_handle_list_cref handles, pfc::string_base& out, uint32_t& displayflags, const GUID& caller) override
	{
		get_item_name(index, out);
		return true;
	}

	bool get_item_description(uint32_t index, pfc::string_base& out) override
	{
		get_item_name(index, out);
		return true;
	}

	uint32_t get_num_items() override
	{
		return 2;
	}

	void context_command(uint32_t index, metadb_handle_list_cref handles, const GUID& caller) override
	{
		const HWND hwnd = core_api::get_main_window();

		if (index == 0)
		{
			if (popup(hwnd))
			{
				auto cb = fb2k::service_new<CoverResizer>(handles);
				threaded_process::get()->run_modeless(cb, threaded_process_flags, hwnd, "Resizing covers...");
			}
		}
		else if (index == 1)
		{
			fb2k::imageInfo_t info;
			std::string folder(pfc::string_directory(handles[0]->get_path()));
			std::unique_ptr<Gdiplus::Image> image;

			if (browse_for_image(hwnd, folder, info, image) && popup(hwnd))
			{
				const int format = prefs::format.get_value();
				MimeCLSID clsid{};
				if (format == 0) clsid = CoverResizer::get_clsid(info.mime);
				else if (format == 1) clsid = CoverResizer::get_clsid(mime_jpeg);
				else if (format == 2) clsid = CoverResizer::get_clsid(mime_png);

				if (clsid.has_value())
				{
					pfc::com_ptr_t<IStream> stream;
					if (FAILED(CreateStreamOnHGlobal(nullptr, TRUE, stream.receive_ptr()))) return;

					const double dmax = static_cast<double>(prefs::size.get_value());
					std::unique_ptr<Gdiplus::Bitmap> resized;
					if (CoverResizer::resize(dmax, image, resized)) // returns false if image is already smaller than specified max size
					{
						if (resized->GetLastStatus() != Gdiplus::Ok) return;
						if (resized->Save(stream.get_ptr(), &clsid.value()) != Gdiplus::Ok) return;
					}
					else
					{
						if (image->Save(stream.get_ptr(), &clsid.value()) != Gdiplus::Ok) return;
					}

					album_art_data_ptr data = CoverResizer::istream_to_data(stream.get_ptr());

					if (data.is_valid())
					{
						auto cb = fb2k::service_new<CoverAttach>(handles, data);
						threaded_process::get()->run_modeless(cb, threaded_process_flags, hwnd, "Attaching cover...");
					}
				}
				else
				{
					popup_message::g_show("Cannot attach image due to invalid type.", group_resize);
				}
			}
		}
	}

	void get_item_name(uint32_t index, pfc::string_base& out) override
	{
		if (index == 0) out = "Resize";
		else if (index == 1) out = "Attach image and Resize";
	}

private:
	bool browse_for_image(HWND parent, const std::string& folder, fb2k::imageInfo_t& info, std::unique_ptr<Gdiplus::Image>& out)
	{
		auto image_api = fb2k::imageLoaderLite::tryGet();
		if (image_api.is_empty())
		{
			popup_message::g_show(image_loader_error, group_resize);
			return false;
		}

		pfc::string8 path;
		if (uGetOpenFileName(parent, "Picture files|*.jpg;*.jpeg;*.png;*.bmp;*.gif;*.tiff;*.webp", 0, nullptr, "Browse for image", folder.starts_with("file://") ? folder.c_str() : nullptr, path, FALSE) == FALSE) return false;

		std::wstring wpath = string_wide_from_utf8_fast(path).get_ptr();
		pfc::com_ptr_t<IStream> stream;
		STATSTG sts;

		if (FAILED(SHCreateStreamOnFileEx(wpath.data(), STGM_READ | STGM_SHARE_DENY_WRITE, GENERIC_READ, FALSE, nullptr, stream.receive_ptr()))) return false;
		if (FAILED(stream->Stat(&sts, STATFLAG_DEFAULT))) return false;

		const DWORD bytes = sts.cbSize.LowPart;
		std::vector<uint8_t> buffer(bytes);
		ULONG bytes_read = 0;

		if (FAILED(stream->Read(buffer.data(), bytes, &bytes_read)) || bytes != bytes_read) return false;
		album_art_data_ptr data = album_art_data_impl::g_create(buffer.data(), buffer.size());
		if (data.is_empty()) return false;

		try
		{
			std::unique_ptr<Gdiplus::Image> image(image_api->load(data, &info));
			out = std::move(image);
			return true;
		}
		catch (const std::exception& e)
		{
			popup_message::g_show(e.what(), group_resize);
		}
		return false;
	}

	bool popup(HWND parent)
	{
		modal_dialog_scope scope;
		if (scope.can_create())
		{
			scope.initialize(parent);
			PopupDialog dlg;
			return dlg.DoModal(parent) == IDOK;
		}
		return false;
	}
};

static contextmenu_group_popup_factory g_context_group_resize(guid_context_group_resize, contextmenu_groups::root, group_resize, 0);
FB2K_SERVICE_FACTORY(ContextMenuResize);
