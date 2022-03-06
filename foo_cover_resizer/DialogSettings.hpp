#pragma once

namespace resizer
{
	static constexpr std::array formats =
	{
		L"JPG",
		L"PNG"
	};

	class CDialogSettings : public CDialogImpl<CDialogSettings>
	{
	public:
		BEGIN_MSG_MAP_EX(CDialogSettings)
			MSG_WM_INITDIALOG(OnInitDialog)
			COMMAND_RANGE_HANDLER_EX(IDOK, IDCANCEL, OnCloseCmd)
			COMMAND_CODE_HANDLER_EX(EN_UPDATE, OnUpdate)
		END_MSG_MAP()

		enum { IDD = IDD_DIALOG_SETTINGS };

		BOOL OnInitDialog(CWindow, LPARAM)
		{
			m_button_ok = GetDlgItem(IDOK);
			m_combo_type = GetDlgItem(IDC_COMBO_TYPE);
			m_combo_format = GetDlgItem(IDC_COMBO_FORMAT);
			m_edit_size = GetDlgItem(IDC_EDIT_SIZE);

			for (const size_t i : std::views::iota(0U, album_art_ids::num_types()))
			{
				m_combo_type.AddString(string_wide_from_utf8_fast(album_art_ids::query_capitalized_name(i)));
			}

			for (const auto& format : formats)
			{
				m_combo_format.AddString(format);
			}

			m_combo_type.SetCurSel(settings::type.get_value());
			m_combo_format.SetCurSel(settings::format.get_value());
			pfc::setWindowText(m_edit_size, std::to_string(settings::size.get_value()).c_str());

			CenterWindow();
			return TRUE;
		}

		int GetSize()
		{
			pfc::string8 str = pfc::getWindowText(m_edit_size);
			if (pfc::string_is_numeric(str)) return std::stoi(str.get_ptr());
			return 0;
		}

		void OnCloseCmd(UINT, int nID, CWindow)
		{
			if (nID == IDOK)
			{
				settings::type = m_combo_type.GetCurSel();
				settings::format = m_combo_format.GetCurSel();
				settings::size = GetSize();
			}

			EndDialog(nID);
		}

		void OnUpdate(UINT, int, CWindow)
		{
			m_button_ok.EnableWindow(GetSize() >= 200);
		}

		CButton m_button_ok;
		CComboBox m_combo_format, m_combo_type;
		CEdit m_edit_size;
	};
}
