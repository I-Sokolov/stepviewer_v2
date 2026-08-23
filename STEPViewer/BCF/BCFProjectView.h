#pragma once

#include "bcfAPI.h"
#include "BCFTopicView.h"

class CMySTEPViewerDoc;

class CBCFProjectView : public CDialogEx
{
	DECLARE_DYNAMIC(CBCFProjectView)

public:
	CBCFProjectView(CMySTEPViewerDoc& doc);
	virtual ~CBCFProjectView();

	bool IsBCF(LPCTSTR filePath) const;
	void Open(LPCTSTR filePath);
	void Close();
	bool SaveModified();

	BCFTopic* GetActiveTopic();
	void ViewTopicModels(BCFTopic* topic);

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_BCF_PROJECT_VIEW };
#endif

protected:
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual void OnCancel();
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()
	afx_msg void OnClose();
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	afx_msg void OnItemChangedTopics(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDoubleClickTopics(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnClickedTopicDetails();
	afx_msg void OnClickedNewTopic();
	afx_msg void OnClickedDeleteTopic();
	afx_msg void OnClickedSaveProject();
	afx_msg void OnClickedCloseDialog();
	afx_msg void OnKillFocusProjectInfo();

private:
	void LoadProject();
	void RefreshTopics(BCFTopic* selectTopic = NULL);
	void UpdateProjectInfo();
	void UpdateButtons();
	bool SaveProject();
	void ShowLog(bool knownError);
	void RestorePlacement();
	void SavePlacement();
	void RestoreColumnWidths();
	void SaveColumnWidths();

private:
	CMySTEPViewerDoc& m_doc;
	BCFProject* m_project;
	CString m_filePath;
	CString m_email;
	CString m_projectId;
	CString m_projectName;
	CListCtrl m_topics;
	CBCFTopicView m_topicView;
	bool m_initialized;
};
