#include "stdafx.h"

#include "BCFViewControls.h"
#include "Resource.h"

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
		FormatBCFDateTime(comment.GetDate()).GetString());
	return value;
}

CString FormatBCFCommentModified(BCFComment& comment)
{
	CString value;
	if (*comment.GetModifiedAuthor() || *comment.GetModifiedDate()) {
		value.Format(L"Modified by %s %s", FromUTF8(comment.GetModifiedAuthor()).GetString(),
			FormatBCFDateTime(comment.GetModifiedDate()).GetString());
	}
	return value;
}

CString FormatBCFDateTime(const char* value)
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
	if (!GetDateFormatEx(LOCALE_NAME_USER_DEFAULT, DATE_SHORTDATE,
			&time, nullptr, date, _countof(date), nullptr) ||
		!GetTimeFormatEx(LOCALE_NAME_USER_DEFAULT, TIME_NOSECONDS,
			&time, nullptr, clock, _countof(clock))) {
		return FromUTF8(value);
	}

	CString result(date);
	result += L" ";
	result += clock;
	return result;
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

BEGIN_MESSAGE_MAP(CBCFCommentsListBox, CListBox)
	ON_WM_SIZE()
END_MESSAGE_MAP()

int CBCFCommentsListBox::AddComment(BCFComment& comment)
{
	int item = AddString(FromUTF8(comment.GetText()));
	if (item != LB_ERR && item != LB_ERRSPACE) {
		SetItemDataPtr(item, &comment);
		SetItemHeight(item, MeasureCommentHeight(&comment));
	}
	return item;
}

int CBCFCommentsListBox::AddAction(LPCTSTR text)
{
	const int item = AddString(text);
	if (item != LB_ERR && item != LB_ERRSPACE) {
		SetItemDataPtr(item, nullptr);
		SetItemHeight(item, MeasureActionHeight());
	}
	return item;
}

int CBCFCommentsListBox::MeasureActionHeight() const
{
	CClientDC dc(const_cast<CBCFCommentsListBox*>(this));
	CFont* oldFont = dc.SelectObject(GetFont());
	TEXTMETRIC metrics = {};
	dc.GetTextMetrics(&metrics);
	dc.SelectObject(oldFont);
	const int scale = dc.GetDeviceCaps(LOGPIXELSY);
	return metrics.tmHeight + 2 * MulDiv(12, scale, 96);
}

int CBCFCommentsListBox::MeasureCommentHeight(BCFComment* comment) const
{
	CClientDC dc(const_cast<CBCFCommentsListBox*>(this));
	CFont* oldFont = dc.SelectObject(GetFont());

	CRect client;
	GetClientRect(client);
	const int scale = dc.GetDeviceCaps(LOGPIXELSY);
	const int outerMargin = MulDiv(4, scale, 96);
	const int padding = MulDiv(8, scale, 96);
	const int spacing = MulDiv(6, scale, 96);
	const int width = max(MulDiv(80, scale, 96),
		client.Width() - GetSystemMetrics(SM_CXVSCROLL) - 2 * (outerMargin + padding));

	CString text = comment ? FromUTF8(comment->GetText()) : CString();
	if (text.IsEmpty()) {
		text = L"(No text)";
	}
	CRect textRect(0, 0, width, 0);
	dc.DrawText(text, textRect, DT_CALCRECT | DT_WORDBREAK | DT_EDITCONTROL | DT_NOPREFIX);

	CString metadata;
	if (comment) {
		metadata = FormatBCFCommentCreated(*comment);
		CString modified = FormatBCFCommentModified(*comment);
		if (!modified.IsEmpty()) {
			metadata += L"\n";
			metadata += modified;
		}
	}
	else {
		metadata = L" ";
	}
	CRect metadataRect(0, 0, width, 0);
	dc.DrawText(metadata, metadataRect, DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX);

	dc.SelectObject(oldFont);
	return 2 * (outerMargin + padding) + spacing + textRect.Height() + metadataRect.Height();
}

void CBCFCommentsListBox::MeasureItem(LPMEASUREITEMSTRUCT measureItem)
{
	BCFComment* comment = nullptr;
	if (measureItem->itemID != static_cast<UINT>(-1)) {
		void* data = GetItemDataPtr(measureItem->itemID);
		if (data != reinterpret_cast<void*>(LB_ERR)) {
			comment = static_cast<BCFComment*>(data);
		}
	}
	measureItem->itemHeight = comment
		? MeasureCommentHeight(comment)
		: MeasureActionHeight();
}

void CBCFCommentsListBox::DrawItem(LPDRAWITEMSTRUCT drawItem)
{
	if (drawItem->itemID == static_cast<UINT>(-1)) {
		return;
	}

	CDC dc;
	dc.Attach(drawItem->hDC);
	const int savedState = dc.SaveDC();

	const bool selected = (drawItem->itemState & ODS_SELECTED) != 0;
	const COLORREF background = GetSysColor(selected ? COLOR_HIGHLIGHT : COLOR_WINDOW);
	const COLORREF textColor = GetSysColor(selected ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT);
	const COLORREF metadataColor = selected ? textColor : GetSysColor(COLOR_GRAYTEXT);

	CRect itemRect(drawItem->rcItem);
	dc.FillSolidRect(itemRect, GetSysColor(COLOR_BTNFACE));

	const int scale = dc.GetDeviceCaps(LOGPIXELSY);
	const int outerMargin = MulDiv(4, scale, 96);
	const int padding = MulDiv(8, scale, 96);
	const int spacing = MulDiv(6, scale, 96);
	CRect cardRect(itemRect);
	cardRect.DeflateRect(outerMargin, outerMargin);
	dc.FillSolidRect(cardRect, background);
	dc.Draw3dRect(cardRect, GetSysColor(COLOR_3DSHADOW), GetSysColor(COLOR_3DHILIGHT));

	void* data = GetItemDataPtr(drawItem->itemID);
	BCFComment* comment = data == reinterpret_cast<void*>(LB_ERR)
		? nullptr
		: static_cast<BCFComment*>(data);
	if (comment) {
		CFont* oldFont = dc.SelectObject(GetFont());
		dc.SetBkMode(TRANSPARENT);

		CRect contentRect(cardRect);
		contentRect.DeflateRect(padding, padding);
		CString metadata = FormatBCFCommentCreated(*comment);
		CString modified = FormatBCFCommentModified(*comment);
		if (!modified.IsEmpty()) {
			metadata += L"\n";
			metadata += modified;
		}
		CRect metadataRect(contentRect);
		metadataRect.top = metadataRect.bottom;
		dc.DrawText(metadata, metadataRect, DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX);
		metadataRect.OffsetRect(0, -metadataRect.Height());

		CRect textRect(contentRect);
		textRect.bottom = metadataRect.top - spacing;
		CString text = FromUTF8(comment->GetText());
		if (text.IsEmpty()) {
			text = L"(No text)";
		}
		dc.SetTextColor(textColor);
		dc.DrawText(text, textRect, DT_WORDBREAK | DT_EDITCONTROL | DT_NOPREFIX);

		dc.SetTextColor(metadataColor);
		dc.DrawText(metadata, metadataRect, DT_WORDBREAK | DT_NOPREFIX);
		dc.SelectObject(oldFont);
	}
	else {
		CString text;
		GetText(drawItem->itemID, text);
		CFont* oldFont = dc.SelectObject(GetFont());
		dc.SetBkMode(TRANSPARENT);
		dc.SetTextColor(textColor);
		dc.DrawText(text, cardRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
		dc.SelectObject(oldFont);
	}

	if ((drawItem->itemState & ODS_FOCUS) != 0) {
		cardRect.DeflateRect(1, 1);
		dc.DrawFocusRect(cardRect);
	}

	dc.RestoreDC(savedState);
	dc.Detach();
}

void CBCFCommentsListBox::UpdateItemHeights()
{
	for (int item = 0; item < GetCount(); ++item) {
		void* data = GetItemDataPtr(item);
		if (data != reinterpret_cast<void*>(LB_ERR)) {
			BCFComment* comment = static_cast<BCFComment*>(data);
			SetItemHeight(item, comment
				? MeasureCommentHeight(comment)
				: MeasureActionHeight());
		}
	}
	Invalidate();
}

void CBCFCommentsListBox::OnSize(UINT type, int cx, int cy)
{
	CListBox::OnSize(type, cx, cy);
	if (GetSafeHwnd()) {
		UpdateItemHeights();
	}
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

BEGIN_MESSAGE_MAP(CBCFIncudeLoadedModels, CDialogEx)
END_MESSAGE_MAP()

CBCFIncudeLoadedModels::CBCFIncudeLoadedModels(
	const std::vector<CString>& files, CWnd* parent)
	: CDialogEx(IDD_BCF_INCLUDE_LOADED_MODELS, parent)
	, m_files(files)
	, m_mode(1)
{
}

void CBCFIncudeLoadedModels::DoDataExchange(CDataExchange* dataExchange)
{
	CDialogEx::DoDataExchange(dataExchange);
	DDX_Control(dataExchange, IDC_BCF_LOADED_MODELS, m_fileList);
	DDX_Radio(dataExchange, IDC_BCF_LOADED_MODELS_EXTERNAL, m_mode);
}

BOOL CBCFIncudeLoadedModels::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	CClientDC dc(&m_fileList);
	CFont* oldFont = m_fileList.GetFont()
		? dc.SelectObject(m_fileList.GetFont())
		: nullptr;
	int horizontalExtent = 0;
	for (const CString& file : m_files) {
		m_fileList.AddString(file);
		horizontalExtent = max(horizontalExtent,
			static_cast<int>(dc.GetTextExtent(file).cx));
	}
	if (oldFont) {
		dc.SelectObject(oldFont);
	}
	m_fileList.SetHorizontalExtent(horizontalExtent + 4);
	UpdateData(FALSE);
	return TRUE;
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
