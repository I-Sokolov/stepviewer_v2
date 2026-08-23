// W:\DevArea\RDF\stepviewer_v2\STEPViewer\BCF\BCFAddDocumentReference.cpp : implementation file
//

#include "stdafx.h"
#include "STEPViewer.h"
#include "BCFAddDocumentReference.h"
#include "BCFTopicView.h"

// CBCFAddDocumentReference dialog

IMPLEMENT_DYNAMIC(CBCFAddDocumentReference, CDialogEx)

CBCFAddDocumentReference::CBCFAddDocumentReference(CBCFTopicView& view)
	: CDialogEx(IDD_BCF_ADDOCUMENT, &view)
	, m_view(view)
	, m_strPath(_T(""))
	, m_strDescription(_T(""))
	, m_isExternal(TRUE)
{

}

CBCFAddDocumentReference::~CBCFAddDocumentReference()
{
}

void CBCFAddDocumentReference::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_PATH, m_strPath);
	DDX_Text(pDX, IDC_EDIT_DESCRIPTION, m_strDescription);
	DDX_Check(pDX, IDC_CHECK_EXTERNAL, m_isExternal);
}


BEGIN_MESSAGE_MAP(CBCFAddDocumentReference, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_BROWSE, &CBCFAddDocumentReference::OnClickedButtonBrowse)
END_MESSAGE_MAP()


// CBCFAddDocumentReference message handlers


void CBCFAddDocumentReference::OnOK()
{
	auto topic = &m_view.GetTopic();

	UpdateData();

	m_strPath.Trim();
	m_strDescription.Trim();

	if (m_strPath.IsEmpty()) {
		AfxMessageBox(L"Path is empty", MB_ICONSTOP);
		return;
	}

	auto doc = topic->AddDocumentReference(ToUTF8(m_strPath).c_str(), m_isExternal);
	if (!doc) {
		m_view.ShowLog(true);
		return;
	}

	if (!m_strDescription.IsEmpty()) {
		if (!doc->SetDescription(ToUTF8(m_strDescription).c_str())) {
			m_view.ShowLog(true);
		}
	}

	CDialogEx::OnOK();
}


void CBCFAddDocumentReference::OnClickedButtonBrowse()
{
	UpdateData();

#define DOCS L"*.txt; *.pdf; *.doc; *.docx; *.xls; *.xlsx"
#define FILTER L"Document files(" DOCS ")|" DOCS "|All files(*.*)|*.*"
	CFileDialog dlgFile(TRUE, nullptr, m_strPath, OFN_FILEMUSTEXIST, FILTER);

	if (dlgFile.DoModal() == IDOK) {
		m_strPath = dlgFile.GetPathName();
		UpdateData(false);
	}
}
