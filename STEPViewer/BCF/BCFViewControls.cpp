#include "stdafx.h"

#include "BCFViewControls.h"

namespace
{
	CString FormatDateTime(const char* value)
	{
		if (!value || !*value) {
			return CString();
		}

		SYSTEMTIME time = {};
		if (sscanf_s(value, "%4hu-%2hu-%2huT%2hu:%2hu:%2hu",
			&time.wYear, &time.wMonth, &time.wDay,
			&time.wHour, &time.wMinute, &time.wSecond) != 6) {
			return FromUTF8(value);
		}

		FILETIME fileTime;
		if (!SystemTimeToFileTime(&time, &fileTime)) {
			return FromUTF8(value);
		}

		wchar_t date[64] = {};
		wchar_t clock[64] = {};
		if (!GetDateFormatEx(LOCALE_NAME_USER_DEFAULT, DATE_SHORTDATE, &time, NULL, date, _countof(date), NULL)
			|| !GetTimeFormatEx(LOCALE_NAME_USER_DEFAULT, TIME_NOSECONDS, &time, NULL, clock, _countof(clock))) {
			return FromUTF8(value);
		}

		CString result(date);
		result += L" ";
		result += clock;
		return result;
	}
}

LPCTSTR RegisterBCFPaneClass()
{
	static CString className = AfxRegisterWndClass(CS_DBLCLKS, ::LoadCursor(nullptr, IDC_ARROW),
		reinterpret_cast<HBRUSH>(COLOR_3DFACE + 1), nullptr);
	return className;
}

void SetBCFControlFont(CWnd& control, CWnd* owner)
{
	CFont* font = nullptr;
	for (CWnd* window = owner; window && !font; window = window->GetParent()) {
		font = window->GetFont();
	}
	if (!font) {
		font = &afxGlobalData.fontRegular;
	}
	control.SetFont(font);
}

void SetBCFComboValue(CComboBox& combo, const CString& value)
{
	if (value.IsEmpty()) {
		combo.SetCurSel(-1);
		return;
	}
	int item = combo.FindStringExact(-1, value);
	if (item == CB_ERR) {
		item = combo.AddString(value);
	}
	combo.SetCurSel(item);
}

BOOL CreateBCFStaticLabel(CStatic& label, LPCTSTR text, CWnd* parent)
{
	BOOL result = label.Create(text, WS_CHILD | WS_VISIBLE | SS_LEFT, CRect(0, 0, 0, 0), parent);
	SetBCFControlFont(label, parent);
	return result;
}

CString FormatBCFCommentCreated(BCFComment& comment)
{
	CString value;
	value.Format(L"Created by %s %s", FromUTF8(comment.GetAuthor()).GetString(),
		FormatDateTime(comment.GetDate()).GetString());
	return value;
}

CString FormatBCFCommentModified(BCFComment& comment)
{
	CString value;
	if (*comment.GetModifiedAuthor() || *comment.GetModifiedDate()) {
		value.Format(L"Modified by %s %s", FromUTF8(comment.GetModifiedAuthor()).GetString(),
			FormatDateTime(comment.GetModifiedDate()).GetString());
	}
	return value;
}

CString GetBCFTopicDisplayName(BCFTopic& topic)
{
	uint16_t index = 0;
	BCFProject& project = topic.GetProject();
	while (BCFTopic* candidate = project.GetTopic(index++)) {
		if (candidate == &topic) {
			break;
		}
	}

	CString text;
	text.Format(L"#%d: %s - %s", index, FromUTF8(topic.GetGuid()).GetString(),
		FromUTF8(topic.GetTitle()).GetString());
	return text;
}

CBCFSelectFileDlg::CBCFSelectFileDlg(LPCTSTR filePath, bool external, CWnd* parent,
	LPCTSTR filter, DWORD flags)
	: CFileDialog(TRUE, nullptr, filePath, flags, filter, parent)
	, m_external(external)
{
	AddRadioButtonList(ModeControl);
	AddControlItem(ModeControl, ExternalFileMode, L"Use external file");
	AddControlItem(ModeControl, EmbedFileMode, L"Embed to BCF package");
	SetSelectedControlItem(ModeControl, external ? ExternalFileMode : EmbedFileMode);
}

BOOL CBCFSelectFileDlg::OnFileNameOK()
{
	DWORD mode = 0;
	if (FAILED(GetSelectedControlItem(ModeControl, mode))) {
		AfxMessageBox(L"Cannot determine how the selected file should be stored.", MB_ICONERROR);
		return TRUE;
	}
	m_external = mode == ExternalFileMode;
	return CFileDialog::OnFileNameOK();
}

BEGIN_MESSAGE_MAP(CBCFEdit, CEdit)
	ON_WM_PAINT()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
END_MESSAGE_MAP()

void CBCFEdit::OnPaint()
{
	CEdit::OnPaint();
	if (::GetFocus() != GetSafeHwnd()) {
		return;
	}

	CClientDC dc(this);
	CRect client;
	GetClientRect(client);
	CPen pen(PS_SOLID, 2, ::GetSysColor(COLOR_HIGHLIGHT));
	CPen* oldPen = dc.SelectObject(&pen);
	dc.MoveTo(client.left, client.bottom - 1);
	dc.LineTo(client.right, client.bottom - 1);
	dc.SelectObject(oldPen);
}

void CBCFEdit::OnSetFocus(CWnd* oldWnd)
{
	CEdit::OnSetFocus(oldWnd);
	Invalidate(FALSE);
}

void CBCFEdit::OnKillFocus(CWnd* newWnd)
{
	CEdit::OnKillFocus(newWnd);
	Invalidate(FALSE);
}
