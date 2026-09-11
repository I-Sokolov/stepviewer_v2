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
}

BEGIN_MESSAGE_MAP(CBCFTopicForm, CWnd)
	ON_WM_SIZE()
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_PANE_VIEW_PROJECT, &CBCFTopicForm::OnViewProject)
	ON_BN_CLICKED(IDC_PANE_SELECT_SNIPPET_FILE, &CBCFTopicForm::OnSelectSnippetFile)
	ON_BN_CLICKED(IDC_PANE_SELECT_TOPIC_LABELS, &CBCFTopicForm::OnSelectTopicLabels)
	ON_BN_CLICKED(IDC_PANE_ADD_BIM_FILES, &CBCFTopicForm::OnAddBimFiles)
	ON_CONTROL(CLBN_CHKCHANGE, IDC_PANE_BIM_FILES, &CBCFTopicForm::OnCheckBimFiles)
	ON_NOTIFY(TCN_SELCHANGE, IDC_PANE_TABS, &CBCFTopicForm::OnTabChanged)
	ON_CONTROL(LBN_SELCHANGE, IDC_PANE_COMMENTS, &CBCFTopicForm::OnCommentChanged)
	ON_CONTROL(LBN_DBLCLK, IDC_PANE_COMMENTS, &CBCFTopicForm::OnCommentDoubleClick)
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

	m_viewProject.Create(L"<<", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
		CRect(), this, IDC_PANE_VIEW_PROJECT);
	SetBCFControlFont(m_viewProject, this);
	CreateBCFStaticLabel(m_topicInfo, L"Topic", this);
	m_separator.Create(L"", WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ, CRect(), this);

	m_tabs.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | TCS_TABS, CRect(), this, IDC_PANE_TABS);	
	SetBCFControlFont(m_tabs, this);
	
	m_tabs.InsertItem(0, L"Title && Comment");
	m_tabs.InsertItem(1, L"Attributes");
	m_tabs.InsertItem(2, L"BIM Files");
	m_tabs.InsertItem(3, L"Snippet");
	m_tabs.InsertItem(4, L"References");

	m_title.Create(WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL, CRect(), this, IDC_PANE_TOPIC_TITLE);

	CreateBCFStaticLabel(m_descriptionLabel, L"Description:", this);
	m_description.Create(WS_CHILD | WS_TABSTOP | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN | WS_VSCROLL,
		CRect(), this, IDC_PANE_TOPIC_DESCRIPTION);

	SetBCFControlFont(m_title, this);
	SetBCFControlFont(m_description, this);

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
	m_bimFiles.Create(WS_CHILD | WS_BORDER | WS_TABSTOP | WS_VSCROLL |
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
	CreateBCFStaticLabel(m_documentsLabel, L"Documents", this);
	m_documents.Create(WS_CHILD | WS_BORDER | WS_TABSTOP | WS_VSCROLL |
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
	m_links.Create(WS_CHILD | WS_BORDER | WS_TABSTOP | WS_VSCROLL |
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
	m_relatedTopics.Create(WS_CHILD | WS_BORDER | WS_TABSTOP | WS_VSCROLL |
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
	FormatTopicInfo();
	
	ReloadComments();
	ReloadDocuments();
	ReloadLinks();
	ReloadRelatedTopics();
	m_pane->LoadBimFiles(*topic);
	ReloadBimFiles();
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
		m_comments.AddAction(L"<< Add comment >>");
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
	if (selection == LB_ERR || !m_topic) {
		return;
	}
	BCFComment* comment = static_cast<BCFComment*>(m_comments.GetItemDataPtr(selection));
	if (!comment) {
		comment = m_topic->AddComment();
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
	}
	m_pane->ShowComment(comment);
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
				const int item = m_bimFiles.AddString(model->getPath());
				m_bimFiles.SetItemDataPtr(item, model);
				m_bimFiles.SetCheck(item, m_usedBimModels.count(model) ? BST_CHECKED : BST_UNCHECKED);
			}
		}
	}
	if (selection != LB_ERR && selection < m_bimFiles.GetCount()) {
		m_bimFiles.SetCurSel(selection);
	}
	if (topIndex != LB_ERR && topIndex < m_bimFiles.GetCount()) {
		m_bimFiles.SetTopIndex(topIndex);
	}
	m_bimFiles.SetRedraw(TRUE);
	m_bimFiles.Invalidate();
}

bool CBCFTopicForm::AddBimFile(const CString& path)
{
	if (!m_topic || !m_topic->AddBimFile(ToUTF8(path).c_str(), false)) {
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
	CFileDialog dialog(TRUE, nullptr, L"",
		OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_ALLOWMULTISELECT, BIM_MODELS_FILTER);
	if (dialog.DoModal() != IDOK) {
		return;
	}

	bool ok = true;
	for (POSITION position = dialog.GetStartPosition(); position;) {
		CString path = dialog.GetNextPathName(position);
		ok = AddBimFile(path) && ok;
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
			AddBimFile(model->getPath());
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
		const bool snippetLabel = i >= 6 && i <= 8;
		m_attributeLabels[i].ShowWindow(
			(snippetLabel ? tab == 3 : tab == 1) ? SW_SHOW : SW_HIDE);
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
		control->ShowWindow(tab == 3 ? SW_SHOW : SW_HIDE);
	}
	m_bimFiles.ShowWindow(tab == 2 ? SW_SHOW : SW_HIDE);
	m_addBimFiles.ShowWindow(tab == 2 ? SW_SHOW : SW_HIDE);
	m_documents.ShowWindow(tab == 4 ? SW_SHOW : SW_HIDE);
	m_documentsLabel.ShowWindow(tab == 4 ? SW_SHOW : SW_HIDE);
	m_addDocument.ShowWindow(tab == 4 ? SW_SHOW : SW_HIDE);
	m_removeDocument.ShowWindow(tab == 4 ? SW_SHOW : SW_HIDE);
	m_linksLabel.ShowWindow(tab == 4 ? SW_SHOW : SW_HIDE);
	m_links.ShowWindow(tab == 4 ? SW_SHOW : SW_HIDE);
	m_addLink.ShowWindow(tab == 4 ? SW_SHOW : SW_HIDE);
	m_removeLink.ShowWindow(tab == 4 ? SW_SHOW : SW_HIDE);
	m_relatedTopicsLabel.ShowWindow(tab == 4 ? SW_SHOW : SW_HIDE);
	m_relatedTopics.ShowWindow(tab == 4 ? SW_SHOW : SW_HIDE);
	m_addRelatedTopic.ShowWindow(tab == 4 ? SW_SHOW : SW_HIDE);
	m_removeRelatedTopic.ShowWindow(tab == 4 ? SW_SHOW : SW_HIDE);
	m_comments.ShowWindow(tab == 0 ? SW_SHOW : SW_HIDE);
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
		m_comments.MoveWindow(commentsLeft, page.top, commentsWidth, page.Height());
	}
	else if (m_tabs.GetCurSel() == 1) {
		CStatic* labels[] = {
			&m_attributeLabels[0], &m_attributeLabels[1], &m_attributeLabels[2],
			&m_attributeLabels[3], &m_attributeLabels[4], &m_attributeLabels[5],
			&m_labelsLabel, &m_attributeLabels[9], &m_attributeLabels[10]
		};
		CWnd* controls[] = {
			&m_type, &m_stage, &m_status,
			&m_assigned, &m_priority, &m_due,
			&m_labels, &m_index, &m_serverId
		};
		const int rowsPerColumn = 3;
		const int columnWidth = page.Width() / 3;
		int labelWidths[3] = {};
		for (size_t i = 0; i < _countof(labels); ++i) {
			const int column = static_cast<int>(i) / rowsPerColumn;
			labelWidths[column] = max(labelWidths[column], getLabelWidth(*labels[i]));
		}
		for (int& width : labelWidths) {
			width += margin;
		}

		for (size_t i = 0; i < _countof(controls); ++i) {
			const int column = static_cast<int>(i) / rowsPerColumn;
			const int row = static_cast<int>(i) % rowsPerColumn;
			const int left = page.left + column * columnWidth;
			const int width = column == 2 ? static_cast<int>(page.right) - left : columnWidth - margin;
			const int top = page.top + row * (rowHeight + rowSpacing);
			labels[i]->MoveWindow(
				left, top + labelOffset, labelWidths[column], textHeight);
			const int controlLeft = left + labelWidths[column];
			int controlWidth = max(rowHeight, width - labelWidths[column]);
			if (controls[i] == &m_labels) {
				controlWidth = max(rowHeight, controlWidth - rowHeight - margin / 2);
				m_selectTopicLabels.MoveWindow(
					left + width - rowHeight, top, rowHeight, rowHeight);
			}
			const bool isEdit = controls[i]->IsKindOf(RUNTIME_CLASS(CEdit)) != FALSE;
			controls[i]->MoveWindow(
				controlLeft, top + labelOffset, controlWidth,
				isEdit ? rowHeight - labelOffset : rowsPerColumn * rowHeight);
		}
	}
	else if (m_tabs.GetCurSel() == 2) {
		const int addButtonWidth = max(
			rowHeight, static_cast<int>(dc.GetTextExtent(L"Add...").cx) + 2 * margin);
		m_addBimFiles.MoveWindow(
			page.left, page.bottom - rowHeight, addButtonWidth, rowHeight);
		m_bimFiles.MoveWindow(
			page.left, page.top, page.Width(),
			max(rowHeight, page.Height() - rowHeight - rowSpacing));
	}
	else if (m_tabs.GetCurSel() == 3) {
		const int labelIndices[] = { 7, 6, 8 };
		int labelWidth = 0;
		for (int labelIndex : labelIndices) {
			labelWidth = max(labelWidth, getLabelWidth(m_attributeLabels[labelIndex]));
		}
		labelWidth += margin;
		const int controlLeft = page.left + labelWidth;

		const int fileTop = page.top;
		m_attributeLabels[7].MoveWindow(
			page.left, fileTop + labelOffset, labelWidth, textHeight);
		m_selectSnippetFile.MoveWindow(
			page.right - rowHeight, fileTop, rowHeight, rowHeight);
		m_snippetReference.MoveWindow(
			controlLeft, fileTop + labelOffset,
			max(rowHeight, static_cast<int>(page.right) - controlLeft - rowHeight - margin / 2),
			rowHeight - labelOffset);

		const int typeTop = fileTop + rowHeight + rowSpacing;
		m_attributeLabels[6].MoveWindow(
			page.left, typeTop + labelOffset, labelWidth, textHeight);
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
			max(rowHeight, static_cast<int>(page.right) - controlLeft - externalWidth - rowSpacing));
		m_snippetType.MoveWindow(
			controlLeft, typeTop + labelOffset, typeWidth, 3 * rowHeight);
		m_snippetExternal.MoveWindow(
			controlLeft + typeWidth + rowSpacing, typeTop, externalWidth, rowHeight);

		const int schemaTop = typeTop + rowHeight + rowSpacing;
		m_attributeLabels[8].MoveWindow(
			page.left, schemaTop + labelOffset, labelWidth, textHeight);
		m_snippetSchema.MoveWindow(
			controlLeft, schemaTop + labelOffset,
			max(rowHeight, static_cast<int>(page.right) - controlLeft),
			rowHeight - labelOffset);
	}
	else if (m_tabs.GetCurSel() == 4) {
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
