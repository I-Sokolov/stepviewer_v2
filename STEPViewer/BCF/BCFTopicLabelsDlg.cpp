#include "stdafx.h"

#include "BCFTopicLabelsDlg.h"
#include "Resource.h"
#include "bcfAPI.h"

#include <unordered_set>
#include <vector>

IMPLEMENT_DYNAMIC(CBCFTopicLabelsDlg, CDialogEx)

CBCFTopicLabelsDlg::CBCFTopicLabelsDlg(BCFTopic& topic, CWnd* parent)
	: CDialogEx(IDD_BCF_TOPIC_LABELS, parent)
	, m_topic(topic)
{
}

void CBCFTopicLabelsDlg::DoDataExchange(CDataExchange* dataExchange)
{
	CDialogEx::DoDataExchange(dataExchange);
	DDX_Control(dataExchange, IDC_BCF_TOPIC_LABELS, m_labels);
}

BEGIN_MESSAGE_MAP(CBCFTopicLabelsDlg, CDialogEx)
END_MESSAGE_MAP()

BOOL CBCFTopicLabelsDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	m_labels.SetCheckStyle(BS_AUTOCHECKBOX);

	std::unordered_set<std::string> selected;
	for (uint16_t i = 0; const char* label = m_topic.GetLabel(i); ++i) {
		selected.insert(label);
	}

	std::unordered_set<std::string> added;
	BCFExtensions& extensions = m_topic.GetProject().GetExtensions();
	for (uint16_t i = 0; const char* label = extensions.GetElement(BCFTopicLabels, i); ++i) {
		int item = m_labels.AddString(FromUTF8(label));
		m_labels.SetCheck(item, selected.count(label) ? BST_CHECKED : BST_UNCHECKED);
		added.insert(label);
	}
	for (const std::string& label : selected) {
		if (!added.count(label)) {
			int item = m_labels.AddString(FromUTF8(label.c_str()));
			m_labels.SetCheck(item, BST_CHECKED);
		}
	}

	return TRUE;
}

void CBCFTopicLabelsDlg::OnOK()
{
	std::unordered_set<std::string> selected;
	for (int i = 0; i < m_labels.GetCount(); ++i) {
		if (m_labels.GetCheck(i) == BST_CHECKED) {
			CString label;
			m_labels.GetText(i, label);
			selected.insert(ToUTF8(label));
		}
	}

	std::vector<std::string> existing;
	for (uint16_t i = 0; const char* label = m_topic.GetLabel(i); ++i) {
		existing.emplace_back(label);
	}

	bool ok = true;
	for (const std::string& label : existing) {
		if (!selected.count(label)) {
			ok = m_topic.RemoveLabel(label.c_str()) && ok;
		}
		else {
			selected.erase(label);
		}
	}
	for (const std::string& label : selected) {
		ok = m_topic.AddLabel(label.c_str()) && ok;
	}

	if (!ok) {
		AfxMessageBox(L"Failed to update topic labels.", MB_ICONERROR);
		return;
	}
	CDialogEx::OnOK();
}
