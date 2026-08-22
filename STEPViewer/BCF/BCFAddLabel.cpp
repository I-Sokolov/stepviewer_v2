// W:\DevArea\RDF\stepviewer_v2\STEPViewer\BCF\BCFAddLabel.cpp : implementation file
//

#include "stdafx.h"
#include "STEPViewer.h"
#include "BCFAddLabel.h"
#include "BCFTopicView.h"

#include <unordered_set>


// CBCFAddLabel dialog

IMPLEMENT_DYNAMIC(CBCFAddLabel, CDialogEx)

CBCFAddLabel::CBCFAddLabel(CBCFTopicView& view)
	: CDialogEx(IDD_BCF_ADDLABEL, &view)
	, m_view(view)
{

}

CBCFAddLabel::~CBCFAddLabel()
{
}

void CBCFAddLabel::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_LABELS, m_wndAddLabel);
	DDX_CBString(pDX, IDC_COMBO_LABELS, m_strAddLabel);
}


BEGIN_MESSAGE_MAP(CBCFAddLabel, CDialogEx)
END_MESSAGE_MAP()


// CBCFAddLabel message handlers


BOOL CBCFAddLabel::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	auto topic = m_view.GetActiveTopic();
	if (!topic) {
		ASSERT(0);
		EndDialog(IDCANCEL);
	}

	std::unordered_set<std::string> exist;
	uint16_t i = 0;
	while (auto label = topic->GetLabel(i++)) {
		exist.insert(label);
	}

	auto& bcfProject = topic->GetProject();
	auto& ext = bcfProject.GetExtensions();
	i = 0;
	while (auto label = ext.GetElement(BCFTopicLabels, i++)) {
		if (exist.find(label) == exist.end()) {
			m_wndAddLabel.AddString(FromUTF8(label));
		}
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}


void CBCFAddLabel::OnOK()
{
	auto topic = m_view.GetActiveTopic();
	if (!topic) {
		ASSERT(0);
		EndDialog(IDCANCEL);
		return;
	}

	UpdateData();

	m_strAddLabel.Trim();
	if (!m_strAddLabel.IsEmpty()) {
		if (topic->AddLabel(ToUTF8(m_strAddLabel).c_str())) {
			CDialogEx::OnOK();
		}
		else {
			m_view.ShowLog(true);
		}
	}
	else{
		AfxMessageBox(L"Enter label");
	}
}
