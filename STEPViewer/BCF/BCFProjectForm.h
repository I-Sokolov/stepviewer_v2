#pragma once

#include "bcfAPI.h"

class CBCFView;

class CBCFProjectForm : public CWnd
{
public:
	BOOL Create(CBCFView* pane);
	void Load(BCFTopic* selectTopic = nullptr);
	bool Commit();
	BCFTopic* GetSelectedTopic() const;

protected:
	afx_msg void OnSize(UINT type, int cx, int cy);
	afx_msg void OnTopicChanged(NMHDR* header, LRESULT* result);
	afx_msg void OnTopicDoubleClick(NMHDR* header, LRESULT* result);
	afx_msg void OnNewTopic();
	afx_msg void OnTopicDetails();
	afx_msg void OnDeleteTopic();
	afx_msg void OnNewFile();
	afx_msg void OnOpenFile();
	afx_msg void OnSaveFile();
	DECLARE_MESSAGE_MAP()

private:
	void UpdateButtons();

	CBCFView* m_pane = nullptr;
	CStatic m_topicsLabel;
	CListCtrl m_topics;
	CButton m_newFile;
	CButton m_openFile;
	CButton m_saveFile;
	CButton m_newTopic;
	CButton m_topicDetails;
	CButton m_deleteTopic;
};
