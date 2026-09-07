#include "stdafx.h"

#include "BCFView.h"
#include "BCFViewPointMgr.h"
#include "BCFTopicLabelsDlg.h"
#include "STEPViewerDoc.h"
#include "Resource.h"
#include "_ap_model_factory.h"
#include "_ptr.h"

#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;

class CBCFPopupMenu : public CMFCPopupMenu
{
public:
	virtual BOOL Create(CWnd* parent, int x, int y, HMENU menu, BOOL locked = FALSE,
		BOOL ownMessage = FALSE) override
	{
		const BOOL showAllCommands = CMFCMenuBar::IsShowAllCommands();
		CMFCMenuBar::SetShowAllCommands(TRUE);
		const BOOL created = CMFCPopupMenu::Create(parent, x, y, menu, locked, ownMessage);
		CMFCMenuBar::SetShowAllCommands(showAllCommands);
		return created;
	}

};

class CBCFMenuButton : public CMFCToolBarMenuButton
{
	DECLARE_SERIAL(CBCFMenuButton)

public:
	CBCFMenuButton(HMENU menu = nullptr)
		: CMFCToolBarMenuButton(static_cast<UINT>(-1), menu, -1)
	{
	}

	virtual CMFCPopupMenu* CreatePopupMenu() override
	{
		return new CBCFPopupMenu;
	}
};

IMPLEMENT_SERIAL(CBCFMenuButton, CMFCToolBarMenuButton, 1)

namespace
{
	enum ControlId
	{
		IDC_PANE_PROJECT_ID = 2100,
		IDC_PANE_PROJECT_NAME,
		IDC_PANE_TOPICS,
		IDC_PANE_TABS,
		IDC_PANE_VIEW_PROJECT,
		IDC_PANE_TOPIC_TITLE,
		IDC_PANE_TOPIC_DESCRIPTION,
		IDC_PANE_TOPIC_TYPE,
		IDC_PANE_TOPIC_STAGE,
		IDC_PANE_TOPIC_STATUS,
		IDC_PANE_TOPIC_ASSIGNED,
		IDC_PANE_TOPIC_PRIORITY,
		IDC_PANE_TOPIC_DUE,
		IDC_PANE_TOPIC_SNIPPET,
		IDC_PANE_TOPIC_REFERENCE,
		IDC_PANE_TOPIC_SCHEMA,
		IDC_PANE_TOPIC_INDEX,
		IDC_PANE_TOPIC_SERVER_ID,
		IDC_PANE_SELECT_SNIPPET_FILE,
		IDC_PANE_SELECT_TOPIC_LABELS,
		IDC_PANE_COMMENTS,
		IDC_PANE_COMMENT_TEXT
	};

	const int COLUMN_COUNT = 10;
	const wchar_t* COLUMN_NAMES[COLUMN_COUNT] = {
		L"GUID", L"Title", L"Type", L"Stage", L"Status",
		L"Assigned", L"Priority", L"Due", L"Created", L"Modified"
	};
	const int COLUMN_WIDTHS[COLUMN_COUNT] = { 190, 180, 90, 90, 90, 130, 80, 90, 130, 130 };

	LPCTSTR RegisterPaneClass()
	{
		static CString className = AfxRegisterWndClass(CS_DBLCLKS, ::LoadCursor(nullptr, IDC_ARROW),
			reinterpret_cast<HBRUSH>(COLOR_3DFACE + 1), nullptr);
		return className;
	}

	void SetControlFont(CWnd& control, CWnd* owner)
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

	BOOL CreateStaticLabel(CStatic& label, LPCTSTR text, CWnd* parent)
	{
		BOOL result = label.Create(text, WS_CHILD | WS_VISIBLE | SS_LEFT, CRect(0, 0, 0, 0), parent);
		SetControlFont(label, parent);
		return result;
	}

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

	CString FormatCommentCreated(BCFComment& comment)
	{
		CString value;
		value.Format(L"Created by %s %s", FromUTF8(comment.GetAuthor()).GetString(),
			FormatDateTime(comment.GetDate()).GetString());
		return value;
	}

	CString FormatCommentModified(BCFComment& comment)
	{
		CString value;
		if (*comment.GetModifiedAuthor() || *comment.GetModifiedDate()) {
			value.Format(L"Modified by %s %s", FromUTF8(comment.GetModifiedAuthor()).GetString(),
				FormatDateTime(comment.GetModifiedDate()).GetString());
		}
		return value;
	}
}

CBCFSelectFileDlg::CBCFSelectFileDlg(LPCTSTR filePath, bool external, CWnd* parent)
	: CFileDialog(TRUE, nullptr, filePath, OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
		L"All files (*.*)|*.*||", parent)
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

BEGIN_MESSAGE_MAP(CBCFProjectForm, CWnd)
	ON_WM_SIZE()
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_PANE_TOPICS, &CBCFProjectForm::OnTopicChanged)
	ON_NOTIFY(NM_DBLCLK, IDC_PANE_TOPICS, &CBCFProjectForm::OnTopicDoubleClick)
END_MESSAGE_MAP()

BOOL CBCFProjectForm::Create(CBCFView* pane)
{
	m_pane = pane;

	if (!CreateEx(0, RegisterPaneClass(), L"", WS_CHILD | WS_CLIPCHILDREN, CRect(), pane, 0)) {
		return FALSE;
	}

	CreateStaticLabel(m_topicsLabel, L"Topics:", this);
	
	m_topics.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
		CRect(), this, IDC_PANE_TOPICS);
	SetControlFont(m_topics, this);
	m_topics.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
	
	for (int column = 0; column < COLUMN_COUNT; ++column) {
		m_topics.InsertColumn(column, COLUMN_NAMES[column], LVCFMT_LEFT, COLUMN_WIDTHS[column]);
	}
	
	return TRUE;
}

void CBCFProjectForm::Load(BCFTopic* selectTopic)
{
	BCFTopic* selected = selectTopic ? selectTopic : GetSelectedTopic();
	BCFProject* project = m_pane->GetProject();
	m_topics.SetRedraw(FALSE);
	m_topics.DeleteAllItems();
	int selectedItem = -1;
	if (project) {
		for (uint16_t i = 0; BCFTopic* topic = project->GetTopic(i); ++i) {
			int item = m_topics.InsertItem(m_topics.GetItemCount(), FromUTF8(topic->GetGuid()));
			m_topics.SetItemText(item, 1, FromUTF8(topic->GetTitle()));
			m_topics.SetItemText(item, 2, FromUTF8(topic->GetTopicType()));
			m_topics.SetItemText(item, 3, FromUTF8(topic->GetStage()));
			m_topics.SetItemText(item, 4, FromUTF8(topic->GetTopicStatus()));
			m_topics.SetItemText(item, 5, FromUTF8(topic->GetAssignedTo()));
			m_topics.SetItemText(item, 6, FromUTF8(topic->GetPriority()));
			m_topics.SetItemText(item, 7, FromUTF8(topic->GetDueDate()));
			CString created = CBCFProjectDlg::FormatDateTime(topic->GetCreationDate());
			if (*topic->GetCreationAuthor()) {
				created.AppendFormat(L" - %s", FromUTF8(topic->GetCreationAuthor()).GetString());
			}
			m_topics.SetItemText(item, 8, created);
			CString modified = CBCFProjectDlg::FormatDateTime(topic->GetModifiedDate());
			if (*topic->GetModifiedAuthor()) {
				modified.AppendFormat(L" - %s", FromUTF8(topic->GetModifiedAuthor()).GetString());
			}
			m_topics.SetItemText(item, 9, modified);
			m_topics.SetItemData(item, reinterpret_cast<DWORD_PTR>(topic));
			if (topic == selected) {
				selectedItem = item;
			}
		}
	}
	m_topics.SetRedraw(TRUE);
	m_topics.Invalidate();
	if (selectedItem < 0 && m_topics.GetItemCount()) {
		selectedItem = 0;
	}
	if (selectedItem >= 0) {
		m_topics.SetItemState(selectedItem, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	}
	m_pane->RefreshCommandUI();
}

bool CBCFProjectForm::Commit()
{
	return m_pane->CommitProjectInfo();
}

BCFTopic* CBCFProjectForm::GetSelectedTopic() const
{
	POSITION position = m_topics.GetFirstSelectedItemPosition();
	if (!position) {
		return nullptr;
	}
	int item = const_cast<CListCtrl&>(m_topics).GetNextSelectedItem(position);
	return reinterpret_cast<BCFTopic*>(m_topics.GetItemData(item));
}

void CBCFProjectForm::OnSize(UINT type, int cx, int cy)
{
	CWnd::OnSize(type, cx, cy);
	if (!m_topics.GetSafeHwnd()) {
		return;
	}
	const int margin = 8;
	m_topicsLabel.MoveWindow(margin, margin, 60, 18);
	m_topics.MoveWindow(margin, margin + 20, max(20, cx - 2 * margin),
		max(20, cy - 3 * margin - 12));
}

void CBCFProjectForm::OnTopicChanged(NMHDR*, LRESULT* result)
{
	m_pane->RefreshCommandUI();
	*result = 0;
}

void CBCFProjectForm::OnTopicDoubleClick(NMHDR*, LRESULT* result)
{
	if (BCFTopic* topic = GetSelectedTopic()) {
		m_pane->ShowTopic(topic);
	}
	*result = 0;
}

BEGIN_MESSAGE_MAP(CBCFTopicForm, CWnd)
	ON_WM_SIZE()
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_PANE_VIEW_PROJECT, &CBCFTopicForm::OnViewProject)
	ON_BN_CLICKED(IDC_PANE_SELECT_SNIPPET_FILE, &CBCFTopicForm::OnSelectSnippetFile)
	ON_BN_CLICKED(IDC_PANE_SELECT_TOPIC_LABELS, &CBCFTopicForm::OnSelectTopicLabels)
	ON_NOTIFY(TCN_SELCHANGE, IDC_PANE_TABS, &CBCFTopicForm::OnTabChanged)
	ON_CONTROL(LBN_SELCHANGE, IDC_PANE_COMMENTS, &CBCFTopicForm::OnCommentChanged)
	ON_CONTROL(LBN_DBLCLK, IDC_PANE_COMMENTS, &CBCFTopicForm::OnCommentDoubleClick)
END_MESSAGE_MAP()

BOOL CBCFTopicForm::Create(CBCFView* pane)
{
	m_pane = pane;

	if (!CreateEx(0, RegisterPaneClass(), L"", WS_CHILD | WS_CLIPCHILDREN, CRect(), pane, 0)) {
		return FALSE;
	}

	m_viewProject.Create(L"<<", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
		CRect(), this, IDC_PANE_VIEW_PROJECT);
	SetControlFont(m_viewProject, this);
	CreateStaticLabel(m_topicInfo, L"Topic", this);
	m_separator.Create(L"", WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ, CRect(), this);

	m_tabs.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | TCS_TABS, CRect(), this, IDC_PANE_TABS);	
	SetControlFont(m_tabs, this);
	
	m_tabs.InsertItem(0, L"Title");
	m_tabs.InsertItem(1, L"Attributes");
	m_tabs.InsertItem(2, L"Comments");
	m_tabs.InsertItem(3, L"Documents");
	m_tabs.InsertItem(4, L"Links");

	m_title.Create(WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL, CRect(), this, IDC_PANE_TOPIC_TITLE);

	CreateStaticLabel(m_descriptionLabel, L"Description:", this);
	m_description.Create(WS_CHILD | WS_TABSTOP | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN | WS_VSCROLL,
		CRect(), this, IDC_PANE_TOPIC_DESCRIPTION);

	SetControlFont(m_title, this);
	SetControlFont(m_description, this);

	const wchar_t* labels[12] = {
		L"Type:", L"Stage:", L"Status:", L"Assigned:", L"Priority:", L"Due:",
		L"Type:", L"File:", L"Schema:", L"Index:", L"Server Id:", L""
	};

	m_snippetGroup.Create(L"Snippet", WS_CHILD | BS_GROUPBOX, CRect(), this, 0);
	SetControlFont(m_snippetGroup, this);

	for (int i = 0; i < 12; ++i) {
		CreateStaticLabel(m_attributeLabels[i], labels[i], this);
	}
	CreateStaticLabel(m_labelsLabel, L"Labels:", this);
	m_labelsLabel.ModifyStyleEx(0, WS_EX_TRANSPARENT);

	DWORD comboStyle = WS_CHILD | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWN;
	m_type.Create(comboStyle, CRect(), this, IDC_PANE_TOPIC_TYPE);
	m_stage.Create(comboStyle, CRect(), this, IDC_PANE_TOPIC_STAGE);
	m_status.Create(comboStyle, CRect(), this, IDC_PANE_TOPIC_STATUS);
	m_assigned.Create(comboStyle, CRect(), this, IDC_PANE_TOPIC_ASSIGNED);
	m_priority.Create(comboStyle, CRect(), this, IDC_PANE_TOPIC_PRIORITY);
	m_snippetType.Create(comboStyle, CRect(), this, IDC_PANE_TOPIC_SNIPPET);
	CWnd* combos[] = { &m_type, &m_stage, &m_status, &m_assigned, &m_priority, &m_snippetType };
	for (CWnd* combo : combos) {
		SetControlFont(*combo, this);
	}
	m_snippetExternal.Create(L"External", WS_CHILD | BS_AUTOCHECKBOX, CRect(), this, 0);
	SetControlFont(m_snippetExternal, this);
	m_snippetExternal.EnableWindow(FALSE);
	m_selectSnippetFile.Create(L"...", WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
		CRect(), this, IDC_PANE_SELECT_SNIPPET_FILE);
	SetControlFont(m_selectSnippetFile, this);
	DWORD editStyle = WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL;
	m_due.Create(editStyle, CRect(), this, IDC_PANE_TOPIC_DUE);
	m_labels.Create(editStyle | ES_READONLY, CRect(), this, 0);
	m_selectTopicLabels.Create(L"...", WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
		CRect(), this, IDC_PANE_SELECT_TOPIC_LABELS);
	SetControlFont(m_selectTopicLabels, this);
	m_snippetReference.Create(editStyle | ES_READONLY, CRect(), this, IDC_PANE_TOPIC_REFERENCE);
	m_snippetSchema.Create(editStyle, CRect(), this, IDC_PANE_TOPIC_SCHEMA);
	m_index.Create(editStyle, CRect(), this, IDC_PANE_TOPIC_INDEX);
	m_serverId.Create(editStyle, CRect(), this, IDC_PANE_TOPIC_SERVER_ID);
	CWnd* edits[] = {
		&m_due, &m_labels, &m_snippetReference, &m_snippetSchema, &m_index, &m_serverId
	};
	for (CWnd* edit : edits) {
		SetControlFont(*edit, this);
	}
	m_comments.Create(WS_CHILD | WS_BORDER | WS_TABSTOP | WS_VSCROLL | LBS_NOTIFY | LBS_OWNERDRAWVARIABLE |
		LBS_HASSTRINGS | LBS_NOINTEGRALHEIGHT, CRect(), this, IDC_PANE_COMMENTS);
	SetControlFont(m_comments, this);
	CreateStaticLabel(m_documentsPlaceholder, L"Documents are not available in this version.", this);
	CreateStaticLabel(m_linksPlaceholder, L"Links are not available in this version.", this);

	CStatic* tabLabels[] = {
		&m_descriptionLabel, &m_documentsPlaceholder, &m_linksPlaceholder
	};
	for (CStatic* label : tabLabels) {
		label->ModifyStyleEx(0, WS_EX_TRANSPARENT);
	}
	for (CStatic& label : m_attributeLabels) {
		label.ModifyStyleEx(0, WS_EX_TRANSPARENT);
	}

	m_tabs.SetCurSel(0);
	ShowTab(0);
	return TRUE;
}

void CBCFTopicForm::LoadExtension(CComboBox& combo, BCFEnumeration enumeration)
{
	combo.ResetContent();
	if (!m_topic) {
		return;
	}
	BCFExtensions& extensions = m_topic->GetProject().GetExtensions();
	for (uint16_t i = 0; const char* value = extensions.GetElement(enumeration, i); ++i) {
		combo.AddString(FromUTF8(value));
	}
}

void CBCFTopicForm::UpdateLabels()
{
	CString labelsText;
	if (m_topic) {
		for (uint16_t i = 0; const char* label = m_topic->GetLabel(i); ++i) {
			if (i != 0) {
				labelsText += L", ";
			}
			labelsText += FromUTF8(label);
		}
	}
	m_labels.SetWindowText(labelsText);
}

void CBCFTopicForm::Load(BCFTopic* topic)
{
	m_topic = topic;
	if (!topic) {
		return;
	}
	LoadExtension(m_type, BCFTopicTypes);
	LoadExtension(m_stage, BCFStages);
	LoadExtension(m_status, BCFTopicStatuses);
	LoadExtension(m_assigned, BCFUsers);
	LoadExtension(m_priority, BCFPriorities);
	LoadExtension(m_snippetType, BCFSnippetTypes);
	m_title.SetWindowText(FromUTF8(topic->GetTitle()));
	m_description.SetWindowText(FromUTF8(topic->GetDescription()));
	m_type.SetWindowText(FromUTF8(topic->GetTopicType()));
	m_stage.SetWindowText(FromUTF8(topic->GetStage()));
	m_status.SetWindowText(FromUTF8(topic->GetTopicStatus()));
	m_assigned.SetWindowText(FromUTF8(topic->GetAssignedTo()));
	m_priority.SetWindowText(FromUTF8(topic->GetPriority()));
	m_due.SetWindowText(FromUTF8(topic->GetDueDate()));
	m_index.SetWindowText(FromUTF8(topic->GetIndexStr()));
	m_serverId.SetWindowText(FromUTF8(topic->GetServerAssignedId()));
	UpdateLabels();
	BCFBimSnippet* snippet = topic->GetBimSnippet(false);
	m_snippetType.SetWindowText(snippet ? FromUTF8(snippet->GetSnippetType()) : CString());
	m_snippetSchema.SetWindowText(snippet ? FromUTF8(snippet->GetReferenceSchema()) : CString());
	m_snippetExternal.SetCheck(snippet && snippet->GetIsExternal() ? BST_CHECKED : BST_UNCHECKED);
	CString reference = snippet ? FromUTF8(snippet->GetReference()) : CString();
	if (snippet && !snippet->GetIsExternal()) {
		fs::path path(static_cast<LPCWSTR>(reference));
		reference = path.filename().wstring().c_str();
	}
	m_snippetReference.SetWindowText(reference);
	CComboBox* combos[] = { &m_type, &m_stage, &m_status, &m_assigned, &m_priority, &m_snippetType };
	for (CComboBox* combo : combos) {
		const int textLength = combo->GetWindowTextLength();
		combo->SetEditSel(textLength, textLength);
	}
	FormatTopicInfo();
	
	ReloadComments();
	m_pane->LoadBimFiles(*topic);
	m_tabs.SetCurSel(0);
	ShowTab(0);
}

bool CBCFTopicForm::Commit()
{
	if (!m_topic) {
		return true;
	}
	CString title, description, type, stage, status, assigned, priority, due, snippetType, schema, index, serverId;
	m_title.GetWindowText(title);
	m_description.GetWindowText(description);
	m_type.GetWindowText(type);
	m_stage.GetWindowText(stage);
	m_status.GetWindowText(status);
	m_assigned.GetWindowText(assigned);
	m_priority.GetWindowText(priority);
	m_due.GetWindowText(due);
	m_snippetType.GetWindowText(snippetType);
	m_snippetSchema.GetWindowText(schema);
	m_index.GetWindowText(index);
	m_serverId.GetWindowText(serverId);
	bool ok = m_topic->SetTitle(ToUTF8(title).c_str());
	ok = m_topic->SetDescription(ToUTF8(description).c_str()) && ok;
	ok = m_topic->SetTopicType(ToUTF8(type).c_str()) && ok;
	ok = m_topic->SetStage(ToUTF8(stage).c_str()) && ok;
	ok = m_topic->SetTopicStatus(ToUTF8(status).c_str()) && ok;
	ok = m_topic->SetAssignedTo(ToUTF8(assigned).c_str()) && ok;
	ok = m_topic->SetPriority(ToUTF8(priority).c_str()) && ok;
	ok = m_topic->SetDueDate(ToUTF8(due).c_str()) && ok;
	ok = (index.IsEmpty() ? m_topic->SetIndexStr("") : m_topic->SetIndex(_wtoi(index))) && ok;
	ok = m_topic->SetServerAssignedId(ToUTF8(serverId).c_str()) && ok;
	BCFBimSnippet* existingSnippet = m_topic->GetBimSnippet(false);
	const bool hasReference = existingSnippet && *existingSnippet->GetReference();
	if (snippetType.IsEmpty() && !hasReference && schema.IsEmpty()) {
		if (existingSnippet) {
			BCFBimSnippet* snippet = existingSnippet;
			ok = snippet->Remove() && ok;
		}
	}
	else {
		if (BCFBimSnippet* snippet = m_topic->GetBimSnippet(true)) {
			ok = snippet->SetSnippetType(ToUTF8(snippetType).c_str()) && ok;
			ok = snippet->SetReferenceSchema(ToUTF8(schema).c_str()) && ok;
		}
		else {
			ok = false;
		}
	}

	FormatTopicInfo ();
	
	m_pane->ShowLog (!ok);
	return ok;
}

void CBCFTopicForm::FormatTopicInfo ()
{
	if (!m_topic) {
		return;
	}

	CString value;
	value.Format(L"Topic %s created by %s %s", FromUTF8(m_topic->GetGuid()).GetString(),
		FromUTF8(m_topic->GetCreationAuthor()).GetString(),
		CBCFProjectDlg::FormatDateTime(m_topic->GetCreationDate()).GetString());

	if (*m_topic->GetModifiedAuthor() || *m_topic->GetModifiedDate()) {
		value.AppendFormat(L", modified by %s %s", FromUTF8(m_topic->GetModifiedAuthor()).GetString(),
			CBCFProjectDlg::FormatDateTime(m_topic->GetModifiedDate()).GetString());
	}

	m_topicInfo.SetWindowText(value);
}

void CBCFTopicForm::ReloadComments(BCFComment* selectComment)
{
	m_comments.ResetContent();
	int selected = -1;
	if (m_topic) {
		for (uint16_t i = 0; BCFComment* comment = m_topic->GetComment(i); ++i) {
			int item = m_comments.AddComment(*comment);
			if (comment == selectComment) {
				selected = item;
			}
		}
	}
	if (selected < 0 && m_comments.GetCount() > 0) {
		selected = 0;
	}
	m_comments.SetCurSel(selected);
	OnCommentChanged();
}

BCFComment* CBCFTopicForm::GetSelectedComment() const
{
	int selection = m_comments.GetCurSel();
	return selection == LB_ERR
		? nullptr
		: static_cast<BCFComment*>(m_comments.GetItemDataPtr(selection));
}

void CBCFTopicForm::OnCommentChanged()
{
	BCFComment* comment = GetSelectedComment();
	if (comment && m_pane->GetDocument()) {
		CBCFViewPointMgr(*m_pane->GetDocument()).SetViewFromComment(*comment);
		m_pane->ShowLog(false);
	}
	m_pane->RefreshCommandUI();
}

void CBCFTopicForm::OnCommentDoubleClick()
{
	int selection = m_comments.GetCurSel();
	if (selection != LB_ERR) {
		m_pane->ShowComment(static_cast<BCFComment*>(m_comments.GetItemDataPtr(selection)));
	}
}

void CBCFTopicForm::OnTabChanged(NMHDR*, LRESULT* result)
{
	ShowTab(m_tabs.GetCurSel());
	*result = 0;
}

void CBCFTopicForm::OnViewProject()
{
	m_pane->ShowProject();
}

void CBCFTopicForm::OnSelectSnippetFile()
{
	if (!m_topic) {
		return;
	}

	BCFBimSnippet* snippet = m_topic->GetBimSnippet(false);
	CBCFSelectFileDlg dialog(
		snippet ? FromUTF8(snippet->GetReference()) : CString(),
		snippet && snippet->GetIsExternal(), this);
	if (dialog.DoModal() != IDOK) {
		return;
	}

	snippet = m_topic->GetBimSnippet(true);
	bool ok = snippet &&
		snippet->SetReference(ToUTF8(dialog.GetPathName()).c_str()) &&
		snippet->SetIsExternal(dialog.IsExternal());
	if (ok) {
		m_snippetExternal.SetCheck(dialog.IsExternal() ? BST_CHECKED : BST_UNCHECKED);
		m_snippetReference.SetWindowText(
			dialog.IsExternal() ? dialog.GetPathName() : dialog.GetFileName());
	}
	m_pane->ShowLog(!ok);
}

void CBCFTopicForm::OnSelectTopicLabels()
{
	if (m_topic && CBCFTopicLabelsDlg(*m_topic, this).DoModal() == IDOK) {
		UpdateLabels();
	}
}

HBRUSH CBCFTopicForm::OnCtlColor(CDC* dc, CWnd* window, UINT controlColor)
{
	if (controlColor == CTLCOLOR_STATIC &&
		window->IsKindOf(RUNTIME_CLASS(CStatic)) &&
		window->GetSafeHwnd() != m_topicInfo.GetSafeHwnd() &&
		window->GetSafeHwnd() != m_separator.GetSafeHwnd()) {
		dc->SetBkMode(TRANSPARENT);
		return static_cast<HBRUSH>(::GetStockObject(HOLLOW_BRUSH));
	}
	return CWnd::OnCtlColor(dc, window, controlColor);
}

void CBCFTopicForm::ShowTab(int tab)
{
	AdjustLayout();
	m_title.ShowWindow(tab == 0 ? SW_SHOW : SW_HIDE);
	m_descriptionLabel.ShowWindow(tab == 0 ? SW_SHOW : SW_HIDE);
	m_description.ShowWindow(tab == 0 ? SW_SHOW : SW_HIDE);
	CWnd* attributes[] = {
		&m_type, &m_stage, &m_status, &m_assigned, &m_priority, &m_due,
		&m_snippetType, &m_snippetExternal, &m_selectSnippetFile,
		&m_snippetReference, &m_snippetSchema,
		&m_index, &m_serverId, &m_selectTopicLabels
	};
	for (int i = 0; i < 12; ++i) {
		m_attributeLabels[i].ShowWindow(tab == 1 ? SW_SHOW : SW_HIDE);
	}
	for (CWnd* control : attributes) {
		control->ShowWindow(tab == 1 ? SW_SHOW : SW_HIDE);
	}
	m_labelsLabel.ShowWindow(tab == 1 ? SW_SHOW : SW_HIDE);
	m_labels.ShowWindow(tab == 1 ? SW_SHOW : SW_HIDE);
	m_snippetGroup.ShowWindow(tab == 1 ? SW_SHOW : SW_HIDE);
	if (tab == 1) {
		CComboBox* combos[] = { &m_type, &m_stage, &m_status, &m_assigned, &m_priority, &m_snippetType };
		for (CComboBox* combo : combos) {
			const int textLength = combo->GetWindowTextLength();
			combo->SetEditSel(textLength, textLength);
		}
	}
	m_comments.ShowWindow(tab == 2 ? SW_SHOW : SW_HIDE);
	m_documentsPlaceholder.ShowWindow(tab == 3 ? SW_SHOW : SW_HIDE);
	m_linksPlaceholder.ShowWindow(tab == 4 ? SW_SHOW : SW_HIDE);
	RedrawWindow(nullptr, nullptr,
		RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN | RDW_UPDATENOW);
}

void CBCFTopicForm::AdjustLayout()
{
	if (!m_tabs.GetSafeHwnd()) {
		return;
	}
	
	CRect client;
	GetClientRect(client);

	CClientDC dc(this);
	CFont* font = m_topicInfo.GetFont();
	CFont* oldFont = font ? dc.SelectObject(font) : nullptr;
	TEXTMETRIC textMetrics = {};
	dc.GetTextMetrics(&textMetrics);
	const int textHeight = textMetrics.tmAscent + textMetrics.tmDescent + textMetrics.tmExternalLeading;
	const int rowHeight = textHeight + textHeight / 5;
	const int margin = rowHeight / 3;
	const int labelOffset = (rowHeight - textHeight) / 2;
	const int separatorHeight = max(static_cast<int>(textMetrics.tmInternalLeading), rowHeight / 8);
	const int separatorTop = rowHeight + margin / 2;
	const int tabsTop = separatorTop + separatorHeight + margin / 2;
	const int tabsWidth = max(rowHeight, client.Width() - 2 * margin);
	const int tabsHeight = max(2 * rowHeight, client.Height() - tabsTop - margin);

	CString buttonText;
	m_viewProject.GetWindowText(buttonText);
	const int buttonWidth = static_cast<int>(dc.GetTextExtent(buttonText).cx) + 2 * margin;
	m_viewProject.MoveWindow(0, 0, buttonWidth, rowHeight);
	m_topicInfo.MoveWindow(buttonWidth + margin, labelOffset,
		max(rowHeight, client.Width() - buttonWidth - 2 * margin), textHeight);
	m_separator.MoveWindow(0, separatorTop, client.Width(), separatorHeight);
	m_tabs.MoveWindow(margin, tabsTop, tabsWidth, tabsHeight);

	CRect page(0, 0, tabsWidth, tabsHeight);
	m_tabs.AdjustRect(FALSE, page);
	page.OffsetRect(margin, tabsTop);
	page.DeflateRect(margin, margin);

	auto getLabelWidth = [&dc](CStatic& label) {
		CString text;
		label.GetWindowText(text);
		return static_cast<int>(dc.GetTextExtent(text).cx);
	};

	if (m_tabs.GetCurSel() == 0) {
		m_title.MoveWindow(page.left, page.top + labelOffset,
			page.Width(), rowHeight - labelOffset);

		const int descriptionLabelTop = page.top + rowHeight + margin;
		m_descriptionLabel.MoveWindow(page.left, descriptionLabelTop,
			getLabelWidth(m_descriptionLabel), textHeight);
		const int descriptionTop = descriptionLabelTop + textHeight + margin / 2;
		m_description.MoveWindow(page.left, descriptionTop, page.Width(),
			max(rowHeight, static_cast<int>(page.bottom) - descriptionTop));
	}
	else if (m_tabs.GetCurSel() == 1) {
		CWnd* leftControls[] = {
			&m_type, &m_stage, &m_status, &m_assigned, &m_priority, &m_due, &m_index, &m_serverId
		};
		const int leftLabelIndices[] = { 0, 1, 2, 3, 4, 5, 9, 10 };
		CWnd* snippetControls[] = {
			&m_snippetType, &m_snippetReference, &m_snippetExternal, &m_snippetSchema
		};
		const int snippetLabelIndices[] = { 6, 7, -1, 8 };
		int labelWidth = 0;
		for (int labelIndex : leftLabelIndices) {
			labelWidth = max(labelWidth, getLabelWidth(m_attributeLabels[labelIndex]));
		}
		labelWidth += margin;

		const int columnWidth = max(labelWidth + 3 * rowHeight, page.Width() / 2);
		for (size_t i = 0; i < _countof(leftControls); ++i) {
			const int top = page.top + static_cast<int>(i) * (rowHeight + margin / 2);
			m_attributeLabels[leftLabelIndices[i]].MoveWindow(
				page.left, top + labelOffset, labelWidth, textHeight);
			const bool isEdit = leftControls[i]->IsKindOf(RUNTIME_CLASS(CEdit)) != FALSE;
			leftControls[i]->MoveWindow(page.left + labelWidth, top + labelOffset,
				max(rowHeight, columnWidth - margin - labelWidth),
				isEdit ? rowHeight - labelOffset : static_cast<int>(_countof(leftControls)) * rowHeight);
		}

		const int groupLeft = page.left + columnWidth;
		const int groupWidth = max(rowHeight, static_cast<int>(page.right) - groupLeft);
		const int labelsLabelWidth = getLabelWidth(m_labelsLabel) + margin;
		m_labelsLabel.MoveWindow(
			groupLeft, page.top + labelOffset, labelsLabelWidth, textHeight);
		const int labelsButtonWidth = rowHeight;
		m_labels.MoveWindow(
			groupLeft + labelsLabelWidth, page.top + labelOffset,
			max(rowHeight, groupWidth - labelsLabelWidth - labelsButtonWidth - margin / 2),
			rowHeight - labelOffset);
		m_selectTopicLabels.MoveWindow(
			groupLeft + groupWidth - labelsButtonWidth, page.top,
			labelsButtonWidth, rowHeight);
		const int groupTop = page.top + rowHeight + margin / 2;
		const int groupHeight = textHeight + margin +
			static_cast<int>(_countof(snippetControls)) * (rowHeight + margin / 2) + margin;
		m_snippetGroup.MoveWindow(groupLeft, groupTop, groupWidth, groupHeight);

		int snippetLabelWidth = 0;
		for (int labelIndex : snippetLabelIndices) {
			if (labelIndex >= 0) {
				snippetLabelWidth = max(snippetLabelWidth, getLabelWidth(m_attributeLabels[labelIndex]));
			}
		}
		snippetLabelWidth += margin;
		const int groupContentLeft = groupLeft + margin;
		const int groupContentTop = groupTop + textHeight + margin / 2;
		const int groupContentWidth = max(rowHeight, groupWidth - 2 * margin);
		for (size_t i = 0; i < _countof(snippetControls); ++i) {
			const int top = groupContentTop + static_cast<int>(i) * (rowHeight + margin / 2);
			if (snippetLabelIndices[i] >= 0) {
				m_attributeLabels[snippetLabelIndices[i]].MoveWindow(
					groupContentLeft, top + labelOffset, snippetLabelWidth, textHeight);
			}
			const bool isEdit = snippetControls[i]->IsKindOf(RUNTIME_CLASS(CEdit)) != FALSE;
			const int verticalOffset = snippetControls[i] == &m_snippetExternal ? 0 : labelOffset;
			int controlWidth = max(rowHeight, groupContentWidth - snippetLabelWidth);
			if (snippetControls[i] == &m_snippetExternal) {
				CString text;
				m_snippetExternal.GetWindowText(text);
				controlWidth = ::GetSystemMetrics(SM_CXMENUCHECK) +
					static_cast<int>(dc.GetTextExtent(text).cx) + margin;
			}
			snippetControls[i]->MoveWindow(groupContentLeft + snippetLabelWidth, top + verticalOffset,
				controlWidth,
				isEdit ? rowHeight - labelOffset :
				(snippetControls[i] == &m_snippetExternal
					? rowHeight
					: static_cast<int>(_countof(leftControls)) * rowHeight));
			if (snippetControls[i] == &m_snippetExternal) {
				const int buttonLeft = groupContentLeft + snippetLabelWidth + controlWidth + margin / 2;
				m_selectSnippetFile.MoveWindow(
					buttonLeft, top, rowHeight, rowHeight);
			}
		}
	}
	else if (m_tabs.GetCurSel() == 2) {
		m_comments.MoveWindow(page);
	}
	else if (m_tabs.GetCurSel() == 3) {
		m_documentsPlaceholder.MoveWindow(page.left, page.top, page.Width(), textHeight);
	}
	else if (m_tabs.GetCurSel() == 4) {
		m_linksPlaceholder.MoveWindow(page.left, page.top, page.Width(), textHeight);
	}

	if (m_tabs.GetCurSel() == 1) {
		CComboBox* combos[] = { &m_type, &m_stage, &m_status, &m_assigned, &m_priority, &m_snippetType };
		for (CComboBox* combo : combos) {
			const int textLength = combo->GetWindowTextLength();
			combo->SetEditSel(textLength, textLength);
		}
	}

	if (oldFont) {
		dc.SelectObject(oldFont);
	}
}

void CBCFTopicForm::OnSize(UINT type, int cx, int cy)
{
	CWnd::OnSize(type, cx, cy);
	AdjustLayout();
}

BEGIN_MESSAGE_MAP(CBCFCommentForm, CWnd)
	ON_WM_SIZE()
END_MESSAGE_MAP()

BOOL CBCFCommentForm::Create(CBCFView* pane)
{
	m_pane = pane;
	if (!CreateEx(0, RegisterPaneClass(), L"", WS_CHILD | WS_CLIPCHILDREN, CRect(), pane, 0)) {
		return FALSE;
	}
	CreateStaticLabel(m_createdInfo, L"Created by", this);
	CreateStaticLabel(m_modifiedInfo, L"Modified by", this);
	CreateStaticLabel(m_textLabel, L"Comment:", this);
	m_text.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_MULTILINE | ES_AUTOVSCROLL |
		ES_WANTRETURN | WS_VSCROLL, CRect(), this, IDC_PANE_COMMENT_TEXT);
	SetControlFont(m_text, this);
	return TRUE;
}

void CBCFCommentForm::Load(BCFComment* comment)
{
	m_comment = comment;
	if (!GetSafeHwnd()) {
		return;
	}
	m_createdInfo.SetWindowText(comment ? FormatCommentCreated(*comment) : CString());
	m_modifiedInfo.SetWindowText(comment ? FormatCommentModified(*comment) : CString());
	m_text.SetWindowText(comment ? FromUTF8(comment->GetText()) : CString());
}

bool CBCFCommentForm::Commit()
{
	if (!m_comment) {
		return true;
	}
	CString text;
	m_text.GetWindowText(text);
	bool ok = m_comment->SetText(ToUTF8(text).c_str());
	m_pane->ShowLog(!ok);
	if (ok) {
		m_createdInfo.SetWindowText(FormatCommentCreated(*m_comment));
		m_modifiedInfo.SetWindowText(FormatCommentModified(*m_comment));
	}
	return ok;
}

void CBCFCommentForm::OnSize(UINT type, int cx, int cy)
{
	CWnd::OnSize(type, cx, cy);
	if (!m_text.GetSafeHwnd()) {
		return;
	}
	const int margin = 8;
	m_createdInfo.MoveWindow(margin, margin, max(20, cx - 2 * margin), 18);
	m_modifiedInfo.MoveWindow(margin, margin + 20, max(20, cx - 2 * margin), 18);
	m_textLabel.MoveWindow(margin, margin + 45, 80, 18);
	m_text.MoveWindow(margin, margin + 65, max(20, cx - 2 * margin), max(25, cy - 3 * margin - 65));
}

BEGIN_MESSAGE_MAP(CBCFView, CDockablePane)
	ON_WM_CREATE()
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
	ON_COMMAND(ID_BCF_FILE_NEW, &CBCFView::OnNewFile)
	ON_COMMAND(ID_BCF_FILE_OPEN, &CBCFView::OnOpenFile)
	ON_COMMAND(ID_BCF_FILE_SAVE, &CBCFView::OnSaveFile)
	ON_COMMAND(ID_BCF_PANE_ADD_TOPIC, &CBCFView::OnAddTopic)
	ON_COMMAND(ID_BCF_PANE_DELETE_TOPIC, &CBCFView::OnDeleteTopic)
	ON_COMMAND(ID_BCF_PANE_TOPIC_DETAILS, &CBCFView::OnTopicDetails)
	ON_COMMAND(ID_BCF_VIEW_PROJECT, &CBCFView::OnViewProject)
	ON_COMMAND(ID_BCF_VIEW_TOPIC, &CBCFView::OnViewTopic)
	ON_COMMAND(ID_BCF_VIEW_COMMENT, &CBCFView::OnViewComment)
	ON_COMMAND(ID_BCF_PANE_SAVE_COMMENT, &CBCFView::OnSaveComment)
	ON_COMMAND(ID_BCF_PANE_DELETE_COMMENT, &CBCFView::OnDeleteComment)
	ON_UPDATE_COMMAND_UI(ID_BCF_FILE_SAVE, &CBCFView::OnUpdateProjectCommand)
	ON_UPDATE_COMMAND_UI(ID_BCF_PANE_ADD_TOPIC, &CBCFView::OnUpdateProjectCommand)
	ON_UPDATE_COMMAND_UI(ID_BCF_PANE_DELETE_TOPIC, &CBCFView::OnUpdateTopicCommand)
	ON_UPDATE_COMMAND_UI(ID_BCF_PANE_TOPIC_DETAILS, &CBCFView::OnUpdateTopicCommand)
	ON_UPDATE_COMMAND_UI(ID_BCF_VIEW_PROJECT, &CBCFView::OnUpdateViewProject)
	ON_UPDATE_COMMAND_UI(ID_BCF_VIEW_TOPIC, &CBCFView::OnUpdateViewTopic)
	ON_UPDATE_COMMAND_UI(ID_BCF_VIEW_COMMENT, &CBCFView::OnUpdateViewComment)
	ON_UPDATE_COMMAND_UI(ID_BCF_PANE_SAVE_COMMENT, &CBCFView::OnUpdateCommentCommand)
	ON_UPDATE_COMMAND_UI(ID_BCF_PANE_DELETE_COMMENT, &CBCFView::OnUpdateCommentCommand)
END_MESSAGE_MAP()

CBCFView::CBCFView()
	: m_document(nullptr), m_project(nullptr), m_activeForm(ProjectForm)
{
}

CBCFView::~CBCFView()
{
	ReleaseProject();
}

int CBCFView::OnCreate(LPCREATESTRUCT createStruct)
{
	if (CDockablePane::OnCreate(createStruct) == -1) {
		return -1;
	}

	SetFont(m_dialogFont.CreatePointFont(80, L"MS Shell Dlg")
		? &m_dialogFont
		: &afxGlobalData.fontRegular);

	if (!m_menuBar.Create(this, AFX_DEFAULT_TOOLBAR_STYLE, IDR_BCF_VIEW_MENU) ||
		!m_menu.LoadMenu(IDR_BCF_VIEW_MENU)) {
		return -1;
	}
	m_menuBar.SetPaneStyle((m_menuBar.GetPaneStyle() | CBRS_TOOLTIPS | CBRS_FLYBY) &
		~(CBRS_GRIPPER | CBRS_SIZE_DYNAMIC | CBRS_BORDER_TOP | CBRS_BORDER_BOTTOM | CBRS_BORDER_LEFT | CBRS_BORDER_RIGHT));
	m_menuBar.SetOwner(this);
	m_menuBar.SetRouteCommandsViaFrame(FALSE);
	m_menuBar.SetDefaultMenuResId(IDR_BCF_VIEW_MENU);
	m_menuBar.SetMenuButtonRTC(RUNTIME_CLASS(CBCFMenuButton));
	m_menuBar.CreateFromMenu(m_menu.GetSafeHmenu(), TRUE);
	m_menuBar.SetMessageWnd(this);

	CreateStaticLabel(m_projectIdLabel, L"Project Id:", this);
	CreateStaticLabel(m_projectNameLabel, L"Name:", this);
	
	m_projectId.Create(WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_READONLY,
		CRect(), this, IDC_PANE_PROJECT_ID);
	m_projectName.Create(WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
		CRect(), this, IDC_PANE_PROJECT_NAME);

	SetControlFont(m_projectId, this);
	SetControlFont(m_projectName, this);
	
	if (!m_projectForm.Create(this) || !m_topicForm.Create(this) || !m_commentForm.Create(this)) {
		return -1;
	}
	
	ShowForm(ProjectForm);
	return 0;
}

BOOL CBCFView::OnEraseBkgnd(CDC* dc)
{
	CRect client;
	GetClientRect(client);
	dc->FillRect(client, CBrush::FromHandle(::GetSysColorBrush(COLOR_3DFACE)));
	return TRUE;
}

void CBCFView::Activate()
{
	ShowPane(TRUE, FALSE, TRUE);
	SetFocus();
}

void CBCFView::NewProject()
{
	if (!AskAndSaveModified()) {
		return;
	}
	ReleaseProject();
	m_project = BCFProject::Create();
	if (!m_project) {
		AfxMessageBox(L"Failed to initialize BCF project.", MB_OK | MB_ICONERROR);
		return;
	}
	m_email = AfxGetApp()->GetProfileString(L"BCF", L"User");
	m_project->SetOptions(ToUTF8(m_email).c_str(), true, true);
	m_filePath.Empty();
	LoadProjectInfo();
	UpdateCaption();
	ShowProject();
	Activate();
}

void CBCFView::OpenProject()
{
	CFileDialog dialog(TRUE, nullptr, L"", OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, BCF_PACKAGES_FILTER);
	if (dialog.DoModal() != IDOK || !AskAndSaveModified()) {
		return;
	}
	ReleaseProject();
	m_project = BCFProject::Create();
	if (!m_project) {
		AfxMessageBox(L"Failed to initialize BCF project.", MB_OK | MB_ICONERROR);
		return;
	}
	m_email = AfxGetApp()->GetProfileString(L"BCF", L"User");
	m_project->SetOptions(ToUTF8(m_email).c_str(), true, true);
	m_filePath = dialog.GetPathName();
	if (!m_project->ReadFile(ToUTF8(m_filePath).c_str(), true)) {
		ShowLog(true);
		ReleaseProject();
		return;
	}
	LoadProjectInfo();
	UpdateCaption();
	ShowProject();
	Activate();
}

bool CBCFView::CommitCurrent()
{
	if (!CommitProjectInfo()) {
		return false;
	}
	if (m_activeForm == ProjectForm) {
		return true;
	}
	if (m_activeForm == TopicForm) {
		return m_topicForm.Commit();
	}
	return m_commentForm.Commit();
}

bool CBCFView::SaveProject()
{
	if (!m_project || !CommitCurrent()) {
		return false;
	}
	CFileDialog dialog(FALSE, L"bcf", m_filePath, OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY,
		L"BCF files (*.bcf)|*.bcf|BCF packages (*.bcfzip)|*.bcfzip|All Files (*.*)|*.*||");
	if (dialog.DoModal() != IDOK) {
		return false;
	}
	CString path = dialog.GetPathName();
	bool ok = m_project->WriteFile(ToUTF8(path).c_str(), BCFVer_3_0);
	ShowLog(!ok);
	if (ok) {
		m_filePath = path;
		UpdateCaption();
		m_projectForm.Load();
	}
	return ok;
}

bool CBCFView::AskAndSaveModified()
{
	if (!CommitCurrent()) {
		return false;
	}
	if (!m_project || !m_project->IsModified()) {
		return true;
	}
	int answer = AfxMessageBox(L"BCF pane project is modified. Do you want to save it?",
		MB_YESNOCANCEL | MB_ICONQUESTION);
	return answer == IDYES ? SaveProject() : answer == IDNO;
}

void CBCFView::CloseProject(bool prompt)
{
	if (!prompt || AskAndSaveModified()) {
		ReleaseProject();
		ShowProject();
	}
}

void CBCFView::OnCloseMainDocument()
{
	CloseProject(false);
	m_document = nullptr;
}

void CBCFView::ReleaseProject()
{
	if (m_project) {
		ShowLog(false);
		m_project->Delete();
		m_project = nullptr;
	}
	m_filePath.Empty();
	m_bimModels.clear();
	LoadProjectInfo();
	UpdateCaption();
	m_topicForm.Load(nullptr);
	m_commentForm.Load(nullptr);
}

void CBCFView::ShowProject()
{
	if (!CommitProjectInfo()) {
		return;
	}
	if (m_activeForm == TopicForm && !m_topicForm.Commit()) {
		return;
	}
	if (m_activeForm == CommentForm) {
		if (!m_commentForm.Commit()) {
			return;
		}
		m_topicForm.ReloadComments(m_commentForm.GetComment());
	}
	m_projectForm.Load();
	ShowForm(ProjectForm);
}

void CBCFView::ShowTopic(BCFTopic* topic)
{
	if (!topic || !CommitProjectInfo()) {
		return;
	}
	if (m_activeForm == ProjectForm) {
		if (!m_projectForm.Commit()) {
			return;
		}
	}
	else if (m_activeForm == CommentForm) {
		BCFComment* comment = m_commentForm.GetComment();
		if (!m_commentForm.Commit()) {
			return;
		}
		if (m_topicForm.GetTopic() == topic) {
			m_topicForm.ReloadComments(comment);
			ShowForm(TopicForm);
			return;
		}
	}
	m_topicForm.Load(topic);
	ShowForm(TopicForm);
}

void CBCFView::ShowComment(BCFComment* comment)
{
	if (!comment || !CommitProjectInfo() || !m_topicForm.Commit()) {
		return;
	}
	m_commentForm.Load(comment);
	ShowForm(CommentForm);
}

void CBCFView::ShowForm(Form form)
{
	m_activeForm = form;
	m_projectForm.ShowWindow(form == ProjectForm ? SW_SHOW : SW_HIDE);
	m_topicForm.ShowWindow(form == TopicForm ? SW_SHOW : SW_HIDE);
	m_commentForm.ShowWindow(form == CommentForm ? SW_SHOW : SW_HIDE);
	AdjustLayout();
	RefreshCommandUI();
}

void CBCFView::LoadProjectInfo()
{
	if (!m_projectId.GetSafeHwnd()) {
		return;
	}
	m_projectId.SetWindowText(m_project ? FromUTF8(m_project->GetProjectId()) : CString());
	m_projectName.SetWindowText(m_project ? FromUTF8(m_project->GetName()) : CString());
}

bool CBCFView::CommitProjectInfo()
{
	if (!m_project) {
		return true;
	}
	CString name;
	m_projectName.GetWindowText(name);
	name.Trim();
	if (name == FromUTF8(m_project->GetName())) {
		return true;
	}
	bool ok = m_project->SetName(ToUTF8(name).c_str());
	ShowLog(!ok);
	return ok;
}

void CBCFView::UpdateCaption()
{
	CString caption(L"BCF View");
	if (!m_filePath.IsEmpty()) {
		fs::path path(ToUTF8(m_filePath));
		caption.AppendFormat(L" - %s", FromUTF8(path.filename().string().c_str()).GetString());
	}
	SetWindowText(caption);
}

void CBCFView::OnAddTopic()
{
	if (!m_project) {
		return;
	}
	if (!m_projectForm.Commit()) {
		return;
	}
	BCFTopic* topic = m_project->AddTopic(nullptr, nullptr, nullptr);
	ShowLog(!topic);
	if (topic) {
		m_projectForm.Load(topic);
		ShowTopic(topic);
	}
}

void CBCFView::OnDeleteTopic()
{
	BCFTopic* topic = m_projectForm.GetSelectedTopic();
	if (!topic) {
		return;
	}
	CString question;
	question.Format(L"Delete topic \"%s\"?", FromUTF8(topic->GetTitle()).GetString());
	if (AfxMessageBox(question, MB_YESNO | MB_ICONWARNING) == IDYES) {
		bool ok = topic->Remove();
		ShowLog(!ok);
		if (ok) {
			m_projectForm.Load();
		}
	}
}

void CBCFView::OnTopicDetails()
{
	ShowTopic(m_projectForm.GetSelectedTopic());
}

void CBCFView::OnViewProject() { ShowProject(); }
void CBCFView::OnViewTopic() { ShowTopic(m_projectForm.GetSelectedTopic()); }
void CBCFView::OnViewComment() { ShowComment(m_topicForm.GetSelectedComment()); }

void CBCFView::OnSaveComment()
{
	if (m_commentForm.Commit()) {
		m_topicForm.ReloadComments(m_commentForm.GetComment());
	}
}

void CBCFView::OnDeleteComment()
{
	BCFComment* comment = m_commentForm.GetComment();
	if (!comment || AfxMessageBox(L"Delete this comment?", MB_YESNO | MB_ICONWARNING) != IDYES) {
		return;
	}
	bool ok = comment->Remove();
	ShowLog(!ok);
	if (ok) {
		m_commentForm.Load(nullptr);
		m_topicForm.ReloadComments();
		ShowForm(TopicForm);
	}
}

void CBCFView::ShowLog(bool knownError)
{
	const char* message = m_project ? m_project->GetErrors() : nullptr;
	if (knownError && (!message || !*message)) {
		message = "Unknown BCF error";
	}
	if (message && *message) {
		AfxMessageBox(FromUTF8(message), knownError ? MB_ICONERROR : MB_ICONWARNING);
	}
}

_model* CBCFView::GetBimModel(BCFBimFile& file)
{
	auto found = m_bimModels.find(&file);
	if (found != m_bimModels.end()) {
		if (m_document) {
			const auto& models = m_document->getModels();
			if (std::find(models.begin(), models.end(), found->second) != models.end()) {
				return found->second;
			}
		}
		m_bimModels.erase(found);
	}
	if (!m_document) {
		return nullptr;
	}
	CString path = FromUTF8(file.GetReference());
	for (_model* candidate : m_document->getModels()) {
		if (candidate->getPath() == path) {
			m_bimModels[&file] = candidate;
			return candidate;
		}
	}
	if (!fs::exists(ToUTF8(path))) {
		CString message(L"Can not locate BIM file assigned to the topic.\n\n");
		message.AppendFormat(L"Reference: '%s'\n\nDo you want to locate the file manually?", path.GetString());
		if (AfxMessageBox(message, MB_YESNO | MB_ICONEXCLAMATION) != IDYES) {
			return nullptr;
		}
		CFileDialog dialog(TRUE, nullptr, L"", OFN_FILEMUSTEXIST, BIM_MODELS_FILTER);
		if (dialog.DoModal() != IDOK) {
			return nullptr;
		}
		path = dialog.GetPathName();
	}
	_model* model = _ap_model_factory::load(m_document, path, false,
		m_document->getModels().empty() ? nullptr : m_document->getModels()[0], false);
	if (model) {
		_ptr<_ap_model> apModel(model);
		if (apModel->getAP() == enumAP::IFC) {
			m_bimModels[&file] = model;
		}
		else {
			delete model;
			model = nullptr;
		}
	}
	return model;
}

void CBCFView::LoadBimFiles(BCFTopic& topic)
{
	if (!m_document) {
		return;
	}
	std::vector<_model*> activeModels;
	for (uint16_t i = 0; BCFBimFile* file = topic.GetBimFile(i); ++i) {
		if (_model* model = GetBimModel(*file)) {
			activeModels.push_back(model);
		}
	}
	m_document->enableModelsAddIfNeeded(activeModels);
}

void CBCFView::RefreshCommandUI()
{
	if (m_menuBar.GetSafeHwnd()) {
		m_menuBar.OnUpdateCmdUI(nullptr, FALSE);
		m_menuBar.Invalidate();
	}
}

void CBCFView::OnUpdateProjectCommand(CCmdUI* commandUI)
{
	commandUI->Enable(m_activeForm == ProjectForm && m_project != nullptr);
}

void CBCFView::OnUpdateTopicCommand(CCmdUI* commandUI)
{
	commandUI->Enable(m_activeForm == ProjectForm && m_projectForm.GetSelectedTopic() != nullptr);
}

void CBCFView::OnUpdateCommentCommand(CCmdUI* commandUI)
{
	commandUI->Enable(m_activeForm == CommentForm && m_commentForm.GetComment() != nullptr);
}

void CBCFView::OnUpdateViewProject(CCmdUI* commandUI)
{
	commandUI->Enable(TRUE);
	commandUI->SetRadio(m_activeForm == ProjectForm);
}

void CBCFView::OnUpdateViewTopic(CCmdUI* commandUI)
{
	commandUI->Enable(m_projectForm.GetSelectedTopic() != nullptr);
	commandUI->SetRadio(m_activeForm == TopicForm);
}

void CBCFView::OnUpdateViewComment(CCmdUI* commandUI)
{
	const bool hasSelectedComment = m_topicForm.GetTopic() == m_projectForm.GetSelectedTopic() &&
		m_topicForm.GetSelectedComment() != nullptr;
	commandUI->Enable(hasSelectedComment);
	commandUI->SetRadio(m_activeForm == CommentForm);
}

void CBCFView::OnNewFile() { NewProject(); }
void CBCFView::OnOpenFile() { OpenProject(); }
void CBCFView::OnSaveFile() { SaveProject(); }

void CBCFView::AdjustLayout()
{
	if (!GetSafeHwnd() || !m_menuBar.GetSafeHwnd()) {
		return;
	}
	CRect client;
	GetClientRect(client);
	CSize menuSize = m_menuBar.CalcFixedLayout(FALSE, TRUE);
	m_menuBar.SetWindowPos(nullptr, client.left, client.top, client.Width(), menuSize.cy,
		SWP_NOACTIVATE | SWP_NOZORDER);

	CClientDC dc(this);
	CFont* font = m_projectIdLabel.GetFont();
	CFont* oldFont = font ? dc.SelectObject(font) : nullptr;
	TEXTMETRIC textMetrics = {};
	dc.GetTextMetrics(&textMetrics);
	const int textHeight = textMetrics.tmAscent + textMetrics.tmDescent + textMetrics.tmExternalLeading;
	const int rowHeight = textHeight + textHeight / 2;
	const int margin = rowHeight / 3;
	const int labelOffset = (rowHeight - textHeight) / 2;
	const int headerTop = client.top + menuSize.cy + margin;

	CString idLabelText;
	CString nameLabelText;
	CString projectIdText;
	m_projectIdLabel.GetWindowText(idLabelText);
	m_projectNameLabel.GetWindowText(nameLabelText);
	m_projectId.GetWindowText(projectIdText);
	const int idLabelWidth = dc.GetTextExtent(idLabelText).cx + margin;
	const int nameLabelWidth = dc.GetTextExtent(nameLabelText).cx + margin;
	const DWORD projectIdMargins = m_projectId.GetMargins();
	const int projectIdWidth = max(rowHeight,
		static_cast<int>(dc.GetTextExtent(projectIdText).cx) +
		LOWORD(projectIdMargins) + HIWORD(projectIdMargins));
	if (oldFont) {
		dc.SelectObject(oldFont);
	}

	int headerHeight = rowHeight;
	const int minEditWidth = 3 * rowHeight;
	const int twoColumnWidth =
		3 * margin + idLabelWidth + projectIdWidth + nameLabelWidth + minEditWidth;
	if (client.Width() >= twoColumnWidth) {
		m_projectIdLabel.MoveWindow(margin, headerTop + labelOffset, idLabelWidth, textHeight);
		m_projectId.MoveWindow(margin + idLabelWidth, headerTop + labelOffset,
			projectIdWidth, rowHeight - labelOffset);
		const int second = 2 * margin + idLabelWidth + projectIdWidth;
		m_projectNameLabel.MoveWindow(second, headerTop + labelOffset, nameLabelWidth, textHeight);
		m_projectName.MoveWindow(second + nameLabelWidth, headerTop + labelOffset,
			client.right - second - nameLabelWidth - margin, rowHeight - labelOffset);
	}
	else {
		const int labelWidth = max(idLabelWidth, nameLabelWidth);
		const int editWidth = max(rowHeight, client.Width() - 2 * margin - labelWidth);
		m_projectIdLabel.MoveWindow(margin, headerTop + labelOffset, labelWidth, textHeight);
		m_projectId.MoveWindow(margin + labelWidth, headerTop + labelOffset,
			min(projectIdWidth, editWidth), rowHeight - labelOffset);
		m_projectNameLabel.MoveWindow(margin, headerTop + rowHeight + labelOffset, labelWidth, textHeight);
		m_projectName.MoveWindow(margin + labelWidth, headerTop + rowHeight + labelOffset,
			editWidth, rowHeight - labelOffset);
		headerHeight = 2 * rowHeight;
	}
	CRect formRect(client.left, headerTop + headerHeight + margin, client.right, client.bottom);
	m_projectForm.MoveWindow(formRect);
	m_topicForm.MoveWindow(formRect);
	m_commentForm.MoveWindow(formRect);
	RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
}

void CBCFView::OnSize(UINT type, int cx, int cy)
{
	CDockablePane::OnSize(type, cx, cy);
	AdjustLayout();
}

void CBCFView::OnSetFocus(CWnd*)
{
	if (m_activeForm == ProjectForm) {
		m_projectForm.SetFocus();
	}
	else if (m_activeForm == TopicForm) {
		m_topicForm.SetFocus();
	}
	else {
		m_commentForm.SetFocus();
	}
}
