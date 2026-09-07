#pragma once

#include "Resource.h"

struct BCFTopic;

class CBCFTopicLabelsDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CBCFTopicLabelsDlg)

public:
	explicit CBCFTopicLabelsDlg(BCFTopic& topic, CWnd* parent = nullptr);

	enum { IDD = IDD_BCF_TOPIC_LABELS };

protected:
	virtual void DoDataExchange(CDataExchange* dataExchange) override;
	virtual BOOL OnInitDialog() override;
	virtual void OnOK() override;
	afx_msg void OnCreateLabel();

	DECLARE_MESSAGE_MAP()

private:
	BCFTopic& m_topic;
	CCheckListBox m_labels;
};
