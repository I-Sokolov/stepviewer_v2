#include "stdafx.h"

#include "BCFTopicForm.h"
#include "BCFAddDocumentReference.h"
#include "BCFAddReferenceLink.h"
#include "BCFAddRelatedTopic.h"
#include "BCFTopicLabelsDlg.h"
#include "BCFView.h"
#include "BCFViewPointMgr.h"
#include "STEPViewerDoc.h"
#include "Resource.h"
#include "_ap_model_factory.h"

#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;

namespace
{
	void UpdateHorizontalExtent(CListBox& list, int leadingWidth = 0)
	{
		CClientDC dc(&list);
		CFont* oldFont = list.GetFont() ? dc.SelectObject(list.GetFont()) : nullptr;
		int extent = 0;
		CString text;
		for (int i = 0; i < list.GetCount(); ++i) {
			list.GetText(i, text);
			extent = max(extent, static_cast<int>(dc.GetTextExtent(text).cx));
		}
		if (oldFont) {
			dc.SelectObject(oldFont);
		}
		list.SetHorizontalExtent(extent ? extent + leadingWidth + 4 : 0);
	}

	CString GetDocumentText(BCFDocumentReference& document)
	{
		CString text = FromUTF8(document.GetDescription());
		if (!text.IsEmpty()) {
			text.Append(L": ");
		}
		fs::path path = document.GetFilePath();
		text += FromUTF8(path.filename().string().c_str());
		return text;
	}

	CString GetBimFileText(BCFBimFile& file)
	{
		auto name = file.GetReference();
		if (!name || !*name) {
			name = file.GetFilename();
		}

		if (file.GetIsExternal()) {
			return FromUTF8(name);
		}
		else {
			const fs::path reference(name);
			return FromUTF8(reference.filename().string().c_str()) + CString (L" (embedded)");
		}
	}

	bool IsTopicEditControl(UINT id)
	{
		switch (id) {
		case IDC_PANE_TOPIC_TITLE:
		case IDC_PANE_TOPIC_DESCRIPTION:
		case IDC_PANE_TOPIC_DUE:
		case IDC_PANE_TOPIC_SCHEMA:
		case IDC_PANE_TOPIC_INDEX:
		case IDC_PANE_TOPIC_SERVER_ID:
			return true;
		default:
			return false;
		}
	}

	bool IsTopicComboControl(UINT id)
	{
		switch (id) {
		case IDC_PANE_TOPIC_TYPE:
		case IDC_PANE_TOPIC_STAGE:
		case IDC_PANE_TOPIC_STATUS:
		case IDC_PANE_TOPIC_ASSIGNED:
		case IDC_PANE_TOPIC_PRIORITY:
		case IDC_PANE_TOPIC_SNIPPET:
			return true;
		default:
			return false;
		}
	}
}

BEGIN_MESSAGE_MAP(CBCFTopicForm, CWnd)
	ON_WM_SIZE()
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_PANE_SHOW_BCF_CONTENT, &CBCFTopicForm::OnShowBCFContent)
	ON_BN_CLICKED(IDC_PANE_SELECT_SNIPPET_FILE, &CBCFTopicForm::OnSelectSnippetFile)
	ON_BN_CLICKED(IDC_PANE_SELECT_TOPIC_LABELS, &CBCFTopicForm::OnSelectTopicLabels)
	ON_BN_CLICKED(IDC_PANE_ADD_BIM_FILES, &CBCFTopicForm::OnAddBimFiles)
	ON_CONTROL(CLBN_CHKCHANGE, IDC_PANE_BIM_FILES, &CBCFTopicForm::OnCheckBimFiles)
	ON_NOTIFY(TCN_SELCHANGE, IDC_PANE_TABS, &CBCFTopicForm::OnTabChanged)
	ON_CONTROL(LBN_SELCHANGE, IDC_PANE_COMMENTS, &CBCFTopicForm::OnCommentChanged)
	ON_CONTROL(LBN_DBLCLK, IDC_PANE_COMMENTS, &CBCFTopicForm::OnCommentDoubleClick)
	ON_BN_CLICKED(IDC_PANE_NEW_COMMENT, &CBCFTopicForm::OnNewComment)
	ON_BN_CLICKED(IDC_PANE_SHOW_COMMENT_DETAILS, &CBCFTopicForm::OnShowCommentDetails)
	ON_BN_CLICKED(IDC_PANE_DELETE_COMMENT, &CBCFTopicForm::OnDeleteComment)
	ON_BN_CLICKED(IDC_PANE_ADD_DOCUMENT, &CBCFTopicForm::OnAddDocument)
	ON_BN_CLICKED(IDC_PANE_REMOVE_DOCUMENT, &CBCFTopicForm::OnRemoveDocument)
	ON_CONTROL(LBN_SELCHANGE, IDC_PANE_DOCUMENTS, &CBCFTopicForm::OnDocumentChanged)
	ON_BN_CLICKED(IDC_PANE_ADD_LINK, &CBCFTopicForm::OnAddLink)
	ON_BN_CLICKED(IDC_PANE_REMOVE_LINK, &CBCFTopicForm::OnRemoveLink)
	ON_CONTROL(LBN_SELCHANGE, IDC_PANE_LINKS, &CBCFTopicForm::OnLinkChanged)
	ON_BN_CLICKED(IDC_PANE_ADD_RELATED_TOPIC, &CBCFTopicForm::OnAddRelatedTopic)
	ON_BN_CLICKED(IDC_PANE_REMOVE_RELATED_TOPIC, &CBCFTopicForm::OnRemoveRelatedTopic)
	ON_CONTROL(LBN_SELCHANGE, IDC_PANE_RELATED_TOPICS, &CBCFTopicForm::OnRelatedTopicChanged)
END_MESSAGE_MAP()

BOOL CBCFTopicForm::Create(CBCFView* pane)
{
	m_pane = pane;

	if (!CreateEx(0, RegisterBCFPaneClass(), L"", WS_CHILD | WS_CLIPCHILDREN, CRect(), pane, 0)) {
		return FALSE;
	}

	m_showBCFContent.Create(L"Back to BCF content", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
		CRect(), this, IDC_PANE_SHOW_BCF_CONTENT);
	SetBCFControlFont(m_showBCFContent, this);
	CreateBCFStaticLabel(m_topicInfo, L"Topic", this);
	m_separator.Create(L"", WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ, CRect(), this);

	m_tabs.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | TCS_TABS, CRect(), this, IDC_PANE_TABS);	
	SetBCFControlFont(m_tabs, this);
	
	m_tabs.InsertItem(0, L"Title && Comments");
	m_tabs.InsertItem(1, L"Details");
	m_tabs.InsertItem(2, L"References");

	m_title.Create(WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL, CRect(), this, IDC_PANE_TOPIC_TITLE);

	CreateBCFStaticLabel(m_descriptionLabel, L"Description:", this);
	m_description.Create(WS_CHILD | WS_TABSTOP | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN | WS_VSCROLL,
		CRect(), this, IDC_PANE_TOPIC_DESCRIPTION);

	SetBCFControlFont(m_title, this);
	SetBCFControlFont(m_description, this);

	m_bimFilesGroup.Create(L"BIM Files", WS_CHILD | BS_GROUPBOX, CRect(), this, 0);
	SetBCFControlFont(m_bimFilesGroup, this);
	m_snippetGroup.Create(L"Snippet", WS_CHILD | BS_GROUPBOX, CRect(), this, 0);
	SetBCFControlFont(m_snippetGroup, this);

	const wchar_t* labels[12] = {
		L"Type:", L"Stage:", L"Status:", L"Assigned:", L"Priority:", L"Due:",
		L"Type:", L"File:", L"Schema:", L"Index:", L"Server Id:", L""
	};

	for (int i = 0; i < 12; ++i) {
		CreateBCFStaticLabel(m_attributeLabels[i], labels[i], this);
	}
	CreateBCFStaticLabel(m_labelsLabel, L"Labels:", this);
	m_labelsLabel.ModifyStyleEx(0, WS_EX_TRANSPARENT);

	DWORD comboStyle = WS_CHILD | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST;
	m_type.Create(comboStyle, CRect(), this, IDC_PANE_TOPIC_TYPE);
	m_stage.Create(comboStyle, CRect(), this, IDC_PANE_TOPIC_STAGE);
	m_status.Create(comboStyle, CRect(), this, IDC_PANE_TOPIC_STATUS);
	m_assigned.Create(comboStyle, CRect(), this, IDC_PANE_TOPIC_ASSIGNED);
	m_priority.Create(comboStyle, CRect(), this, IDC_PANE_TOPIC_PRIORITY);
	m_snippetType.Create(comboStyle, CRect(), this, IDC_PANE_TOPIC_SNIPPET);
	CWnd* combos[] = { &m_type, &m_stage, &m_status, &m_assigned, &m_priority, &m_snippetType };
	for (CWnd* combo : combos) {
		SetBCFControlFont(*combo, this);
	}
	m_snippetExternal.Create(L"External", WS_CHILD | BS_AUTOCHECKBOX, CRect(), this, 0);
	SetBCFControlFont(m_snippetExternal, this);
	m_snippetExternal.EnableWindow(FALSE);
	m_selectSnippetFile.Create(L"...", WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
		CRect(), this, IDC_PANE_SELECT_SNIPPET_FILE);
	SetBCFControlFont(m_selectSnippetFile, this);
	DWORD editStyle = WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL;
	m_due.Create(editStyle, CRect(), this, IDC_PANE_TOPIC_DUE);
	m_labels.Create(editStyle | ES_READONLY, CRect(), this, 0);
	m_selectTopicLabels.Create(L"...", WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
		CRect(), this, IDC_PANE_SELECT_TOPIC_LABELS);
	SetBCFControlFont(m_selectTopicLabels, this);
	m_snippetReference.Create(editStyle | ES_READONLY, CRect(), this, IDC_PANE_TOPIC_REFERENCE);
	m_snippetSchema.Create(editStyle, CRect(), this, IDC_PANE_TOPIC_SCHEMA);
	m_index.Create(editStyle, CRect(), this, IDC_PANE_TOPIC_INDEX);
	m_serverId.Create(editStyle, CRect(), this, IDC_PANE_TOPIC_SERVER_ID);
	CWnd* edits[] = {
		&m_due, &m_labels, &m_snippetReference, &m_snippetSchema, &m_index, &m_serverId
	};
	for (CWnd* edit : edits) {
		SetBCFControlFont(*edit, this);
	}
	m_bimFiles.Create(WS_CHILD | WS_BORDER | WS_TABSTOP | WS_VSCROLL | WS_HSCROLL |
		LBS_OWNERDRAWFIXED | LBS_HASSTRINGS | LBS_NOINTEGRALHEIGHT,
		CRect(), this, IDC_PANE_BIM_FILES);
	m_bimFiles.SetCheckStyle(BS_AUTOCHECKBOX);
	SetBCFControlFont(m_bimFiles, this);
	m_addBimFiles.Create(L"Add...", WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
		CRect(), this, IDC_PANE_ADD_BIM_FILES);
	SetBCFControlFont(m_addBimFiles, this);
	m_comments.Create(WS_CHILD | WS_BORDER | WS_TABSTOP | WS_VSCROLL | LBS_NOTIFY | LBS_OWNERDRAWVARIABLE |
		LBS_HASSTRINGS | LBS_NOINTEGRALHEIGHT, CRect(), this, IDC_PANE_COMMENTS);
	SetBCFControlFont(m_comments, this);
	m_newComment.Create(L"New comment...", WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
		CRect(), this, IDC_PANE_NEW_COMMENT);
	m_showCommentDetails.Create(L"Show details", WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
		CRect(), this, IDC_PANE_SHOW_COMMENT_DETAILS);
	m_deleteComment.Create(L"Delete comment...", WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
		CRect(), this, IDC_PANE_DELETE_COMMENT);
	SetBCFControlFont(m_newComment, this);
	SetBCFControlFont(m_showCommentDetails, this);
	SetBCFControlFont(m_deleteComment, this);
	CreateBCFStaticLabel(m_documentsLabel, L"Documents", this);
	m_documents.Create(WS_CHILD | WS_BORDER | WS_TABSTOP | WS_VSCROLL | WS_HSCROLL |
		LBS_NOTIFY | LBS_SORT | LBS_NOINTEGRALHEIGHT,
		CRect(), this, IDC_PANE_DOCUMENTS);
	SetBCFControlFont(m_documents, this);
	m_addDocument.Create(L"Add..", WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
		CRect(), this, IDC_PANE_ADD_DOCUMENT);
	SetBCFControlFont(m_addDocument, this);
	m_removeDocument.Create(L"Remove...", WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
		CRect(), this, IDC_PANE_REMOVE_DOCUMENT);
	SetBCFControlFont(m_removeDocument, this);
	CreateBCFStaticLabel(m_linksLabel, L"Links", this);
	m_links.Create(WS_CHILD | WS_BORDER | WS_TABSTOP | WS_VSCROLL | WS_HSCROLL |
		LBS_NOTIFY | LBS_SORT | LBS_NOINTEGRALHEIGHT,
		CRect(), this, IDC_PANE_LINKS);
	SetBCFControlFont(m_links, this);
	m_addLink.Create(L"Add..", WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
		CRect(), this, IDC_PANE_ADD_LINK);
	SetBCFControlFont(m_addLink, this);
	m_removeLink.Create(L"Remove...", WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
		CRect(), this, IDC_PANE_REMOVE_LINK);
	SetBCFControlFont(m_removeLink, this);
	CreateBCFStaticLabel(m_relatedTopicsLabel, L"Related Topics", this);
	m_relatedTopics.Create(WS_CHILD | WS_BORDER | WS_TABSTOP | WS_VSCROLL | WS_HSCROLL |
		LBS_NOTIFY | LBS_SORT | LBS_NOINTEGRALHEIGHT,
		CRect(), this, IDC_PANE_RELATED_TOPICS);
	SetBCFControlFont(m_relatedTopics, this);
	m_addRelatedTopic.Create(L"Add..", WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
		CRect(), this, IDC_PANE_ADD_RELATED_TOPIC);
	SetBCFControlFont(m_addRelatedTopic, this);
	m_removeRelatedTopic.Create(L"Remove...", WS_CHILD | WS_TABSTOP | BS_PUSHBUTTON,
		CRect(), this, IDC_PANE_REMOVE_RELATED_TOPIC);
	SetBCFControlFont(m_removeRelatedTopic, this);

	CStatic* tabLabels[] = {
		&m_descriptionLabel, &m_documentsLabel, &m_linksLabel, &m_relatedTopicsLabel
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

BOOL CBCFTopicForm::OnCommand(WPARAM wParam, LPARAM lParam)
{
	const UINT id = LOWORD(wParam);
	const UINT notification = HIWORD(wParam);
	CWnd* control = GetDlgItem(id);
	const bool commitEdit = notification == EN_KILLFOCUS &&
		IsTopicEditControl(id) && control &&
		control->SendMessage(EM_GETMODIFY) != 0;
	const bool commitCombo = notification == CBN_SELCHANGE &&
		IsTopicComboControl(id);

	const BOOL handled = CWnd::OnCommand(wParam, lParam);
	if (commitEdit || commitCombo) {
		Commit();
	}
	m_pane->UpdateSaveButton(notification == BN_CLICKED);
	return handled;
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
	SetBCFComboValue(m_type, FromUTF8(topic->GetTopicType()));
	SetBCFComboValue(m_stage, FromUTF8(topic->GetStage()));
	SetBCFComboValue(m_status, FromUTF8(topic->GetTopicStatus()));
	SetBCFComboValue(m_assigned, FromUTF8(topic->GetAssignedTo()));
	SetBCFComboValue(m_priority, FromUTF8(topic->GetPriority()));
	m_due.SetWindowText(FromUTF8(topic->GetDueDate()));
	m_index.SetWindowText(FromUTF8(topic->GetIndexStr()));
	m_serverId.SetWindowText(FromUTF8(topic->GetServerAssignedId()));
	UpdateLabels();
	BCFBimSnippet* snippet = topic->GetBimSnippet(false);
	SetBCFComboValue(m_snippetType, snippet ? FromUTF8(snippet->GetSnippetType()) : CString());
	m_snippetSchema.SetWindowText(snippet ? FromUTF8(snippet->GetReferenceSchema()) : CString());
	m_snippetExternal.SetCheck(snippet && snippet->GetIsExternal() ? BST_CHECKED : BST_UNCHECKED);
	CString reference = snippet ? FromUTF8(snippet->GetReference()) : CString();
	if (snippet && !snippet->GetIsExternal()) {
		fs::path path(static_cast<LPCWSTR>(reference));
		reference = path.filename().wstring().c_str();
	}
	m_snippetReference.SetWindowText(reference);
	CEdit* edits[] = {
		&m_title, &m_description, &m_due, &m_snippetSchema, &m_index, &m_serverId
	};
	for (CEdit* edit : edits) {
		edit->SetModify(FALSE);
	}
	FormatTopicInfo();
	
	ReloadDocuments();
	ReloadLinks();
	ReloadRelatedTopics();
	m_pane->LoadBimFiles(*topic);
	ReloadBimFiles();
	m_tabs.SetCurSel(0);
	ShowTab(0);
	ReloadComments();
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
	if (ok) {
		CEdit* edits[] = {
			&m_title, &m_description, &m_due, &m_snippetSchema, &m_index, &m_serverId
		};
		for (CEdit* edit : edits) {
			edit->SetModify(FALSE);
		}
	}
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
	UpdateCommentButtons();
}

void CBCFTopicForm::OnCommentDoubleClick()
{
	OnShowCommentDetails();
}

void CBCFTopicForm::OnNewComment()
{
	if (!m_topic) {
		return;
	}
	BCFComment* comment = m_topic->AddComment();
	m_pane->ShowLog(!comment);
	if (!comment) {
		return;
	}
	if (m_pane->GetDocument()) {
		const bool ok = CBCFViewPointMgr(*m_pane->GetDocument())
			.SaveCurrentViewToComent(*comment);
		m_pane->ShowLog(!ok);
	}
	ReloadComments(comment);
	m_pane->ShowComment(comment);
}

void CBCFTopicForm::OnShowCommentDetails()
{
	m_pane->ShowComment(GetSelectedComment());
}

void CBCFTopicForm::OnDeleteComment()
{
	BCFComment* comment = GetSelectedComment();
	if (!comment ||
		AfxMessageBox(L"Delete this comment?", MB_YESNO | MB_ICONWARNING) != IDYES) {
		return;
	}
	const bool ok = comment->Remove();
	m_pane->ShowLog(!ok);
	if (ok) {
		ReloadComments();
	}
}

void CBCFTopicForm::UpdateCommentButtons()
{
	const bool hasTopic = m_topic != nullptr;
	const bool hasComment = GetSelectedComment() != nullptr;
	m_newComment.EnableWindow(hasTopic);
	m_showCommentDetails.EnableWindow(hasComment);
	m_deleteComment.EnableWindow(hasComment);
}

void CBCFTopicForm::OnTabChanged(NMHDR*, LRESULT* result)
{
	ShowTab(m_tabs.GetCurSel());
	*result = 0;
}

void CBCFTopicForm::FocusInitialControl(bool selectTitle)
{
	if (m_tabs.GetCurSel() != 0) {
		m_tabs.SetCurSel(0);
		ShowTab(0);
	}
	m_title.SetFocus();
	if (selectTitle) {
		m_title.SetSel(0, -1);
	}
}

void CBCFTopicForm::OnShowBCFContent()
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

void CBCFTopicForm::ReloadBimFiles()
{
	m_usedBimModels.clear();
	if (m_topic) {
		for (uint16_t i = 0; BCFBimFile* file = m_topic->GetBimFile(i); ++i) {
			if (_model* model = m_pane->GetBimModel(*file)) {
				m_usedBimModels[model] = file;
			}
		}
	}

	m_bimFiles.SetRedraw(FALSE);
	const int selection = m_bimFiles.GetCurSel();
	const int topIndex = m_bimFiles.GetTopIndex();
	m_bimFiles.ResetContent();
	if (m_pane->GetDocument()) {
		for (_model* model : m_pane->GetDocument()->getModels()) {
			if (model) {
				auto used = m_usedBimModels.find(model);
				const CString text = used == m_usedBimModels.end()
					? model->getPath()
					: GetBimFileText(*used->second);
				const int item = m_bimFiles.AddString(text);
				m_bimFiles.SetItemDataPtr(item, model);
				m_bimFiles.SetCheck(item, used != m_usedBimModels.end() ? BST_CHECKED : BST_UNCHECKED);
			}
		}
	}
	if (selection != LB_ERR && selection < m_bimFiles.GetCount()) {
		m_bimFiles.SetCurSel(selection);
	}
	if (topIndex != LB_ERR && topIndex < m_bimFiles.GetCount()) {
		m_bimFiles.SetTopIndex(topIndex);
	}
	UpdateHorizontalExtent(m_bimFiles, ::GetSystemMetrics(SM_CXMENUCHECK) + 4);
	m_bimFiles.SetRedraw(TRUE);
	m_bimFiles.Invalidate();
}

bool CBCFTopicForm::AddBimFile(const CString& path, bool external)
{
	if (!m_topic || !m_topic->AddBimFile(ToUTF8(path).c_str(), external)) {
		m_pane->ShowLog(true);
		return false;
	}
	return true;
}

void CBCFTopicForm::OnAddBimFiles()
{
	if (!m_topic) {
		return;
	}
	CBCFSelectFileDlg dialog(L"", false, this, BIM_MODELS_FILTER,
		OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_ALLOWMULTISELECT);
	if (dialog.DoModal() != IDOK) {
		return;
	}

	bool ok = true;
	for (POSITION position = dialog.GetStartPosition(); position;) {
		CString path = dialog.GetNextPathName(position);
		ok = AddBimFile(path, dialog.IsExternal()) && ok;
		AfxGetApp()->AddToRecentFileList(path);
	}
	m_pane->LoadBimFiles(*m_topic);
	ReloadBimFiles();
	m_pane->ShowLog(!ok);
}

void CBCFTopicForm::OnCheckBimFiles()
{
	if (!m_topic) {
		return;
	}
	for (int i = 0; i < m_bimFiles.GetCount(); ++i) {
		_model* model = static_cast<_model*>(m_bimFiles.GetItemDataPtr(i));
		auto found = m_usedBimModels.find(model);
		if (m_bimFiles.GetCheck(i) == BST_CHECKED && found == m_usedBimModels.end()) {
			AddBimFile(model->getPath(), false);
			ReloadBimFiles();
			return;
		}
		if (m_bimFiles.GetCheck(i) == BST_UNCHECKED && found != m_usedBimModels.end()) {
			if (!found->second->Remove()) {
				m_pane->ShowLog(true);
			}
			ReloadBimFiles();
			return;
		}
	}
}

BCFDocumentReference* CBCFTopicForm::GetSelectedDocument() const
{
	const int selection = m_documents.GetCurSel();
	return selection == LB_ERR
		? nullptr
		: static_cast<BCFDocumentReference*>(m_documents.GetItemDataPtr(selection));
}

void CBCFTopicForm::ReloadDocuments(BCFDocumentReference* selectDocument)
{
	BCFDocumentReference* selected = selectDocument ? selectDocument : GetSelectedDocument();
	m_documents.SetRedraw(FALSE);
	m_documents.ResetContent();
	int selectedItem = LB_ERR;
	if (m_topic) {
		for (uint16_t i = 0; BCFDocumentReference* document = m_topic->GetDocumentReference(i); ++i) {
			const int item = m_documents.AddString(GetDocumentText(*document));
			m_documents.SetItemDataPtr(item, document);
			if (document == selected) {
				selectedItem = item;
			}
		}
	}
	UpdateHorizontalExtent(m_documents);
	m_documents.SetRedraw(TRUE);
	m_documents.Invalidate();
	if (selectedItem == LB_ERR && m_documents.GetCount() > 0) {
		selectedItem = 0;
	}
	m_documents.SetCurSel(selectedItem);
	OnDocumentChanged();
}

void CBCFTopicForm::OnAddDocument()
{
	if (!m_topic) {
		return;
	}
	CBCFAddDocumentReference dialog(*m_pane, *m_topic);
	if (dialog.DoModal() == IDOK) {
		ReloadDocuments(m_topic->GetDocumentReference(
			static_cast<uint16_t>(m_documents.GetCount())));
	}
}

void CBCFTopicForm::OnRemoveDocument()
{
	BCFDocumentReference* document = GetSelectedDocument();
	if (!document) {
		return;
	}
	CString question;
	question.Format(L"Do you want to remove reference to document '%s'?",
		GetDocumentText(*document).GetString());
	if (AfxMessageBox(question, MB_YESNO) == IDYES) {
		const bool ok = document->Remove();
		m_pane->ShowLog(!ok);
		if (ok) {
			ReloadDocuments();
		}
	}
}

void CBCFTopicForm::OnDocumentChanged()
{
	m_removeDocument.EnableWindow(GetSelectedDocument() != nullptr);
}

void CBCFTopicForm::ReloadLinks(int selection)
{
	if (selection == LB_ERR) {
		selection = m_links.GetCurSel();
	}
	m_links.SetRedraw(FALSE);
	m_links.ResetContent();
	if (m_topic) {
		for (uint16_t i = 0; const char* link = m_topic->GetReferenceLink(i); ++i) {
			m_links.AddString(FromUTF8(link));
		}
	}
	UpdateHorizontalExtent(m_links);
	m_links.SetRedraw(TRUE);
	m_links.Invalidate();
	if (selection == LB_ERR && m_links.GetCount() > 0) {
		selection = 0;
	}
	if (selection >= m_links.GetCount()) {
		selection = m_links.GetCount() - 1;
	}
	m_links.SetCurSel(selection);
	OnLinkChanged();
}

void CBCFTopicForm::OnAddLink()
{
	if (!m_topic) {
		return;
	}
	CBCFAddReferenceLink dialog(*m_pane, *m_topic);
	if (dialog.DoModal() == IDOK) {
		ReloadLinks();
	}
}

void CBCFTopicForm::OnRemoveLink()
{
	const int selection = m_links.GetCurSel();
	if (!m_topic || selection == LB_ERR) {
		return;
	}
	CString link;
	m_links.GetText(selection, link);
	CString question;
	question.Format(L"Do you want to delete reference link '%s'?", link.GetString());
	if (AfxMessageBox(question, MB_YESNO) == IDYES) {
		const bool ok = m_topic->RemoveReferenceLink(ToUTF8(link).c_str());
		m_pane->ShowLog(!ok);
		if (ok) {
			ReloadLinks(selection);
		}
	}
}

void CBCFTopicForm::OnLinkChanged()
{
	m_removeLink.EnableWindow(m_links.GetCurSel() != LB_ERR);
}

BCFTopic* CBCFTopicForm::GetSelectedRelatedTopic() const
{
	const int selection = m_relatedTopics.GetCurSel();
	return selection == LB_ERR
		? nullptr
		: static_cast<BCFTopic*>(m_relatedTopics.GetItemDataPtr(selection));
}

void CBCFTopicForm::ReloadRelatedTopics(BCFTopic* selectTopic)
{
	BCFTopic* selected = selectTopic ? selectTopic : GetSelectedRelatedTopic();
	m_relatedTopics.SetRedraw(FALSE);
	m_relatedTopics.ResetContent();
	int selectedItem = LB_ERR;
	if (m_topic) {
		for (uint16_t i = 0; BCFTopic* topic = m_topic->GetRelatedTopic(i); ++i) {
			const int item = m_relatedTopics.AddString(GetBCFTopicDisplayName(*topic));
			m_relatedTopics.SetItemDataPtr(item, topic);
			if (topic == selected) {
				selectedItem = item;
			}
		}
	}
	UpdateHorizontalExtent(m_relatedTopics);
	m_relatedTopics.SetRedraw(TRUE);
	m_relatedTopics.Invalidate();
	if (selectedItem == LB_ERR && m_relatedTopics.GetCount() > 0) {
		selectedItem = 0;
	}
	m_relatedTopics.SetCurSel(selectedItem);
	OnRelatedTopicChanged();
}

void CBCFTopicForm::OnAddRelatedTopic()
{
	if (!m_topic) {
		return;
	}
	CBCFAddRelatedTopic dialog(*m_pane, *m_topic);
	if (dialog.DoModal() == IDOK) {
		ReloadRelatedTopics();
	}
}

void CBCFTopicForm::OnRemoveRelatedTopic()
{
	BCFTopic* relatedTopic = GetSelectedRelatedTopic();
	if (!m_topic || !relatedTopic) {
		return;
	}
	const bool ok = m_topic->RemoveRelatedTopic(relatedTopic);
	m_pane->ShowLog(!ok);
	if (ok) {
		ReloadRelatedTopics();
	}
}

void CBCFTopicForm::OnRelatedTopicChanged()
{
	m_removeRelatedTopic.EnableWindow(GetSelectedRelatedTopic() != nullptr);
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
	CWnd* attributeControls[] = {
		&m_type, &m_stage, &m_status, &m_assigned, &m_priority, &m_due,
		&m_index, &m_serverId, &m_selectTopicLabels
	};
	for (int i = 0; i < 12; ++i) {
		m_attributeLabels[i].ShowWindow(tab == 1 ? SW_SHOW : SW_HIDE);
	}
	for (CWnd* control : attributeControls) {
		control->ShowWindow(tab == 1 ? SW_SHOW : SW_HIDE);
	}
	m_labelsLabel.ShowWindow(tab == 1 ? SW_SHOW : SW_HIDE);
	m_labels.ShowWindow(tab == 1 ? SW_SHOW : SW_HIDE);
	CWnd* snippetControls[] = {
		&m_snippetType, &m_snippetReference, &m_snippetExternal,
		&m_snippetSchema, &m_selectSnippetFile
	};
	for (CWnd* control : snippetControls) {
		control->ShowWindow(tab == 1 ? SW_SHOW : SW_HIDE);
	}
	m_snippetGroup.ShowWindow(tab == 1 ? SW_SHOW : SW_HIDE);
	m_bimFilesGroup.ShowWindow(tab == 1 ? SW_SHOW : SW_HIDE);
	m_bimFiles.ShowWindow(tab == 1 ? SW_SHOW : SW_HIDE);
	m_addBimFiles.ShowWindow(tab == 1 ? SW_SHOW : SW_HIDE);
	m_documents.ShowWindow(tab == 2 ? SW_SHOW : SW_HIDE);
	m_documentsLabel.ShowWindow(tab == 2 ? SW_SHOW : SW_HIDE);
	m_addDocument.ShowWindow(tab == 2 ? SW_SHOW : SW_HIDE);
	m_removeDocument.ShowWindow(tab == 2 ? SW_SHOW : SW_HIDE);
	m_linksLabel.ShowWindow(tab == 2 ? SW_SHOW : SW_HIDE);
	m_links.ShowWindow(tab == 2 ? SW_SHOW : SW_HIDE);
	m_addLink.ShowWindow(tab == 2 ? SW_SHOW : SW_HIDE);
	m_removeLink.ShowWindow(tab == 2 ? SW_SHOW : SW_HIDE);
	m_relatedTopicsLabel.ShowWindow(tab == 2 ? SW_SHOW : SW_HIDE);
	m_relatedTopics.ShowWindow(tab == 2 ? SW_SHOW : SW_HIDE);
	m_addRelatedTopic.ShowWindow(tab == 2 ? SW_SHOW : SW_HIDE);
	m_removeRelatedTopic.ShowWindow(tab == 2 ? SW_SHOW : SW_HIDE);
	m_comments.ShowWindow(tab == 0 ? SW_SHOW : SW_HIDE);
	m_newComment.ShowWindow(tab == 0 ? SW_SHOW : SW_HIDE);
	m_showCommentDetails.ShowWindow(tab == 0 ? SW_SHOW : SW_HIDE);
	m_deleteComment.ShowWindow(tab == 0 ? SW_SHOW : SW_HIDE);
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
	const int rowSpacing = margin;
	const int labelOffset = (rowHeight - textHeight) / 2;
	const int separatorHeight = max(static_cast<int>(textMetrics.tmInternalLeading), rowHeight / 8);
	const int separatorTop = rowHeight + margin / 2;
	const int tabsTop = separatorTop + separatorHeight + margin / 2;
	const int tabsWidth = max(rowHeight, client.Width() - 2 * margin);
	const int tabsHeight = max(2 * rowHeight, client.Height() - tabsTop - margin);

	CString buttonText;
	m_showBCFContent.GetWindowText(buttonText);
	const int buttonWidth = static_cast<int>(dc.GetTextExtent(buttonText).cx) + 2 * margin;
	m_showBCFContent.MoveWindow(
		max(margin, static_cast<int>(client.right) - buttonWidth - margin),
		0, buttonWidth, rowHeight);
	m_topicInfo.MoveWindow(margin, labelOffset,
		max(rowHeight, client.Width() - buttonWidth - 3 * margin), textHeight);
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
		const int titleWidth = max(rowHeight, (page.Width() - rowSpacing) / 2);
		const int commentsLeft = page.left + titleWidth + rowSpacing;
		const int commentsWidth = max(rowHeight,
			static_cast<int>(page.right) - commentsLeft);
		m_title.MoveWindow(page.left, page.top + labelOffset,
			titleWidth, rowHeight - labelOffset);

		const int descriptionLabelTop = page.top + rowHeight + rowSpacing;
		m_descriptionLabel.MoveWindow(page.left, descriptionLabelTop,
			getLabelWidth(m_descriptionLabel), textHeight);
		const int descriptionTop = descriptionLabelTop + textHeight + rowSpacing;
		m_description.MoveWindow(page.left, descriptionTop, titleWidth,
			max(rowHeight, static_cast<int>(page.bottom) - descriptionTop));
		CString newCommentText;
		CString detailsText;
		CString deleteCommentText;
		m_newComment.GetWindowText(newCommentText);
		m_showCommentDetails.GetWindowText(detailsText);
		m_deleteComment.GetWindowText(deleteCommentText);
		const int newCommentWidth =
			static_cast<int>(dc.GetTextExtent(newCommentText).cx) + 2 * margin;
		const int detailsWidth =
			static_cast<int>(dc.GetTextExtent(detailsText).cx) + 2 * margin;
		const int deleteCommentWidth =
			static_cast<int>(dc.GetTextExtent(deleteCommentText).cx) + 2 * margin;
		const int commentButtonsTop = page.bottom - rowHeight;
		m_comments.MoveWindow(commentsLeft, page.top, commentsWidth,
			max(rowHeight, static_cast<int>(commentButtonsTop - page.top - rowSpacing)));
		int commentButtonLeft = page.right -
			newCommentWidth - detailsWidth - deleteCommentWidth - 2 * rowSpacing;
		m_newComment.MoveWindow(
			commentButtonLeft, commentButtonsTop, newCommentWidth, rowHeight);
		commentButtonLeft += newCommentWidth + rowSpacing;
		m_showCommentDetails.MoveWindow(
			commentButtonLeft, commentButtonsTop, detailsWidth, rowHeight);
		commentButtonLeft += detailsWidth + rowSpacing;
		m_deleteComment.MoveWindow(
			commentButtonLeft, commentButtonsTop, deleteCommentWidth, rowHeight);
	}
	else if (m_tabs.GetCurSel() == 1) {
		const int columnWidth = max(rowHeight, (page.Width() - 2 * rowSpacing) / 3);
		const int leftColumn = page.left;
		const int middleColumn = leftColumn + columnWidth + rowSpacing;
		const int rightColumn = middleColumn + columnWidth + rowSpacing;
		const int rightColumnWidth = max(rowHeight,
			static_cast<int>(page.right) - rightColumn);

		CStatic* leftLabels[] = {
			&m_attributeLabels[0], &m_attributeLabels[1], &m_attributeLabels[2],
			&m_attributeLabels[3], &m_attributeLabels[4], &m_attributeLabels[5]
		};
		CWnd* leftControls[] = {
			&m_type, &m_stage, &m_status, &m_assigned, &m_priority, &m_due
		};
		int leftLabelWidth = 0;
		for (CStatic* label : leftLabels) {
			leftLabelWidth = max(leftLabelWidth, getLabelWidth(*label));
		}
		leftLabelWidth += margin;
		for (size_t i = 0; i < _countof(leftControls); ++i) {
			const int top = page.top +
				static_cast<int>(i) * (rowHeight + rowSpacing);
			leftLabels[i]->MoveWindow(
				leftColumn, top + labelOffset, leftLabelWidth, textHeight);
			const bool isEdit = leftControls[i]->IsKindOf(RUNTIME_CLASS(CEdit)) != FALSE;
			leftControls[i]->MoveWindow(
				leftColumn + leftLabelWidth, top + labelOffset,
				max(rowHeight, columnWidth - leftLabelWidth),
				isEdit ? rowHeight - labelOffset : 3 * rowHeight);
		}

		const int middleLabelWidth = max(
			getLabelWidth(m_labelsLabel),
			max(getLabelWidth(m_attributeLabels[9]),
				getLabelWidth(m_attributeLabels[10]))) + margin;
		m_labelsLabel.MoveWindow(
			middleColumn, page.top + labelOffset, middleLabelWidth, textHeight);
		m_selectTopicLabels.MoveWindow(
			middleColumn + columnWidth - rowHeight, page.top, rowHeight, rowHeight);
		m_labels.MoveWindow(
			middleColumn + middleLabelWidth, page.top + labelOffset,
			max(rowHeight, columnWidth - middleLabelWidth - rowHeight - margin / 2),
			rowHeight - labelOffset);

		const int snippetTop = page.top + rowHeight + rowSpacing;
		const int snippetHeight = 4 * rowHeight + 2 * rowSpacing + margin;
		m_snippetGroup.MoveWindow(
			middleColumn, snippetTop, columnWidth, snippetHeight);
		const int snippetContentLeft = middleColumn + margin;
		const int snippetContentRight = middleColumn + columnWidth - margin;
		const int snippetContentTop = snippetTop + rowHeight;
		const int labelIndices[] = { 7, 6, 8 };
		int snippetLabelWidth = 0;
		for (int labelIndex : labelIndices) {
			snippetLabelWidth = max(
				snippetLabelWidth, getLabelWidth(m_attributeLabels[labelIndex]));
		}
		snippetLabelWidth += margin;
		const int snippetControlLeft = snippetContentLeft + snippetLabelWidth;

		const int fileTop = snippetContentTop;
		m_attributeLabels[7].MoveWindow(
			snippetContentLeft, fileTop + labelOffset,
			snippetLabelWidth, textHeight);
		m_selectSnippetFile.MoveWindow(
			snippetContentRight - rowHeight, fileTop, rowHeight, rowHeight);
		m_snippetReference.MoveWindow(
			snippetControlLeft, fileTop + labelOffset,
			max(rowHeight, snippetContentRight -
				snippetControlLeft - rowHeight - margin / 2),
			rowHeight - labelOffset);

		const int typeTop = fileTop + rowHeight + rowSpacing;
		m_attributeLabels[6].MoveWindow(
			snippetContentLeft, typeTop + labelOffset,
			snippetLabelWidth, textHeight);
		CString externalText;
		m_snippetExternal.GetWindowText(externalText);
		const int externalWidth = ::GetSystemMetrics(SM_CXMENUCHECK) +
			static_cast<int>(dc.GetTextExtent(externalText).cx) + margin;
		int typeWidth = 0;
		CString typeText;
		m_snippetType.GetWindowText(typeText);
		typeWidth = static_cast<int>(dc.GetTextExtent(typeText).cx);
		for (int i = 0; i < m_snippetType.GetCount(); ++i) {
			m_snippetType.GetLBText(i, typeText);
			typeWidth = max(typeWidth, static_cast<int>(dc.GetTextExtent(typeText).cx));
		}
		typeWidth += ::GetSystemMetrics(SM_CXVSCROLL) + 2 * margin;
		typeWidth = min(typeWidth,
			max(rowHeight, snippetContentRight -
				snippetControlLeft - externalWidth - rowSpacing));
		m_snippetType.MoveWindow(
			snippetControlLeft, typeTop + labelOffset, typeWidth, 3 * rowHeight);
		m_snippetExternal.MoveWindow(
			snippetControlLeft + typeWidth + rowSpacing,
			typeTop, externalWidth, rowHeight);

		const int schemaTop = typeTop + rowHeight + rowSpacing;
		m_attributeLabels[8].MoveWindow(
			snippetContentLeft, schemaTop + labelOffset,
			snippetLabelWidth, textHeight);
		m_snippetSchema.MoveWindow(
			snippetControlLeft, schemaTop + labelOffset,
			max(rowHeight, snippetContentRight - snippetControlLeft),
			rowHeight - labelOffset);

		const int indexTop = snippetTop + snippetHeight + rowSpacing;
		CStatic* middleLabels[] = {
			&m_attributeLabels[9], &m_attributeLabels[10]
		};
		CBCFEdit* middleControls[] = { &m_index, &m_serverId };
		for (int i = 0; i < 2; ++i) {
			const int top = indexTop + i * (rowHeight + rowSpacing);
			middleLabels[i]->MoveWindow(
				middleColumn, top + labelOffset, middleLabelWidth, textHeight);
			middleControls[i]->MoveWindow(
				middleColumn + middleLabelWidth, top + labelOffset,
				max(rowHeight, columnWidth - middleLabelWidth),
				rowHeight - labelOffset);
		}

		m_bimFilesGroup.MoveWindow(
			rightColumn, page.top, rightColumnWidth, page.Height());
		const int addButtonWidth = max(
			rowHeight, static_cast<int>(dc.GetTextExtent(L"Add...").cx) + 2 * margin);
		const int bimFilesContentLeft = rightColumn + margin;
		const int bimFilesContentTop = page.top + rowHeight;
		m_addBimFiles.MoveWindow(
			bimFilesContentLeft, page.bottom - rowHeight - margin,
			addButtonWidth, rowHeight);
		m_bimFiles.MoveWindow(
			bimFilesContentLeft, bimFilesContentTop,
			max(rowHeight, rightColumnWidth - 2 * margin),
			max(rowHeight, static_cast<int>(page.bottom) -
				bimFilesContentTop - rowHeight - 2 * margin));
	}
	else if (m_tabs.GetCurSel() == 2) {
		const int columnWidth = max(rowHeight, (page.Width() - 2 * rowSpacing) / 3);
		CStatic* labels[] = { &m_documentsLabel, &m_linksLabel, &m_relatedTopicsLabel };
		CListBox* lists[] = { &m_documents, &m_links, &m_relatedTopics };
		CButton* addButtons[] = { &m_addDocument, &m_addLink, &m_addRelatedTopic };
		CButton* removeButtons[] = { &m_removeDocument, &m_removeLink, &m_removeRelatedTopic };
		for (int i = 0; i < 3; ++i) {
			const int left = page.left + i * (columnWidth + rowSpacing);
			const int width = i < 2
				? columnWidth
				: max(rowHeight, static_cast<int>(page.right) - left);
			labels[i]->MoveWindow(left, page.top + labelOffset, width, textHeight);

			CString addText;
			CString removeText;
			addButtons[i]->GetWindowText(addText);
			removeButtons[i]->GetWindowText(removeText);
			const int addWidth = max(rowHeight,
				static_cast<int>(dc.GetTextExtent(addText).cx) + 2 * margin);
			const int removeWidth = max(rowHeight,
				static_cast<int>(dc.GetTextExtent(removeText).cx) + 2 * margin);
			const int removeLeft = left + width - removeWidth;
			addButtons[i]->MoveWindow(
				removeLeft - rowSpacing - addWidth, page.bottom - rowHeight,
				addWidth, rowHeight);
			removeButtons[i]->MoveWindow(
				removeLeft, page.bottom - rowHeight, removeWidth, rowHeight);
			const int listTop = page.top + rowHeight;
			lists[i]->MoveWindow(left, listTop, width,
				max(rowHeight, static_cast<int>(page.bottom) -
					listTop - rowHeight - rowSpacing));
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
