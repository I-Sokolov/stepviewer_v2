// BCF\BCFTopicView.cpp : implementation file
//

#include "stdafx.h"

#include "STEPViewer.h"
#include "STEPViewerDoc.h"
#include "STEPViewerView.h"
#include "_ap_model_factory.h"
#include "BCF\BCFTopicView.h"
#include "BCF\BCFAddLabel.h"
#include "BCF\BCFAddRelatedTopic.h"
#include "BCF\BCFAddReferenceLink.h"
#include "BCF\BCFAddDocumentReference.h"
#include "BCF\BCFBimFiles.h"

#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
 
#define TAB_Labels			3
#define TAB_Related			2
#define TAB_Links			1
#define TAB_Documents		0

// CBCFTopicView dialog

IMPLEMENT_DYNAMIC(CBCFTopicView, CDialogEx)

BEGIN_MESSAGE_MAP(CBCFTopicView, CDialogEx)
	ON_WM_CLOSE()
	ON_WM_SHOWWINDOW()
	ON_LBN_SELCHANGE(IDC_COMMENTS_LIST, &CBCFTopicView::OnSelchangeCommentsList)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB, &CBCFTopicView::OnSelchangeTab)
	ON_BN_CLICKED(IDC_BUTTON_ADD, &CBCFTopicView::OnClickedButtonAddMulti)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE, &CBCFTopicView::OnClickedButtonRemoveMulti)
	ON_LBN_SELCHANGE(IDC_MULTI_LIST, &CBCFTopicView::OnSelchangeMultiList)
	ON_EN_KILLFOCUS(IDC_TOPIC_TITLE, &CBCFTopicView::OnKillfocusEdit)
	ON_EN_KILLFOCUS(IDC_TOPIC_DESCRIPTION, &CBCFTopicView::OnKillfocusEdit)
	ON_CBN_KILLFOCUS(IDC_TOPIC_TYPE, &CBCFTopicView::OnKillfocusEdit)
	ON_CBN_KILLFOCUS(IDC_TOPIC_STAGE, &CBCFTopicView::OnKillfocusEdit)
	ON_CBN_KILLFOCUS(IDC_TOPIC_STATUS, &CBCFTopicView::OnKillfocusEdit)
	ON_CBN_KILLFOCUS(IDC_TOPIC_ASSIGNED, &CBCFTopicView::OnKillfocusEdit)
	ON_CBN_KILLFOCUS(IDC_TOPIC_PRIORITY, &CBCFTopicView::OnKillfocusEdit)
	ON_EN_KILLFOCUS(IDC_TOPIC_DUE, &CBCFTopicView::OnKillfocusEdit)
	ON_EN_KILLFOCUS(IDC_TOPIC_INDEX, &CBCFTopicView::OnKillfocusEdit)
	ON_EN_KILLFOCUS(IDC_TOPIC_SERVER_ID, &CBCFTopicView::OnKillfocusEdit)
	ON_EN_KILLFOCUS(IDC_TOPIC_COMMENT_TEXT, &CBCFTopicView::OnKillfocusTopicCommentText)
	ON_BN_CLICKED(IDC_UPDATE_VIEWPOINT, &CBCFTopicView::OnClickedUpdateViewpoint)
	ON_BN_CLICKED(IDC_BUTTON_BIMS, &CBCFTopicView::OnClickedButtonBims)
END_MESSAGE_MAP()


CBCFTopicView::CBCFTopicView(CMySTEPViewerDoc& doc)
	: CDialogEx(IDD_BCF_VIEW, AfxGetMainWnd())
	, m_doc (doc)
	, m_viewPointMgr(*this)
	, m_bcfProject(NULL)
	, m_topic(NULL)
{
}

CBCFTopicView::~CBCFTopicView()
{
	Close();
}

void CBCFTopicView::Close()
{
	CommitChanges();
	if (GetSafeHwnd()) {
		ShowWindow(SW_HIDE);
	}
	m_bcfProject = NULL;
	m_topic = NULL;
}

void CBCFTopicView::CommitChanges()
{
	if (!m_bcfProject || !GetSafeHwnd()) {
		return;
	}
	UpdateActiveTopic();
	UpateActiveComment();
}

void CBCFTopicView::Open(BCFProject& project, BCFTopic* topic)
{
	Close();
	m_bcfProject = &project;
	m_topic = topic;
	if (!IsWindow(GetSafeHwnd())) {
		Create(IDD_BCF_VIEW, AfxGetMainWnd());
	}
	ShowWindow(SW_SHOW);
	LoadTopicToView();
	SetForegroundWindow();
}

BCFBimFile* CBCFTopicView::FindBimFileByPath(BCFTopic* topic, const char* searchPath)
{
	if (topic) {
		int i = 0;
		while (auto file = topic->GetBimFile(i++)) {
			auto refPath = file->GetReference();
			if (0 == strcmp(searchPath, refPath)) {
				return file;
			}
		}
	}
	return NULL;
}

void CBCFTopicView::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TOPICS, m_wndTopic);
	DDX_Control(pDX, IDC_TOPIC_TYPE, m_wndTopicType);
	DDX_CBString(pDX, IDC_TOPIC_TYPE, m_strTopicType);
	DDX_Control(pDX, IDC_TOPIC_STAGE, m_wndTopicStage);
	DDX_CBString(pDX, IDC_TOPIC_STAGE, m_strTopicStage);
	DDX_Control(pDX, IDC_TOPIC_STATUS, m_wndTopicStatus);
	DDX_CBString(pDX, IDC_TOPIC_STATUS, m_strTopicStatus);
	DDX_Control(pDX, IDC_TOPIC_ASSIGNED, m_wndAssigned);
	DDX_CBString(pDX, IDC_TOPIC_ASSIGNED, m_strAssigned);
	DDX_Control(pDX, IDC_TOPIC_PRIORITY, m_wndPriority);
	DDX_CBString(pDX, IDC_TOPIC_PRIORITY, m_strPriority);
	DDX_Control(pDX, IDC_SNIPPET_TYPE, m_wndSnippetType);
	DDX_CBString(pDX, IDC_SNIPPET_TYPE, m_strSnippetType);
	DDX_Control(pDX, IDC_TAB, m_wndTab);
	DDX_Control(pDX, IDC_AUTHOR, m_wndAuthor);
	DDX_Control(pDX, IDC_TOPIC_DUE, m_wndDue);
	DDX_Text(pDX, IDC_TOPIC_DUE, m_strDue);
	DDX_Control(pDX, IDC_TOPIC_DESCRIPTION, m_wndDescription);
	DDX_Text(pDX, IDC_TOPIC_DESCRIPTION, m_strDescription);
	DDX_Control(pDX, IDC_TOPIC_TITLE, m_wndTitle);
	DDX_Text(pDX, IDC_TOPIC_TITLE, m_strTitle);
	DDX_Control(pDX, IDC_SNIPPET_REFERENCE, m_wndSnippetReference);
	DDX_Text(pDX, IDC_SNIPPET_REFERENCE, m_strSnippetReference);
	DDX_Control(pDX, IDC_SNIPPET_SCHEMA, m_wndSnippetSchema);
	DDX_Text(pDX, IDC_SNIPPET_SCHEMA, m_strSnippetSchema);
	DDX_Control(pDX, IDC_TOPIC_INDEX, m_wndIndex);
	DDX_Text(pDX, IDC_TOPIC_INDEX, m_strIndex);
	DDX_Control(pDX, IDC_TOPIC_SERVER_ID, m_wndServerIndex);
	DDX_Text(pDX, IDC_TOPIC_SERVER_ID, m_strServerId);
	DDX_Control(pDX, IDC_TOPIC_COMMENT_TEXT, m_wndCommentText);
	DDX_Control(pDX, IDC_COMMENTS_LIST, m_wndCommentsList);
	DDX_Control(pDX, IDC_MULTI_LIST, m_wndMultiList);
	DDX_Control(pDX, IDC_BUTTON_ADD, m_wndAddMulti);
	DDX_Control(pDX, IDC_BUTTON_REMOVE, m_wndRemoveMulti);
	DDX_Control(pDX, IDC_UPDATE_VIEWPOINT, m_wndUpdateViewPoint);
}

void CBCFTopicView::OnClose()
{
	m_wndCommentsList.SetFocus(); //to last upate from edit field
	CDialogEx::OnClose();
	Close();
}


void CBCFTopicView::OnCancel()
{
	//CDialogEx::OnCancel();
}


void CBCFTopicView::OnOK()
{
	//CDialogEx::OnOK();
}

BOOL CBCFTopicView::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_wndTab.InsertItem(0, L"Labels");
	m_wndTab.InsertItem(0, L"Related topic");
	m_wndTab.InsertItem(0, L"Links");
	m_wndTab.InsertItem(0, L"Documents");

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}


void CBCFTopicView::LoadTopicToView()
{
	CWaitCursor wait;

	UpdateTopicCaptions();
	LoadExtensions();
	LoadActiveTopic();
}

void CBCFTopicView::UpdateTopicCaptions()
{
	if (!m_topic) {
		return;
	}

	CString caption;
	caption.Format(L"BCF Topic %s", FromUTF8(m_topic->GetTitle()).GetString());
	SetWindowText(caption);

	CString topicText;
	topicText.Format(L"Topic %s", FromUTF8(m_topic->GetGuid()).GetString());
	m_wndTopic.SetWindowText(topicText);
}

void CBCFTopicView::LoadExtensions()
{
	LoadExtension(m_wndTopicType, BCFTopicTypes);
	LoadExtension(m_wndTopicStatus, BCFTopicStatuses);
	LoadExtension(m_wndPriority, BCFPriorities);
	LoadExtension(m_wndAssigned, BCFUsers);
	LoadExtension(m_wndSnippetType, BCFSnippetTypes);
	LoadExtension(m_wndTopicStage, BCFStages);
}

void CBCFTopicView::LoadExtension(CComboBox& wnd, BCFEnumeration enumeraion)
{
	CString txt;
	wnd.GetWindowText(txt);

	wnd.ResetContent();

	if (m_bcfProject) {
		auto& extensions = m_bcfProject->GetExtensions();

		uint16_t ind = 0;
		while (auto elem = extensions.GetElement(enumeraion, ind++)) {
			wnd.AddString(FromUTF8(elem));
		}
	}

	wnd.SetWindowText(txt);
}

void CBCFTopicView::ShowLog(bool knownError)
{
	const char* msg = NULL;
	if (m_bcfProject) {
		msg = m_bcfProject->GetErrors();
	}

	if (knownError) {
		if (!msg || !*msg) {
			msg = "Unknown BCF error";
		}
	}

	if (msg && *msg) {
		AfxMessageBox(FromUTF8(msg), knownError ? MB_ICONERROR : MB_ICONEXCLAMATION);
	}
}


BCFTopic* CBCFTopicView::GetActiveTopic()
{
	return m_bcfProject ? m_topic : NULL;
}

void CBCFTopicView::FillTopicAuthor(BCFTopic* topic)
{
	CString strAuthor;

	strAuthor.Format(L"Created by %s at %s", FromUTF8(topic->GetCreationAuthor()).GetString(), FromUTF8(topic->GetCreationDate()).GetString());
	if (*topic->GetModifiedAuthor()) {
		CString modifier;
		modifier.Format(L", modified by %s at %s", FromUTF8(topic->GetModifiedAuthor()).GetString(), FromUTF8(topic->GetModifiedDate()).GetString());
		strAuthor.Append(modifier);
	}

	m_wndAuthor.SetWindowText(strAuthor);
}

void CBCFTopicView::LoadActiveTopic()
{
	auto topic = GetActiveTopic();
	if (!topic) {
		return;
	}

	ViewTopicModels(topic);

	FillTopicAuthor(topic);

	m_strTitle = FromUTF8(topic->GetTitle());
	m_strDescription = FromUTF8(topic->GetDescription());

	m_strTopicType = FromUTF8(topic->GetTopicType());
	m_strTopicStage = FromUTF8(topic->GetStage());
	m_strTopicStatus = FromUTF8(topic->GetTopicStatus());
	m_strAssigned = FromUTF8(topic->GetAssignedTo());
	m_strPriority = FromUTF8(topic->GetPriority());
	m_strDue = FromUTF8(topic->GetDueDate());

	m_strIndex = FromUTF8(topic->GetIndexStr());
	m_strServerId = FromUTF8(topic->GetServerAssignedId());

	auto snippet = topic->GetBimSnippet(false);
	m_wndSnippetType.EnableWindow(snippet != NULL);
	m_wndSnippetReference.EnableWindow(snippet!=NULL);
	m_wndSnippetSchema.EnableWindow(snippet != NULL);
	if (snippet) {
		m_strSnippetType = FromUTF8(snippet->GetSnippetType());
		fs::path path = snippet->GetReference();
		m_strSnippetReference = FromUTF8(path.filename().string().c_str());
		path = snippet->GetReferenceSchema();
		m_strSnippetSchema = FromUTF8(path.filename().string().c_str());
	}
	else {
		m_strSnippetType.Empty();
		m_strSnippetReference.Empty();
		m_strSnippetSchema.Empty();
	}

	UpdateData(FALSE);

	LoadComments(topic);
	FillMultiList();
}


void CBCFTopicView::UpdateActiveTopic()
{
	auto topic = GetActiveTopic();
	if (!topic) {
		return;
	}

	UpdateData();

	bool ok = topic->SetTitle(ToUTF8(m_strTitle).c_str());
	ok = topic->SetDescription(ToUTF8(m_strDescription).c_str()) && ok;
	ok = topic->SetTopicType(ToUTF8(m_strTopicType).c_str()) && ok;
	ok = topic->SetStage(ToUTF8(m_strTopicStage).c_str()) && ok;
	ok = topic->SetTopicStatus(ToUTF8(m_strTopicStatus).c_str()) && ok;
	ok = topic->SetAssignedTo(ToUTF8(m_strAssigned).c_str()) && ok;
	ok = topic->SetPriority(ToUTF8(m_strPriority).c_str()) && ok;
	ok = topic->SetDueDate(ToUTF8(m_strDue).c_str()) && ok;
	if (m_strIndex.IsEmpty()) {
		ok = topic->SetIndexStr(ToUTF8(m_strIndex).c_str()) && ok;
	}
	else {
		ok = topic->SetIndex(_wtoi(m_strIndex)) && ok;
	}
	ok = topic->SetServerAssignedId(ToUTF8(m_strServerId).c_str()) && ok;

	ShowLog(!ok);
	if (!ok) {
		LoadActiveTopic(); //restore data was not set
	}

	FillTopicAuthor(topic);
	LoadExtensions();
	UpdateTopicCaptions();
}


void CBCFTopicView::LoadComments(BCFTopic* topic, int select)
{
	m_wndCommentsList.ResetContent();

	uint16_t i = 0;
	while (auto comment = topic->GetComment(i++)) {
		CString text;
		text.Format(L"#%d created by %s at %s",
			(int)i,
			(LPCWSTR)FromUTF8(comment->GetAuthor()),
			(LPCWSTR)FromUTF8(comment->GetDate())
		);
		if (*comment->GetModifiedAuthor()) {
			CString modifier;
			modifier.Format(L", modified by %s at %s",
				(LPCWSTR)FromUTF8(comment->GetModifiedAuthor()),
				(LPCWSTR)FromUTF8(comment->GetModifiedDate()));
			text.Append(modifier);
		}
		auto item = m_wndCommentsList.AddString(text);
		m_wndCommentsList.SetItemDataPtr(item, comment);
	}
	m_wndCommentsList.AddString(L"<My new comment>");

	m_wndCommentsList.SetCurSel(select);
	OnSelchangeCommentsList();
}

void CBCFTopicView::UpateActiveComment()
{
	auto topic = GetActiveTopic();
	if (!topic) {
		return;
	}

	auto indComment = m_wndCommentsList.GetCurSel();
	auto comment = (BCFComment*)m_wndCommentsList.GetItemDataPtr(indComment);

	CString newText;
	m_wndCommentText.GetWindowText(newText);
	newText.Trim();

	bool ok = true;

	if (!comment && !newText.IsEmpty()) {
		comment = topic->AddComment();
		if (comment)
			ok = m_viewPointMgr.SaveCurrentViewToComent(*comment) && ok;
		else
			ok = false;
	}

	if (comment) {
		ok = comment->SetText(ToUTF8(newText).c_str()) && ok;
	}

	ShowLog(!ok);

	LoadComments(topic, indComment);
}


void CBCFTopicView::OnSelchangeCommentsList()
{
	BCFComment* comment = NULL;
	if (m_bcfProject) {

		auto item = m_wndCommentsList.GetCurSel();
		comment = (BCFComment*)m_wndCommentsList.GetItemDataPtr(item);

		if (comment) {
			CString strCommentText = FromUTF8(comment->GetText());
			m_wndCommentText.SetWindowText(strCommentText);

			m_viewPointMgr.SetViewFromComment(*comment);
		}
		else {
			m_wndCommentText.SetWindowText(L""); //new comment
		}
	}

	m_wndUpdateViewPoint.EnableWindow(comment != NULL);
	
	ShowLog(false);
}

_model* CBCFTopicView::GetBimModel(BCFBimFile& file)
{
	_model* model = nullptr;

	auto it = m_mapBimFiles.find(&file);
	if (it != m_mapBimFiles.end()) {

		model = it->second;
	}
	else {
		auto path = FromUTF8(file.GetReference());
		
		for (auto pModel : m_doc.getModels()) {
			if (pModel->getPath() == path) {
				model = pModel;
				break;
			}
		}

		if (!model) {
			if (!fs::exists(ToUTF8(path)))
			{
				//we have to ask user to search the BIM file
				CString msg(L"Can not locate BIM file assigned to the topic.\n\n");
				if (!path.IsEmpty()) {
					msg += L"Reference: '" + path + L"'\n";
				}
				auto name = FromUTF8(file.GetFilename());
				if (!name.IsEmpty()) {
					msg += L"Name: '" + name + L"'\n";
				}
				auto ifcProjectGuid = FromUTF8(file.GetIfcProject());
				if (!ifcProjectGuid.IsEmpty()) {
					msg += L"IfcProject.GlobalId: '" + ifcProjectGuid + L"'\n";
				}
				msg += L"\nDo you want to locate the file manually?";

				if (IDNO != AfxMessageBox(msg, MB_YESNO | MB_ICONEXCLAMATION)) {

					CFileDialog dlgFile(TRUE, nullptr, _T(""), OFN_FILEMUSTEXIST, BIM_MODELS_FILTER);
					if (dlgFile.DoModal() != IDOK)
					{
						return nullptr;
					}
					path = dlgFile.GetPathName();
				}
			}

			if (fs::exists(ToUTF8(path))) {
				model = _ap_model_factory::load(&m_doc, path, false, !m_doc.getModels().empty() ? m_doc.getModels()[0] : nullptr, false);
				//model may be NULL, assume message was shown while load
				if (model) {
					_ptr<_ap_model> apModel(model);
					if (apModel->getAP() == enumAP::IFC) {
						m_mapBimFiles[&file] = model;
					}
					else {
						delete model;
						model = nullptr;
					}
				}
			}
		}
	}

	return model;
}

void CBCFTopicView::ViewTopicModels(BCFTopic* topic)
{
	std::vector<_model*> activeModels;

	if (topic) {
		uint16_t i = 0;
		while (BCFBimFile* file = topic->GetBimFile(i++)) {

			_model* model = GetBimModel(*file);

			if (model) {
				activeModels.push_back(model);
			}
		}
	}

	m_doc.enableModelsAddIfNeeded(activeModels);
}

CMySTEPViewerView* CBCFTopicView::GetViewerView()
{
	auto pos = m_doc.GetFirstViewPosition();
	while (auto view = m_doc.GetNextView(pos)) {
		auto stview = dynamic_cast<CMySTEPViewerView*>(view);
		if (stview) {
			return stview;
		}
	}
	return NULL;
}

void CBCFTopicView::OnSelchangeTab(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
	FillMultiList();
	*pResult = 0;
}

void CBCFTopicView::FillMultiList()
{
	auto sel = m_wndMultiList.GetCurSel();
	m_wndMultiList.ResetContent();

	auto topic = GetActiveTopic();
	
	if (topic) {
		switch (m_wndTab.GetCurSel()) {
		case TAB_Labels:
			FillLabels(topic);
			break;
		case TAB_Related:
			FillRelated(topic);
			break;
		case TAB_Links:
			FillLinks(topic);
			break;
		case TAB_Documents:
			FillDocuments(topic);
			break;
		default:
			ASSERT(FALSE);
		}
	}

	m_wndAddMulti.EnableWindow(topic != NULL);
	
	if (sel != LB_ERR) {
		m_wndMultiList.SetCurSel(sel);
	}
	OnSelchangeMultiList();
}

void CBCFTopicView::FillLabels(BCFTopic* topic)
{
	m_wndMultiList.ResetContent();
	int i = 0;
	while (auto label = topic->GetLabel(i++)) {
		m_wndMultiList.AddString(FromUTF8(label));
	}
}

void CBCFTopicView::FillRelated(BCFTopic* topic)
{
	int i = 0;
	while (auto related = topic->GetRelatedTopic(i++)) {
		auto text = GetTopicDisplayName(*related);
		auto item = m_wndMultiList.AddString(text);
		m_wndMultiList.SetItemDataPtr(item, related);
	}
}

void CBCFTopicView::FillLinks(BCFTopic* topic)
{
	int i = 0;
	while (auto link = topic->GetReferenceLink(i++)) {
		m_wndMultiList.AddString(FromUTF8(link));
	}
}

static CString GetDocumentText(BCFDocumentReference* doc)
{
	CString text = FromUTF8(doc->GetDescription());
	if (!text.IsEmpty()) {
		text.Append(L": ");
	}
	fs::path path = doc->GetFilePath();
	text += FromUTF8(path.filename().string().c_str());

	return text;
}

void CBCFTopicView::FillDocuments(BCFTopic* topic)
{
	int i = 0;
	while (auto doc = topic->GetDocumentReference(i++)) {
		auto item = m_wndMultiList.AddString(GetDocumentText(doc));
		m_wndMultiList.SetItemDataPtr(item, doc);
	}
}

void CBCFTopicView::OnClickedButtonAddMulti()
{
	auto topic = GetActiveTopic();

	if (topic) {
		switch (m_wndTab.GetCurSel()) {
		case TAB_Labels:
			AddLabel(topic);
			break;
		case TAB_Related:
			AddRelated(topic);
			break;
		case TAB_Links:
			AddLink(topic);
			break;
		case TAB_Documents:
			AddDocument(topic);
			break;
		default:
			ASSERT(FALSE);
		}

		FillMultiList();
	}
}

void CBCFTopicView::AddLabel(BCFTopic* /*topic*/)
{
	CBCFAddLabel dlg(*this);
	dlg.DoModal();
}

void CBCFTopicView::AddRelated(BCFTopic* /*topic*/)
{
	CBCFAddRelatedTopic dlg(*this);
	dlg.DoModal();
}

void CBCFTopicView::AddLink(BCFTopic* /*topic*/)
{
	CBCFAddReferenceLink dlg(*this);
	dlg.DoModal();
}

void CBCFTopicView::AddDocument(BCFTopic* /*topic*/)
{
	CBCFAddDocumentReference dlg(*this);
	dlg.DoModal();
}

void CBCFTopicView::OnClickedButtonRemoveMulti()
{
	auto topic = GetActiveTopic();

	if (topic) {
		switch (m_wndTab.GetCurSel()) {
		case TAB_Labels:
			RemoveLabel(topic);
			break;
		case TAB_Related:
			RemoveRelated(topic);
			break;
		case TAB_Links:
			RemoveLink(topic);
			break;
		case TAB_Documents:
			RemoveDocument(topic);
			break;
		default:
			ASSERT(FALSE);
		}

		FillMultiList();
	}
}

void CBCFTopicView::RemoveLabel(BCFTopic* topic)
{
	auto sel = m_wndMultiList.GetCurSel();
	if (sel != LB_ERR) {
		CString label;
		m_wndMultiList.GetText(sel, label);
		if (IDYES == AfxMessageBox(L"Do you want to delete label " + label + L"?", MB_YESNO)) {
			if (!topic->RemoveLabel(ToUTF8(label).c_str())) {
				ShowLog(true);
			}
		}
	}
}

void CBCFTopicView::RemoveRelated(BCFTopic* topic)
{
	auto sel = m_wndMultiList.GetCurSel();
	if (sel != LB_ERR) {
		auto t = (BCFTopic*)m_wndMultiList.GetItemDataPtr(sel);
		if (t) {
			if (!topic->RemoveRelatedTopic(t)) {
				ShowLog(true);
			}
		}
	}
}

void CBCFTopicView::RemoveLink(BCFTopic* topic)
{
	auto sel = m_wndMultiList.GetCurSel();
	if (sel != LB_ERR) {
		CString link;
		m_wndMultiList.GetText(sel, link);
		if (IDYES == AfxMessageBox(L"Do you want to delete reference link '" + link + L"'?", MB_YESNO)) {
			if (!topic->RemoveReferenceLink(ToUTF8(link).c_str())) {
				ShowLog(true);
			}
		}
	}
}

void CBCFTopicView::RemoveDocument(BCFTopic* /*topic*/)
{
	auto sel = m_wndMultiList.GetCurSel();
	if (sel != LB_ERR) {
		auto doc = (BCFDocumentReference*)m_wndMultiList.GetItemDataPtr(sel);
		if (doc) {
			CString quest;
			quest.Format(L"Do you want to remove reference to document '%s'?", GetDocumentText(doc).GetString());
			if (IDYES == AfxMessageBox(quest, MB_YESNO)) {
				if (!doc->Remove()) {
					ShowLog(true);
				}
			}
		}
	}
}



void CBCFTopicView::OnSelchangeMultiList()
{
	m_wndRemoveMulti.EnableWindow(m_wndMultiList.GetCurSel() != LB_ERR);
}


void CBCFTopicView::OnKillfocusEdit()
{
	UpdateActiveTopic();
}

void CBCFTopicView::OnKillfocusTopicCommentText()
{
	UpateActiveComment();
}


void CBCFTopicView::OnClickedUpdateViewpoint()
{
	auto indComment = m_wndCommentsList.GetCurSel();
	if (auto comment = (BCFComment*)m_wndCommentsList.GetItemDataPtr(indComment)) {
		bool ok = m_viewPointMgr.SaveCurrentViewToComent(*comment);
		ShowLog(!ok);
	}
}

CString CBCFTopicView::GetTopicDisplayName(BCFTopic& topic)
{
	int i = 0;
	auto& bcfProject = topic.GetProject();
	while (auto t = bcfProject.GetTopic(i++)) {
		if (t == &topic) {
			break;
		}
	}

	auto guid = FromUTF8(topic.GetGuid());
	auto title = FromUTF8(topic.GetTitle());

	CString text;
	text.Format(L"#%d: %s - %s", i, guid.GetString(), title.GetString());

	return text;
}


void CBCFTopicView::OnClickedButtonBims()
{
	CBCFBimFiles dlg(*this);
	dlg.DoModal();
}
