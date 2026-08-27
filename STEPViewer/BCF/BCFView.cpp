#include "stdafx.h"

#include "BCFView.h"
#include "BCFViewPointMgr.h"
#include "STEPViewerDoc.h"
#include "Resource.h"
#include "_ap_model_factory.h"
#include "_ptr.h"

#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

namespace
{
	enum ControlId
	{
		IDC_PANE_PROJECT_ID = 2100,
		IDC_PANE_PROJECT_NAME,
		IDC_PANE_TOPICS,
		IDC_PANE_TABS,
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
			reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1), nullptr);
		return className;
	}

	void SetControlFont(CWnd& control, CWnd* owner)
	{
		if (owner && owner->GetFont()) {
			control.SetFont(owner->GetFont());
		}
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

void CBCFProjectForm::Load()
{
	BCFTopic* selected = GetSelectedTopic();
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

	CreateStaticLabel(m_topicInfo, L"Topic", this);

	m_tabs.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | TCS_TABS, CRect(), this, IDC_PANE_TABS);	
	SetControlFont(m_tabs, this);
	
	m_tabs.InsertItem(0, L"Topic");
	m_tabs.InsertItem(1, L"Attributes");
	m_tabs.InsertItem(2, L"Comments");
	m_tabs.InsertItem(3, L"Documents");
	m_tabs.InsertItem(4, L"Links");

	CreateStaticLabel(m_titleLabel, L"Title:", this);
	m_title.Create(WS_CHILD | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL, CRect(), this, IDC_PANE_TOPIC_TITLE);

	CreateStaticLabel(m_descriptionLabel, L"Description:", this);
	m_description.Create(WS_CHILD | WS_BORDER | WS_TABSTOP | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN | WS_VSCROLL,
		CRect(), this, IDC_PANE_TOPIC_DESCRIPTION);

	SetControlFont(m_title, this);
	SetControlFont(m_description, this);

	const wchar_t* labels[12] = {
		L"Type:", L"Stage:", L"Status:", L"Assigned:", L"Priority:", L"Due:",
		L"Snippet:", L"Reference:", L"Schema:", L"Index:", L"Server Id:", L""
	};

	for (int i = 0; i < 12; ++i) {
		CreateStaticLabel(m_attributeLabels[i], labels[i], this);
	}

	DWORD comboStyle = WS_CHILD | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWN;
	m_type.Create(comboStyle, CRect(), this, IDC_PANE_TOPIC_TYPE);
	m_stage.Create(comboStyle, CRect(), this, IDC_PANE_TOPIC_STAGE);
	m_status.Create(comboStyle, CRect(), this, IDC_PANE_TOPIC_STATUS);
	m_assigned.Create(comboStyle, CRect(), this, IDC_PANE_TOPIC_ASSIGNED);
	m_priority.Create(comboStyle, CRect(), this, IDC_PANE_TOPIC_PRIORITY);
	m_snippet.Create(comboStyle, CRect(), this, IDC_PANE_TOPIC_SNIPPET);
	CWnd* combos[] = { &m_type, &m_stage, &m_status, &m_assigned, &m_priority, &m_snippet };
	for (CWnd* combo : combos) {
		SetControlFont(*combo, this);
	}
	DWORD editStyle = WS_CHILD | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL;
	m_due.Create(editStyle, CRect(), this, IDC_PANE_TOPIC_DUE);
	m_reference.Create(editStyle, CRect(), this, IDC_PANE_TOPIC_REFERENCE);
	m_schema.Create(editStyle, CRect(), this, IDC_PANE_TOPIC_SCHEMA);
	m_index.Create(editStyle, CRect(), this, IDC_PANE_TOPIC_INDEX);
	m_serverId.Create(editStyle, CRect(), this, IDC_PANE_TOPIC_SERVER_ID);
	CWnd* edits[] = { &m_due, &m_reference, &m_schema, &m_index, &m_serverId };
	for (CWnd* edit : edits) {
		SetControlFont(*edit, this);
	}
	m_comments.Create(WS_CHILD | WS_BORDER | WS_TABSTOP | WS_VSCROLL | LBS_NOTIFY | LBS_OWNERDRAWVARIABLE |
		LBS_HASSTRINGS | LBS_NOINTEGRALHEIGHT, CRect(), this, IDC_PANE_COMMENTS);
	SetControlFont(m_comments, this);
	CreateStaticLabel(m_documentsPlaceholder, L"Documents are not available in this version.", this);
	CreateStaticLabel(m_linksPlaceholder, L"Links are not available in this version.", this);
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
	LoadExtension(m_snippet, BCFSnippetTypes);
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
	BCFBimSnippet* snippet = topic->GetBimSnippet(false);
	m_snippet.SetWindowText(snippet ? FromUTF8(snippet->GetSnippetType()) : CString());
	m_reference.SetWindowText(snippet ? FromUTF8(snippet->GetReference()) : CString());
	m_schema.SetWindowText(snippet ? FromUTF8(snippet->GetReferenceSchema()) : CString());
	UpdateMetadata();
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
	CString title, description, type, stage, status, assigned, priority, due, snippetType, reference, schema, index, serverId;
	m_title.GetWindowText(title);
	m_description.GetWindowText(description);
	m_type.GetWindowText(type);
	m_stage.GetWindowText(stage);
	m_status.GetWindowText(status);
	m_assigned.GetWindowText(assigned);
	m_priority.GetWindowText(priority);
	m_due.GetWindowText(due);
	m_snippet.GetWindowText(snippetType);
	m_reference.GetWindowText(reference);
	m_schema.GetWindowText(schema);
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
	if (snippetType.IsEmpty() && reference.IsEmpty() && schema.IsEmpty()) {
		if (BCFBimSnippet* snippet = m_topic->GetBimSnippet(false)) {
			ok = snippet->Remove() && ok;
		}
	}
	else {
		if (BCFBimSnippet* snippet = m_topic->GetBimSnippet(true)) {
			ok = snippet->SetSnippetType(ToUTF8(snippetType).c_str()) && ok;
			ok = snippet->SetReference(ToUTF8(reference).c_str()) && ok;
			ok = snippet->SetReferenceSchema(ToUTF8(schema).c_str()) && ok;
		}
		else {
			ok = false;
		}
	}
	m_pane->ShowLog(!ok);
	UpdateMetadata();
	return ok;
}

void CBCFTopicForm::UpdateMetadata()
{
	if (!m_topic) {
		return;
	}
	CString value;
	value.Format(L"Topic %s. Created by %s %s", FromUTF8(m_topic->GetGuid()).GetString(),
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

void CBCFTopicForm::OnCommentChanged()
{
	int selection = m_comments.GetCurSel();
	if (selection != LB_ERR) {
		BCFComment* comment = static_cast<BCFComment*>(m_comments.GetItemDataPtr(selection));
		if (comment && m_pane->GetDocument()) {
			CBCFViewPointMgr(*m_pane->GetDocument()).SetViewFromComment(*comment);
			m_pane->ShowLog(false);
		}
	}
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

void CBCFTopicForm::ShowTab(int tab)
{
	m_titleLabel.ShowWindow(tab == 0 ? SW_SHOW : SW_HIDE);
	m_title.ShowWindow(tab == 0 ? SW_SHOW : SW_HIDE);
	m_descriptionLabel.ShowWindow(tab == 0 ? SW_SHOW : SW_HIDE);
	m_description.ShowWindow(tab == 0 ? SW_SHOW : SW_HIDE);
	CWnd* attributes[] = {
		&m_type, &m_stage, &m_status, &m_assigned, &m_priority, &m_due,
		&m_snippet, &m_reference, &m_schema, &m_index, &m_serverId
	};
	for (int i = 0; i < 12; ++i) {
		m_attributeLabels[i].ShowWindow(tab == 1 ? SW_SHOW : SW_HIDE);
	}
	for (CWnd* control : attributes) {
		control->ShowWindow(tab == 1 ? SW_SHOW : SW_HIDE);
	}
	m_comments.ShowWindow(tab == 2 ? SW_SHOW : SW_HIDE);
	m_documentsPlaceholder.ShowWindow(tab == 3 ? SW_SHOW : SW_HIDE);
	m_linksPlaceholder.ShowWindow(tab == 4 ? SW_SHOW : SW_HIDE);
	Layout();
}

void CBCFTopicForm::Layout()
{
	if (!m_tabs.GetSafeHwnd()) {
		return;
	}
	CRect client;
	GetClientRect(client);
	const int margin = 8;
	m_topicInfo.MoveWindow(margin, 5, max(20, client.Width() - 2 * margin), 18);
	m_tabs.MoveWindow(margin, 25, max(20, client.Width() - 2 * margin), max(30, client.Height() - 33));
	CRect page(margin + 10, 56,
		max(margin + 30, static_cast<int>(client.right) - margin - 10),
		max(113, static_cast<int>(client.bottom) - margin - 10));
	if (m_tabs.GetCurSel() == 0) {
		m_titleLabel.MoveWindow(page.left, page.top + 3, 75, 18);
		m_title.MoveWindow(page.left + 78, page.top, max(20, page.Width() - 78), 23);
		m_descriptionLabel.MoveWindow(page.left, page.top + 32, 75, 18);
		m_description.MoveWindow(page.left, page.top + 52, page.Width(),
			max(25, static_cast<int>(page.bottom - page.top) - 52));
	}
	else if (m_tabs.GetCurSel() == 1) {
		CWnd* controls[] = {
			&m_type, &m_stage, &m_status, &m_assigned, &m_priority, &m_due,
			&m_snippet, &m_reference, &m_schema, &m_index, &m_serverId
		};
		int rowHeight = 26;
		int labelWidth = 78;
		int columnWidth = max(150, page.Width() / 2);
		for (int i = 0; i < 11; ++i) {
			int column = i / 6;
			int row = i % 6;
			int left = page.left + column * columnWidth;
			int width = column == 0 ? columnWidth - 8 : page.right - left;
			m_attributeLabels[i].MoveWindow(left, page.top + row * rowHeight + 4, labelWidth, 18);
			controls[i]->MoveWindow(left + labelWidth, page.top + row * rowHeight,
				max(20, width - labelWidth), 200);
		}
	}
	else if (m_tabs.GetCurSel() == 2) {
		m_comments.MoveWindow(page);
	}
	else if (m_tabs.GetCurSel() == 3) {
		m_documentsPlaceholder.MoveWindow(page.left, page.top, page.Width(), 20);
	}
	else if (m_tabs.GetCurSel() == 4) {
		m_linksPlaceholder.MoveWindow(page.left, page.top, page.Width(), 20);
	}
}

void CBCFTopicForm::OnSize(UINT type, int cx, int cy)
{
	CWnd::OnSize(type, cx, cy);
	Layout();
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
	m_text.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_MULTILINE | ES_AUTOVSCROLL |
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
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
	ON_COMMAND(ID_BCF_FILE_NEW, &CBCFView::OnNewFile)
	ON_COMMAND(ID_BCF_FILE_OPEN, &CBCFView::OnOpenFile)
	ON_COMMAND(ID_BCF_FILE_SAVE, &CBCFView::OnSaveFile)
	ON_COMMAND(ID_BCF_PANE_ADD_TOPIC, &CBCFView::OnAddTopic)
	ON_COMMAND(ID_BCF_PANE_DELETE_TOPIC, &CBCFView::OnDeleteTopic)
	ON_COMMAND(ID_BCF_PANE_TOPIC_DETAILS, &CBCFView::OnTopicDetails)
	ON_COMMAND(ID_BCF_PANE_BACK, &CBCFView::OnBack)
	ON_COMMAND(ID_BCF_PANE_SAVE_COMMENT, &CBCFView::OnSaveComment)
	ON_COMMAND(ID_BCF_PANE_DELETE_COMMENT, &CBCFView::OnDeleteComment)
	ON_UPDATE_COMMAND_UI(ID_BCF_FILE_SAVE, &CBCFView::OnUpdateProjectCommand)
	ON_UPDATE_COMMAND_UI(ID_BCF_PANE_ADD_TOPIC, &CBCFView::OnUpdateProjectCommand)
	ON_UPDATE_COMMAND_UI(ID_BCF_PANE_DELETE_TOPIC, &CBCFView::OnUpdateTopicCommand)
	ON_UPDATE_COMMAND_UI(ID_BCF_PANE_TOPIC_DETAILS, &CBCFView::OnUpdateTopicCommand)
	ON_UPDATE_COMMAND_UI(ID_BCF_PANE_BACK, &CBCFView::OnUpdateBack)
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

	if (!m_menuBar.Create(this, AFX_DEFAULT_TOOLBAR_STYLE, IDR_BCF_VIEW_MENU) ||
		!m_menu.LoadMenu(IDR_BCF_VIEW_MENU)) {
		return -1;
	}
	m_menuBar.SetPaneStyle((m_menuBar.GetPaneStyle() | CBRS_TOOLTIPS | CBRS_FLYBY) &
		~(CBRS_GRIPPER | CBRS_SIZE_DYNAMIC | CBRS_BORDER_TOP | CBRS_BORDER_BOTTOM | CBRS_BORDER_LEFT | CBRS_BORDER_RIGHT));
	m_menuBar.SetOwner(this);
	m_menuBar.SetRouteCommandsViaFrame(FALSE);
	m_menuBar.SetDefaultMenuResId(IDR_BCF_VIEW_MENU);
	m_menuBar.CreateFromMenu(m_menu.GetSafeHmenu(), TRUE);
	m_menuBar.SetMessageWnd(this);

	CreateStaticLabel(m_projectIdLabel, L"Project Id:", this);
	CreateStaticLabel(m_projectNameLabel, L"Project Name:", this);
	
	m_projectId.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_READONLY,
		CRect(), this, IDC_PANE_PROJECT_ID);
	m_projectName.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
		CRect(), this, IDC_PANE_PROJECT_NAME);

	SetControlFont(m_projectId, this);
	SetControlFont(m_projectName, this);
	
	if (!m_projectForm.Create(this) || !m_topicForm.Create(this) || !m_commentForm.Create(this)) {
		return -1;
	}
	
	ShowForm(ProjectForm);
	return 0;
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
		BCFTopic* topic = m_topicForm.GetTopic();
		if (!m_commentForm.Commit()) {
			return;
		}
		m_topicForm.ReloadComments(m_commentForm.GetComment());
		m_topicForm.Load(topic);
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
		if (!m_commentForm.Commit()) {
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
		m_projectForm.Load();
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

void CBCFView::OnBack()
{
	if (!CommitProjectInfo()) {
		return;
	}
	if (m_activeForm == CommentForm) {
		BCFComment* comment = m_commentForm.GetComment();
		if (m_commentForm.Commit()) {
			m_topicForm.ReloadComments(comment);
			ShowForm(TopicForm);
		}
	}
	else if (m_activeForm == TopicForm && m_topicForm.Commit()) {
		m_projectForm.Load();
		ShowForm(ProjectForm);
	}
}

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

void CBCFView::OnUpdateBack(CCmdUI* commandUI)
{
	commandUI->Enable(m_activeForm != ProjectForm);
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
	const int margin = 8;
	const int labelWidth = 78;
	const int rowHeight = 23;
	int headerTop = client.top + menuSize.cy + margin;
	int half = max(100, (client.Width() - 3 * margin) / 2);
	m_projectIdLabel.MoveWindow(margin, headerTop + 4, labelWidth, 18);
	m_projectId.MoveWindow(margin + labelWidth, headerTop, max(20, half - labelWidth), rowHeight);
	int second = 2 * margin + half;
	m_projectNameLabel.MoveWindow(second, headerTop + 4, labelWidth, 18);
	m_projectName.MoveWindow(second + labelWidth, headerTop,
		max(20, static_cast<int>(client.right) - second - labelWidth - margin), rowHeight);
	CRect formRect(client.left, headerTop + rowHeight + margin, client.right, client.bottom);
	m_projectForm.MoveWindow(formRect);
	m_topicForm.MoveWindow(formRect);
	m_commentForm.MoveWindow(formRect);
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
