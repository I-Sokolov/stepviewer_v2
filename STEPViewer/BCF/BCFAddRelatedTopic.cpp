// W:\DevArea\RDF\stepviewer_v2\STEPViewer\BCF\BCFAddRelatedTopic.cpp : implementation file
//

#include "stdafx.h"
#include "STEPViewer.h"
#include "BCFAddRelatedTopic.h"
#include "BCFTopicView.h"

#include <unordered_set>


IMPLEMENT_DYNAMIC(CBCFAddRelatedTopic, CDialogEx)

CBCFAddRelatedTopic::CBCFAddRelatedTopic(CBCFTopicView& bcfView)
	: CDialogEx(IDD_BCF_ADDRELATEDTOPIC, &bcfView)
	, m_view(bcfView)
{

}

CBCFAddRelatedTopic::~CBCFAddRelatedTopic()
{
}

void CBCFAddRelatedTopic::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_TOPIC, m_wndListTopic);
	DDX_Control(pDX, IDOK, m_wndOK);
}


BEGIN_MESSAGE_MAP(CBCFAddRelatedTopic, CDialogEx)
	ON_LBN_SELCHANGE(IDC_LIST_TOPIC, &CBCFAddRelatedTopic::OnSelchangeListTopic)
END_MESSAGE_MAP()


// CBCFAddRelatedTopic message handlers


BOOL CBCFAddRelatedTopic::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	auto topic = &m_view.GetTopic();

	std::unordered_set<BCFTopic*> exist;
	uint16_t i = 0;
	while (auto t = topic->GetRelatedTopic(i++)) {
		exist.insert(t);
	}
	exist.insert(topic);

	auto& bcfProject = topic->GetProject();
	i = 0;
	while (auto t = bcfProject.GetTopic(i++)) {
		if (exist.find(t) == exist.end()) {
			CString text = m_view.GetTopicDisplayName(*t);
			auto item = m_wndListTopic.AddString(text);
			m_wndListTopic.SetItemDataPtr(item, t);
		}
	}

	OnSelchangeListTopic();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}


void CBCFAddRelatedTopic::OnOK()
{
	auto item = m_wndListTopic.GetCurSel();
	if (item != LB_ERR) {
		auto t = (BCFTopic*)m_wndListTopic.GetItemData(item);
		if (t) {
			auto& topic = m_view.GetTopic();
			if (topic.AddRelatedTopic(t)) {
				CDialogEx::OnOK();
			}
			else{
				m_view.ShowLog(true);
			}
		}
	}
}


void CBCFAddRelatedTopic::OnSelchangeListTopic()
{
	bool enable = false;
	auto item = m_wndListTopic.GetCurSel();
	if (item != LB_ERR) {
		auto t = (BCFTopic*)m_wndListTopic.GetItemData(item);
		if (t) {
			enable = true;
		}
	}
	m_wndOK.EnableWindow(enable);
}
