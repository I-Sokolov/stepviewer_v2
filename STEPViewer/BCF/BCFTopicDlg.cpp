// BCF\BCFTopicDlg.cpp : implementation file
//

#include "stdafx.h"

#include "STEPViewer.h"
#include "STEPViewerDoc.h"
#include "BCF\BCFProjectDlg.h"
#include "BCF\BCFTopicDlg.h"
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

BEGIN_MESSAGE_MAP(CBCFCommentsListBox, CListBox)
	ON_WM_SIZE()
END_MESSAGE_MAP()

namespace
{
	CString GetCommentMetadata(BCFComment& comment)
	{
		CString metadata;
		metadata.Format(L"Created by %s %s",
			FromUTF8(comment.GetAuthor()).GetString(),
			CBCFProjectDlg::FormatDateTime(comment.GetDate()).GetString());

		if (*comment.GetModifiedAuthor() || *comment.GetModifiedDate()) {
			CString modified;
			modified.Format(L"\nModified by %s %s",
				FromUTF8(comment.GetModifiedAuthor()).GetString(),
				CBCFProjectDlg::FormatDateTime(comment.GetModifiedDate()).GetString());
			metadata.Append(modified);
		}
		return metadata;
	}
}

int CBCFCommentsListBox::AddComment(BCFComment& comment)
{
	int item = AddString(FromUTF8(comment.GetText()));
	if (item != LB_ERR && item != LB_ERRSPACE) {
		SetItemDataPtr(item, &comment);
		SetItemHeight(item, MeasureCommentHeight(&comment));
	}
	return item;
}

int CBCFCommentsListBox::MeasureCommentHeight(BCFComment* comment) const
{
	CClientDC dc(const_cast<CBCFCommentsListBox*>(this));
	CFont* oldFont = dc.SelectObject(GetFont());

	CRect client;
	GetClientRect(client);
	int scale = dc.GetDeviceCaps(LOGPIXELSY);
	int outerMargin = MulDiv(4, scale, 96);
	int padding = MulDiv(8, scale, 96);
	int spacing = MulDiv(6, scale, 96);
	int width = max(MulDiv(80, scale, 96),
		client.Width() - GetSystemMetrics(SM_CXVSCROLL) - 2 * (outerMargin + padding));

	CString text = comment ? FromUTF8(comment->GetText()) : CString();
	if (text.IsEmpty()) {
		text = L"(No text)";
	}
	CRect textRect(0, 0, width, 0);
	dc.DrawText(text, textRect, DT_CALCRECT | DT_WORDBREAK | DT_EDITCONTROL | DT_NOPREFIX);

	CString metadata = comment ? GetCommentMetadata(*comment) : CString(L" ");
	CRect metadataRect(0, 0, width, 0);
	dc.DrawText(metadata, metadataRect, DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX);

	dc.SelectObject(oldFont);
	return 2 * (outerMargin + padding) + spacing + textRect.Height() + metadataRect.Height();
}

void CBCFCommentsListBox::MeasureItem(LPMEASUREITEMSTRUCT measureItem)
{
	BCFComment* comment = nullptr;
	if (measureItem->itemID != static_cast<UINT>(-1)) {
		auto data = GetItemDataPtr(measureItem->itemID);
		if (data != reinterpret_cast<void*>(LB_ERR)) {
			comment = static_cast<BCFComment*>(data);
		}
	}
	measureItem->itemHeight = MeasureCommentHeight(comment);
}

void CBCFCommentsListBox::DrawItem(LPDRAWITEMSTRUCT drawItem)
{
	if (drawItem->itemID == static_cast<UINT>(-1)) {
		return;
	}

	CDC dc;
	dc.Attach(drawItem->hDC);
	int savedState = dc.SaveDC();

	bool selected = (drawItem->itemState & ODS_SELECTED) != 0;
	COLORREF background = GetSysColor(selected ? COLOR_HIGHLIGHT : COLOR_WINDOW);
	COLORREF textColor = GetSysColor(selected ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT);
	COLORREF metadataColor = selected ? textColor : GetSysColor(COLOR_GRAYTEXT);

	CRect itemRect(drawItem->rcItem);
	dc.FillSolidRect(itemRect, GetSysColor(COLOR_BTNFACE));

	int scale = dc.GetDeviceCaps(LOGPIXELSY);
	int outerMargin = MulDiv(4, scale, 96);
	int padding = MulDiv(8, scale, 96);
	int spacing = MulDiv(6, scale, 96);
	CRect cardRect(itemRect);
	cardRect.DeflateRect(outerMargin, outerMargin);
	dc.FillSolidRect(cardRect, background);
	dc.Draw3dRect(cardRect, GetSysColor(COLOR_3DSHADOW), GetSysColor(COLOR_3DHILIGHT));

	auto data = GetItemDataPtr(drawItem->itemID);
	auto comment = data == reinterpret_cast<void*>(LB_ERR) ? nullptr : static_cast<BCFComment*>(data);
	if (comment) {
		CFont* oldFont = dc.SelectObject(GetFont());
		dc.SetBkMode(TRANSPARENT);

		CRect contentRect(cardRect);
		contentRect.DeflateRect(padding, padding);
		CString metadata = GetCommentMetadata(*comment);
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
		auto data = GetItemDataPtr(item);
		if (data != reinterpret_cast<void*>(LB_ERR)) {
			SetItemHeight(item, MeasureCommentHeight(static_cast<BCFComment*>(data)));
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

// CBCFTopicDlg dialog

IMPLEMENT_DYNAMIC(CBCFTopicDlg, CDialogEx)

BEGIN_MESSAGE_MAP(CBCFTopicDlg, CDialogEx)
	ON_WM_SHOWWINDOW()
	ON_LBN_SELCHANGE(IDC_COMMENTS_LIST, &CBCFTopicDlg::OnSelchangeCommentsList)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB, &CBCFTopicDlg::OnSelchangeTab)
	ON_BN_CLICKED(IDC_BUTTON_ADD, &CBCFTopicDlg::OnClickedButtonAddMulti)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE, &CBCFTopicDlg::OnClickedButtonRemoveMulti)
	ON_LBN_SELCHANGE(IDC_MULTI_LIST, &CBCFTopicDlg::OnSelchangeMultiList)
	ON_BN_CLICKED(IDC_UPDATE_VIEWPOINT, &CBCFTopicDlg::OnClickedUpdateViewpoint)
	ON_BN_CLICKED(IDC_BUTTON_BIMS, &CBCFTopicDlg::OnClickedButtonBims)
END_MESSAGE_MAP()


CBCFTopicDlg::CBCFTopicDlg(CBCFProjectDlg& projectDlg, BCFTopic& topic)
	: CDialogEx(IDD_BCF_TOPIC_DLG, &projectDlg)
	, m_projectDlg(projectDlg)
	, m_topic(topic)
	, m_viewPointMgr(projectDlg.GetViewerDoc())
{
}

CBCFTopicDlg::~CBCFTopicDlg()
{
}

void CBCFTopicDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TOPIC_INFO, m_wndTopicInfo);
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
	DDX_Control(pDX, IDC_COMMENTS_LIST, m_wndCommentsList);
	DDX_Control(pDX, IDC_MULTI_LIST, m_wndMultiList);
	DDX_Control(pDX, IDC_BUTTON_ADD, m_wndAddMulti);
	DDX_Control(pDX, IDC_BUTTON_REMOVE, m_wndRemoveMulti);
}

void CBCFTopicDlg::OnCancel()
{
	CDialogEx::OnCancel();
}


void CBCFTopicDlg::OnOK()
{
	UpdateData();
	SaveTopic();
	CDialogEx::OnOK();
}

BOOL CBCFTopicDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_wndTab.InsertItem(0, L"Labels");
	m_wndTab.InsertItem(0, L"Related topic");
	m_wndTab.InsertItem(0, L"Links");
	m_wndTab.InsertItem(0, L"Documents");

	LoadView();

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}


void CBCFTopicDlg::LoadView()
{
	CWaitCursor wait;

	UpdateTopicInfo();
	LoadExtensions();
	LoadBIMFiles();
	LoadTopic();
}

void CBCFTopicDlg::UpdateTopicInfo()
{
	CString caption;
	caption.Format(L"BCF Topic %s", FromUTF8(m_topic.GetTitle()).GetString());
	SetWindowText(caption);

	CString topicInfo;
	topicInfo.Format(L"Topic %s. Created by %s %s",
		FromUTF8(m_topic.GetGuid()).GetString(),
		FromUTF8(m_topic.GetCreationAuthor()).GetString(),
		CBCFProjectDlg::FormatDateTime(m_topic.GetCreationDate()).GetString());
	if (*m_topic.GetModifiedAuthor()) {
		CString modified;
		modified.Format(L", modified by %s %s",
			FromUTF8(m_topic.GetModifiedAuthor()).GetString(),
			CBCFProjectDlg::FormatDateTime(m_topic.GetModifiedDate()).GetString());
		topicInfo.Append(modified);
	}
	m_wndTopicInfo.SetWindowText(topicInfo);
}

void CBCFTopicDlg::LoadExtensions()
{
	LoadExtension(m_wndTopicType, BCFTopicTypes);
	LoadExtension(m_wndTopicStatus, BCFTopicStatuses);
	LoadExtension(m_wndPriority, BCFPriorities);
	LoadExtension(m_wndAssigned, BCFUsers);
	LoadExtension(m_wndSnippetType, BCFSnippetTypes);
	LoadExtension(m_wndTopicStage, BCFStages);
}

void CBCFTopicDlg::LoadExtension(CComboBox& wnd, BCFEnumeration enumeraion)
{
	CString txt;
	wnd.GetWindowText(txt);

	wnd.ResetContent();

	auto& extensions = m_topic.GetProject().GetExtensions();
	uint16_t ind = 0;
	while (auto elem = extensions.GetElement(enumeraion, ind++)) {
		wnd.AddString(FromUTF8(elem));
	}

	wnd.SetWindowText(txt);
}

void CBCFTopicDlg::ShowLog(bool knownError)
{
	const char* msg = m_topic.GetProject().GetErrors();

	if (knownError) {
		if (!msg || !*msg) {
			msg = "Unknown BCF error";
		}
	}

	if (msg && *msg) {
		AfxMessageBox(FromUTF8(msg), knownError ? MB_ICONERROR : MB_ICONEXCLAMATION);
	}
}


void CBCFTopicDlg::LoadTopic()
{
	m_strTitle = FromUTF8(m_topic.GetTitle());
	m_strDescription = FromUTF8(m_topic.GetDescription());

	m_strTopicType = FromUTF8(m_topic.GetTopicType());
	m_strTopicStage = FromUTF8(m_topic.GetStage());
	m_strTopicStatus = FromUTF8(m_topic.GetTopicStatus());
	m_strAssigned = FromUTF8(m_topic.GetAssignedTo());
	m_strPriority = FromUTF8(m_topic.GetPriority());
	m_strDue = FromUTF8(m_topic.GetDueDate());

	m_strIndex = FromUTF8(m_topic.GetIndexStr());
	m_strServerId = FromUTF8(m_topic.GetServerAssignedId());

	auto snippet = m_topic.GetBimSnippet(false);
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

	LoadComments();
	FillMultiList();
}


void CBCFTopicDlg::SaveTopic()
{
	bool ok = m_topic.SetTitle(ToUTF8(m_strTitle).c_str());
	ok = m_topic.SetDescription(ToUTF8(m_strDescription).c_str()) && ok;
	ok = m_topic.SetTopicType(ToUTF8(m_strTopicType).c_str()) && ok;
	ok = m_topic.SetStage(ToUTF8(m_strTopicStage).c_str()) && ok;
	ok = m_topic.SetTopicStatus(ToUTF8(m_strTopicStatus).c_str()) && ok;
	ok = m_topic.SetAssignedTo(ToUTF8(m_strAssigned).c_str()) && ok;
	ok = m_topic.SetPriority(ToUTF8(m_strPriority).c_str()) && ok;
	ok = m_topic.SetDueDate(ToUTF8(m_strDue).c_str()) && ok;
	if (m_strIndex.IsEmpty()) {
		ok = m_topic.SetIndexStr(ToUTF8(m_strIndex).c_str()) && ok;
	}
	else {
		ok = m_topic.SetIndex(_wtoi(m_strIndex)) && ok;
	}
	ok = m_topic.SetServerAssignedId(ToUTF8(m_strServerId).c_str()) && ok;

	ShowLog(!ok);
	if (!ok) {
		LoadTopic(); //restore data was not set
	}

	LoadExtensions();
	UpdateTopicInfo();
}


void CBCFTopicDlg::LoadComments(int select)
{
	m_wndCommentsList.ResetContent();

	uint16_t i = 0;
	while (auto comment = m_topic.GetComment(i++)) {
		m_wndCommentsList.AddComment(*comment);
	}

	m_wndCommentsList.SetCurSel(select);
	OnSelchangeCommentsList();
}

void CBCFTopicDlg::OnSelchangeCommentsList()
{
	BCFComment* comment = NULL;
	auto item = m_wndCommentsList.GetCurSel();
	comment = item == LB_ERR ? NULL : (BCFComment*)m_wndCommentsList.GetItemDataPtr(item);

	if (comment) {
		m_viewPointMgr.SetViewFromComment(*comment);
	}

	ShowLog(false);
}

_model* CBCFTopicDlg::GetBimModel(BCFBimFile& file)
{
	return m_projectDlg.GetBimModel(file);
}

void CBCFTopicDlg::LoadBIMFiles()
{
	std::vector<_model*> activeModels;
	uint16_t i = 0;
	while (auto file = m_topic.GetBimFile(i++)) {
		if (auto model = GetBimModel(*file)) {
			activeModels.push_back(model);
		}
	}
	m_projectDlg.GetViewerDoc().enableModelsAddIfNeeded(activeModels);
}

void CBCFTopicDlg::OnSelchangeTab(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
	FillMultiList();
	*pResult = 0;
}

void CBCFTopicDlg::FillMultiList()
{
	auto sel = m_wndMultiList.GetCurSel();
	m_wndMultiList.ResetContent();

	auto topic = &m_topic;
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

	m_wndAddMulti.EnableWindow(TRUE);
	
	if (sel != LB_ERR) {
		m_wndMultiList.SetCurSel(sel);
	}
	OnSelchangeMultiList();
}

void CBCFTopicDlg::FillLabels(BCFTopic* topic)
{
	m_wndMultiList.ResetContent();
	uint16_t i = 0;
	while (auto label = topic->GetLabel(i++)) {
		m_wndMultiList.AddString(FromUTF8(label));
	}
}

void CBCFTopicDlg::FillRelated(BCFTopic* topic)
{
	uint16_t i = 0;
	while (auto related = topic->GetRelatedTopic(i++)) {
		auto text = GetTopicDisplayName(*related);
		auto item = m_wndMultiList.AddString(text);
		m_wndMultiList.SetItemDataPtr(item, related);
	}
}

void CBCFTopicDlg::FillLinks(BCFTopic* topic)
{
	uint16_t i = 0;
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

void CBCFTopicDlg::FillDocuments(BCFTopic* topic)
{
	uint16_t i = 0;
	while (auto doc = topic->GetDocumentReference(i++)) {
		auto item = m_wndMultiList.AddString(GetDocumentText(doc));
		m_wndMultiList.SetItemDataPtr(item, doc);
	}
}

void CBCFTopicDlg::OnClickedButtonAddMulti()
{
	auto topic = &m_topic;
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

void CBCFTopicDlg::AddLabel(BCFTopic* /*topic*/)
{
	CBCFAddLabel dlg(*this);
	dlg.DoModal();
}

void CBCFTopicDlg::AddRelated(BCFTopic* /*topic*/)
{
	CBCFAddRelatedTopic dlg(*this);
	dlg.DoModal();
}

void CBCFTopicDlg::AddLink(BCFTopic* /*topic*/)
{
	CBCFAddReferenceLink dlg(*this);
	dlg.DoModal();
}

void CBCFTopicDlg::AddDocument(BCFTopic* /*topic*/)
{
	CBCFAddDocumentReference dlg(*this);
	dlg.DoModal();
}

void CBCFTopicDlg::OnClickedButtonRemoveMulti()
{
	auto topic = &m_topic;
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

void CBCFTopicDlg::RemoveLabel(BCFTopic* topic)
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

void CBCFTopicDlg::RemoveRelated(BCFTopic* topic)
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

void CBCFTopicDlg::RemoveLink(BCFTopic* topic)
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

void CBCFTopicDlg::RemoveDocument(BCFTopic* /*topic*/)
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



void CBCFTopicDlg::OnSelchangeMultiList()
{
	m_wndRemoveMulti.EnableWindow(m_wndMultiList.GetCurSel() != LB_ERR);
}


void CBCFTopicDlg::OnClickedUpdateViewpoint()
{
	auto indComment = m_wndCommentsList.GetCurSel();
	if (auto comment = (BCFComment*)m_wndCommentsList.GetItemDataPtr(indComment)) {
		bool ok = m_viewPointMgr.SaveCurrentViewToComent(*comment);
		ShowLog(!ok);
	}
}

CString CBCFTopicDlg::GetTopicDisplayName(BCFTopic& topic)
{
	uint16_t i = 0;
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


void CBCFTopicDlg::OnClickedButtonBims()
{
	CBCFBimFiles dlg(*this);
	dlg.DoModal();
}
