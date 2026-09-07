// BCFAddReferenceLink.cpp : implementation file
//

#include "stdafx.h"
#include "STEPViewer.h"
#include "BCFTopicDlg.h"
#include "BCFAddReferenceLink.h"
#include "BCFView.h"


// CBCFAddReferenceLink dialog

IMPLEMENT_DYNAMIC(CBCFAddReferenceLink, CDialogEx)

CBCFAddReferenceLink::CBCFAddReferenceLink(CBCFTopicDlg& view)
	: CDialogEx(IDD_BCF_ADDREFERENCELINK, &view)
	, m_topic(&view.GetTopic())
	, m_topicView(&view)
	, m_paneView(nullptr)
{

}

CBCFAddReferenceLink::CBCFAddReferenceLink(CBCFView& view, BCFTopic& topic)
	: CDialogEx(IDD_BCF_ADDREFERENCELINK, &view)
	, m_topic(&topic)
	, m_topicView(nullptr)
	, m_paneView(&view)
{
}

CBCFAddReferenceLink::~CBCFAddReferenceLink()
{
}

void CBCFAddReferenceLink::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT1, m_wndEdit);
	DDX_Control(pDX, IDOK, m_wndOK);
}


BEGIN_MESSAGE_MAP(CBCFAddReferenceLink, CDialogEx)
	ON_EN_CHANGE(IDC_EDIT1, &CBCFAddReferenceLink::OnChangeEdit)
END_MESSAGE_MAP()


void CBCFAddReferenceLink::OnOK()
{
	CString text;
	m_wndEdit.GetWindowText(text);
	text.Trim();
	if (!text.IsEmpty())
	{
		if (m_topic->AddReferenceLink(ToUTF8(text).c_str())) {
			CDialogEx::OnOK();
		}
		else {
			if (m_topicView) {
				m_topicView->ShowLog(true);
			}
			else {
				m_paneView->ShowLog(true);
			}
		}
	}
}


void CBCFAddReferenceLink::OnChangeEdit()
{
	CString text;
	m_wndEdit.GetWindowText(text);
	text.Trim();
	m_wndOK.EnableWindow(!text.IsEmpty());
}
