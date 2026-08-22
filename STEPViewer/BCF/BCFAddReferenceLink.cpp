// BCFAddReferenceLink.cpp : implementation file
//

#include "stdafx.h"
#include "STEPViewer.h"
#include "BCFTopicView.h"
#include "BCFAddReferenceLink.h"


// CBCFAddReferenceLink dialog

IMPLEMENT_DYNAMIC(CBCFAddReferenceLink, CDialogEx)

CBCFAddReferenceLink::CBCFAddReferenceLink(CBCFTopicView& view)
	: CDialogEx(IDD_BCF_ADDREFERENCELINK, &view)
	, m_view(view)
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
		if (auto topic = m_view.GetActiveTopic()) {
			if (topic->AddReferenceLink(ToUTF8(text).c_str())) {
				CDialogEx::OnOK();
			}
			else {
				m_view.ShowLog(true);
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
