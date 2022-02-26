#include "stdafx.hpp"

namespace cinfo
{
	static constexpr std::array field_names =
	{
		"front_cover_width",
		"front_cover_height",
		"front_cover_size",
		"front_cover_format",
		"front_cover_bytes",
	};

	class MetadbIndexClient : public metadb_index_client
	{
	public:
		metadb_index_hash transform(const file_info& info, const playable_location& location) override
		{
			return generate_hash(location.get_path());
		}
	};

	static auto g_client = new service_impl_single_t<MetadbIndexClient>;
	static metadb_index_manager::ptr g_cachedAPI;

	class InitStageCallback : public init_stage_callback
	{
	public:
		void on_init_stage(uint32_t stage) override
		{
			if (stage == init_stages::before_config_read)
			{
				g_cachedAPI = metadb_index_manager::get();
				try
				{
					g_cachedAPI->add(g_client, guid_metadb_index, system_time_periods::week * 4);
					g_cachedAPI->dispatch_global_refresh();
				}
				catch (const std::exception& e)
				{
					g_cachedAPI->remove(guid_metadb_index);
					FB2K_console_formatter() << component_name << " stats: Critical initialisation failure: " << e;
				}
			}
		}
	};

	class InitQuit : public initquit
	{
	public:
		void on_quit() override
		{
			g_cachedAPI.release();
		}
	};

	class MetadbDisplayFieldProvider : public metadb_display_field_provider
	{
	public:
		bool process_field(uint32_t index, metadb_handle* handle, titleformat_text_out* out) override
		{
			metadb_index_hash hash;
			if (!hashHandle(handle, hash)) return false;

			const Fields f = get(hash);

			switch (index)
			{
			case 0:
				if (f.front_cover_width == 0) return false;
				out->write_int(titleformat_inputtypes::meta, f.front_cover_width);
				return true;
			case 1:
				if (f.front_cover_height == 0) return false;
				out->write_int(titleformat_inputtypes::meta, f.front_cover_height);
				return true;
			case 2:
				if (f.front_cover_bytes == 0) return false;
				out->write(titleformat_inputtypes::meta, pfc::format_file_size_short(f.front_cover_bytes));
				return true;
			case 3:
				if (f.front_cover_format.is_empty()) return false;
				out->write(titleformat_inputtypes::meta, f.front_cover_format);
				return true;
			case 4:
				if (f.front_cover_bytes == 0) return false;
				out->write_int(titleformat_inputtypes::meta, f.front_cover_bytes);
				return true;
			}
			return false;
		}

		uint32_t get_field_count() override
		{
			return field_names.size();
		}

		void get_field_name(uint32_t index, pfc::string_base& out) override
		{
			out = field_names[index];
		}
	};

	class FileOperationCallback : public file_operation_callback
	{
	public:
		using PathList = const pfc::list_base_const_t<const char*>&;

		void on_files_copied_sorted(PathList from, PathList to) override
		{
			update(from, to, false);
		}

		void on_files_deleted_sorted(PathList) override {}

		void on_files_moved_sorted(PathList from, PathList to) override
		{
			update(from, to, true);
		}

	private:
		void update(PathList from, PathList to, bool clear_old)
		{
			HashList to_refresh;
			const size_t count = from.get_count();

			for (const size_t i : std::views::iota(0U, count))
			{
				metadb_index_hash old_hash = generate_hash(from[i]);
				metadb_index_hash new_hash = generate_hash(to[i]);
				set(new_hash, get(old_hash));
				to_refresh += new_hash;

				if (clear_old)
				{
					set(old_hash, Fields());
					to_refresh += old_hash;
				}
			}

			refresh(to_refresh);
		}
	};

	FB2K_SERVICE_FACTORY(FileOperationCallback);
	FB2K_SERVICE_FACTORY(InitStageCallback);
	FB2K_SERVICE_FACTORY(InitQuit);
	FB2K_SERVICE_FACTORY(MetadbDisplayFieldProvider);

	Fields get(metadb_index_hash hash)
	{
		mem_block_container_impl temp;
		theAPI()->get_user_data(guid_metadb_index, hash, temp);
		if (temp.get_size() > 0)
		{
			try
			{
				stream_reader_formatter_simple_ref reader(temp.get_ptr(), temp.get_size());
				Fields f;
				reader >> f.front_cover_width;
				reader >> f.front_cover_height;
				reader >> f.front_cover_bytes;
				reader >> f.front_cover_format;
				return f;
			}
			catch (exception_io_data) {}
		}
		return Fields();
	}

	bool hashHandle(const metadb_handle_ptr& handle, metadb_index_hash& hash)
	{
		return g_client->hashHandle(handle, hash);
	}

	metadb_index_hash generate_hash(file_path_display path)
	{
		return hasher_md5::get()->process_single_string(path).xorHalve();
	}

	metadb_index_manager::ptr theAPI()
	{
		if (g_cachedAPI.is_valid()) return g_cachedAPI;
		return metadb_index_manager::get();
	}

	void refresh(const HashList& hashes)
	{
		fb2k::inMainThread([hashes]
			{
				theAPI()->dispatch_refresh(guid_metadb_index, hashes);
			});
	}

	void reset(metadb_handle_list_cref handles)
	{
		HashList to_refresh;
		HashSet hashes;

		const size_t count = handles.get_count();
		for (const size_t i : std::views::iota(0U, count))
		{
			metadb_index_hash hash;
			if (hashHandle(handles[i], hash) && hashes.emplace(hash).second)
			{
				set(hash, Fields());
				to_refresh += hash;
			}
		}
		refresh(to_refresh);
	}

	void set(metadb_index_hash hash, Fields f)
	{
		stream_writer_formatter_simple writer;
		writer << f.front_cover_width;
		writer << f.front_cover_height;
		writer << f.front_cover_bytes;
		writer << f.front_cover_format;
		theAPI()->set_user_data(guid_metadb_index, hash, writer.m_buffer.get_ptr(), writer.m_buffer.get_size());
	}
}
