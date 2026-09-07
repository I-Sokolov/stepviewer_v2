#include "stdafx.h"

#include "BCFTopicLabelsDlg.h"
#include "Resource.h"
#include "bcfAPI.h"

#include <unordered_set>
#include <vector>

namespace
{
	class CCreateLabelDlg : public CDialogEx
	{
	public:
		explicit CCreateLabelDlg(CWnd* parent)
			: CDialogEx(IDD_BCF_CREATE_LABEL, parent)
		{
		}

		CString GetLabel() const { return m_label; }

	protected:
		virtual void DoDataExchange(CDataExchange* dataExchange) override
		{
			CDialogEx::DoDataExchange(dataExchange);
			DDX_Text(dataExchange, IDC_BCF_NEW_LABEL, m_label);
		}

		virtual void OnOK() override
		{
			UpdateData(TRUE);
			m_label.Trim();
			if (m_label.IsEmpty()) {
				AfxMessageBox(L"Enter label.", MB_ICONEXCLAMATION);
				return;
			}
			CDialogEx::OnOK();
		}

	private:
		CString m_label;
	};
}

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
	ON_BN_CLICKED(IDC_BCF_CREATE_LABEL, &CBCFTopicLabelsDlg::OnCreateLabel)
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

void CBCFTopicLabelsDlg::OnCreateLabel()
{
	CCreateLabelDlg dialog(this);
	if (dialog.DoModal() != IDOK) {
		return;
	}

	CString label = dialog.GetLabel();
	int item = m_labels.FindStringExact(-1, label);
	if (item == LB_ERR) {
		BCFExtensions& extensions = m_topic.GetProject().GetExtensions();
		if (!extensions.AddElement(BCFTopicLabels, ToUTF8(label).c_str())) {
			AfxMessageBox(L"Failed to create label.", MB_ICONERROR);
			return;
		}
		item = m_labels.AddString(label);
	}
	m_labels.SetCheck(item, BST_CHECKED);
	m_labels.SetCurSel(item);
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
