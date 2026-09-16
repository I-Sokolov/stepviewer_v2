#include "stdafx.h"

#include "BCFProjectSettingsDlg.h"
#include "Resource.h"

namespace
{
	class CBCFExtensionValueDlg : public CDialogEx
	{
	public:
		explicit CBCFExtensionValueDlg(CWnd* parent)
			: CDialogEx(IDD_BCF_SETTINGS_VALUE, parent)
		{
		}

		CString GetValue() const { return m_value; }

	protected:
		virtual void DoDataExchange(CDataExchange* dataExchange) override
		{
			CDialogEx::DoDataExchange(dataExchange);
			DDX_Text(dataExchange, IDC_BCF_SETTINGS_VALUE, m_value);
		}

		virtual void OnOK() override
		{
			UpdateData(TRUE);
			m_value.Trim();
			if (m_value.IsEmpty()) {
				AfxMessageBox(L"Value is empty.", MB_ICONERROR);
				return;
			}
			CDialogEx::OnOK();
		}

	private:
		CString m_value;
	};

	struct EnumerationPage
	{
		BCFEnumeration enumeration;
		LPCTSTR title;
		LPCTSTR profileKey;
	};

	const LPCTSTR ENUMERATION_PROFILE_SECTION = L"BCF Project Enumerations";
	const EnumerationPage ENUMERATION_PAGES[] = {
		{ BCFTopicTypes, L"Topic Types", L"TopicTypes" },
		{ BCFTopicStatuses, L"Topic Statuses", L"TopicStatuses" },
		{ BCFPriorities, L"Priorities", L"Priorities" },
		{ BCFTopicLabels, L"Topic Labels", L"TopicLabels" },
		{ BCFUsers, L"Users", L"Users" },
		{ BCFSnippetTypes, L"Snippet Types", L"SnippetTypes" },
		{ BCFStages, L"Stages", L"Stages" }
	};

	CString GetProfileCountKey(const EnumerationPage& definition)
	{
		CString key;
		key.Format(L"%s.Count", definition.profileKey);
		return key;
	}

	CString GetProfileValueKey(const EnumerationPage& definition, int index)
	{
		CString key;
		key.Format(L"%s.%d", definition.profileKey, index);
		return key;
	}
}

CBCFExtensionUserPage::CBCFExtensionUserPage(const CString& user)
	: CPropertyPage(IDD_BCF_SETTINGS_USER)
	, m_user(user)
{
}

void CBCFExtensionUserPage::DoDataExchange(CDataExchange* dataExchange)
{
	CPropertyPage::DoDataExchange(dataExchange);
	DDX_Text(dataExchange, IDC_BCF_SETTINGS_USER, m_user);
	m_user.Trim();
}

BEGIN_MESSAGE_MAP(CBCFExtensionEnumerationPage, CPropertyPage)
	ON_BN_CLICKED(IDC_BCF_SETTINGS_ADD, &CBCFExtensionEnumerationPage::OnAdd)
	ON_BN_CLICKED(IDC_BCF_SETTINGS_REMOVE, &CBCFExtensionEnumerationPage::OnRemove)
	ON_BN_CLICKED(IDC_BCF_SETTINGS_UP, &CBCFExtensionEnumerationPage::OnMoveUp)
	ON_BN_CLICKED(IDC_BCF_SETTINGS_DOWN, &CBCFExtensionEnumerationPage::OnMoveDown)
	ON_LBN_SELCHANGE(IDC_BCF_SETTINGS_LIST, &CBCFExtensionEnumerationPage::OnSelectionChanged)
END_MESSAGE_MAP()

CBCFExtensionEnumerationPage::CBCFExtensionEnumerationPage(
	BCFExtensions& extensions, BCFEnumeration enumeration, LPCTSTR title)
	: CPropertyPage(IDD_BCF_SETTINGS_ENUMERATION)
	, m_extensions(extensions)
	, m_enumeration(enumeration)
	, m_title(title)
{
	m_psp.dwFlags |= PSP_USETITLE;
	m_psp.pszTitle = m_title;
}

BOOL CBCFExtensionEnumerationPage::OnInitDialog()
{
	CPropertyPage::OnInitDialog();
	m_values.SubclassDlgItem(IDC_BCF_SETTINGS_LIST, this);
	m_remove.SubclassDlgItem(IDC_BCF_SETTINGS_REMOVE, this);
	m_moveUp.SubclassDlgItem(IDC_BCF_SETTINGS_UP, this);
	m_moveDown.SubclassDlgItem(IDC_BCF_SETTINGS_DOWN, this);
	for (uint16_t i = 0; const char* value = m_extensions.GetElement(m_enumeration, i); ++i) {
		CString text = FromUTF8(value);
		m_originalValues.push_back(text);
		m_values.AddString(text);
	}
	UpdateButtons();
	return TRUE;
}

void CBCFExtensionEnumerationPage::OnAdd()
{
	CBCFExtensionValueDlg dialog(this);
	if (dialog.DoModal() == IDOK) {
		const CString value = dialog.GetValue();
		if (m_values.FindStringExact(-1, value) == LB_ERR) {
			const int item = m_values.AddString(value);
			m_values.SetCurSel(item);
			SetModified();
		}
		UpdateButtons();
	}
}

void CBCFExtensionEnumerationPage::OnRemove()
{
	const int selection = m_values.GetCurSel();
	if (selection != LB_ERR) {
		m_values.DeleteString(selection);
		m_values.SetCurSel(min(selection, m_values.GetCount() - 1));
		SetModified();
		UpdateButtons();
	}
}

void CBCFExtensionEnumerationPage::MoveSelection(int offset)
{
	const int selection = m_values.GetCurSel();
	const int target = selection + offset;
	if (selection == LB_ERR || target < 0 || target >= m_values.GetCount()) {
		return;
	}
	CString value;
	m_values.GetText(selection, value);
	m_values.DeleteString(selection);
	m_values.InsertString(target, value);
	m_values.SetCurSel(target);
	SetModified();
	UpdateButtons();
}

void CBCFExtensionEnumerationPage::OnMoveUp() { MoveSelection(-1); }
void CBCFExtensionEnumerationPage::OnMoveDown() { MoveSelection(1); }
void CBCFExtensionEnumerationPage::OnSelectionChanged() { UpdateButtons(); }

void CBCFExtensionEnumerationPage::UpdateButtons()
{
	const int selection = m_values.GetCurSel();
	m_remove.EnableWindow(selection != LB_ERR);
	m_moveUp.EnableWindow(selection > 0);
	m_moveDown.EnableWindow(selection != LB_ERR && selection + 1 < m_values.GetCount());
}

bool CBCFExtensionEnumerationPage::IsEmpty()
{
	return m_values.GetSafeHwnd()
		? m_values.GetCount() == 0
		: m_extensions.GetElement(m_enumeration, 0) == nullptr;
}

bool CBCFExtensionEnumerationPage::IsRequired() const
{
	return m_enumeration == BCFTopicTypes ||
		m_enumeration == BCFTopicStatuses;
}

bool CBCFExtensionEnumerationPage::Apply()
{
	if (!m_values.GetSafeHwnd()) {
		return true;
	}
	bool ok = true;
	for (const CString& value : m_originalValues) {
		ok = m_extensions.RemoveElement(m_enumeration, ToUTF8(value).c_str()) && ok;
	}
	for (int i = 0; i < m_values.GetCount(); ++i) {
		CString value;
		m_values.GetText(i, value);
		ok = m_extensions.AddElement(m_enumeration, ToUTF8(value).c_str()) && ok;
	}
	return ok;
}

CBCFProjectSettingsDlg::CBCFProjectSettingsDlg(
	BCFProject& project, const CString& user, CWnd* parent)
	: CPropertySheet(L"Project settings", parent)
	, m_userPage(user)
{
	AddPage(&m_userPage);
	BCFExtensions& extensions = project.GetExtensions();
	for (const EnumerationPage& definition : ENUMERATION_PAGES) {
		m_extensionPages.emplace_back(new CBCFExtensionEnumerationPage(
			extensions, definition.enumeration, definition.title));
		AddPage(m_extensionPages.back().get());
	}
}

bool CBCFProjectSettingsDlg::LoadEnumerationProfile(BCFProject& project)
{
	CWinApp* app = AfxGetApp();
	BCFExtensions& extensions = project.GetExtensions();
	for (const EnumerationPage& definition : ENUMERATION_PAGES) {
		const int count = app->GetProfileInt(
			ENUMERATION_PROFILE_SECTION, GetProfileCountKey(definition), -1);
		if (count < 0) {
			continue;
		}

		std::vector<CString> currentValues;
		for (uint16_t i = 0; const char* value = extensions.GetElement(definition.enumeration, i); ++i) {
			currentValues.push_back(FromUTF8(value));
		}
		for (const CString& value : currentValues) {
			if (!extensions.RemoveElement(definition.enumeration, ToUTF8(value).c_str())) {
				return false;
			}
		}
		for (int i = 0; i < count; ++i) {
			const CString value = app->GetProfileString(
				ENUMERATION_PROFILE_SECTION, GetProfileValueKey(definition, i));
			if (value.IsEmpty() ||
				!extensions.AddElement(definition.enumeration, ToUTF8(value).c_str())) {
				return false;
			}
		}
	}
	return true;
}

bool CBCFProjectSettingsDlg::SaveEnumerationProfile(BCFProject& project)
{
	CWinApp* app = AfxGetApp();
	BCFExtensions& extensions = project.GetExtensions();
	for (const EnumerationPage& definition : ENUMERATION_PAGES) {
		const CString countKey = GetProfileCountKey(definition);
		const int oldCount = app->GetProfileInt(
			ENUMERATION_PROFILE_SECTION, countKey, 0);
		uint16_t count = 0;
		for (; const char* value = extensions.GetElement(definition.enumeration, count); ++count) {
			if (!app->WriteProfileString(ENUMERATION_PROFILE_SECTION,
				GetProfileValueKey(definition, count), FromUTF8(value))) {
				return false;
			}
		}
		for (int i = count; i < oldCount; ++i) {
			if (!app->WriteProfileString(ENUMERATION_PROFILE_SECTION,
				GetProfileValueKey(definition, i), nullptr)) {
				return false;
			}
		}
		if (!app->WriteProfileInt(ENUMERATION_PROFILE_SECTION, countKey, count)) {
			return false;
		}
	}
	return true;
}

BEGIN_MESSAGE_MAP(CBCFProjectSettingsDlg, CPropertySheet)
	ON_COMMAND(IDOK, &CBCFProjectSettingsDlg::OnApplySettings)
END_MESSAGE_MAP()

void CBCFProjectSettingsDlg::OnApplySettings()
{
	if (!m_userPage.UpdateData(TRUE)) {
		return;
	}
	if (m_userPage.GetUser().IsEmpty()) {
		AfxMessageBox(L"User is required.", MB_OK | MB_ICONERROR);
		return;
	}
	CString emptyRequiredEnumerations;
	CString emptyEnumerations;
	for (const auto& page : m_extensionPages) {
		if (page->IsEmpty()) {
			CString& list = page->IsRequired()
				? emptyRequiredEnumerations
				: emptyEnumerations;
			list.AppendFormat(L"\n- %s", page->GetTitle().GetString());
		}
	}
	if (!emptyRequiredEnumerations.IsEmpty()) {
		CString message(L"The following required enumerations are empty:\n");
		message += emptyRequiredEnumerations;
		message += L"\n\nAdd at least one value to each enumeration.";
		AfxMessageBox(message, MB_OK | MB_ICONERROR);
		return;
	}
	if (!emptyEnumerations.IsEmpty()) {
		CString message(L"The following enumerations are empty:\n");
		message += emptyEnumerations;
		message += L"\n\nDo you want to continue?";
		if (AfxMessageBox(message, MB_YESNO | MB_ICONWARNING) != IDYES) {
			return;
		}
	}
	bool ok = true;
	for (const auto& page : m_extensionPages) {
		ok = page->Apply() && ok;
	}
	if (!ok) {
		AfxMessageBox(L"Failed to update project extensions.", MB_ICONERROR);
		return;
	}
	EndDialog(IDOK);
}
