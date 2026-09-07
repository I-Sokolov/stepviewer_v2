#pragma once

#include "bcfAPI.h"
#include "BCFViewControls.h"

class CBCFView;

class CBCFCommentForm : public CWnd
{
public:
	BOOL Create(CBCFView* pane);
	void Load(BCFComment* comment);
	bool Commit();
	BCFComment* GetComment() const { return m_comment; }

protected:
	afx_msg void OnSize(UINT type, int cx, int cy);
	afx_msg void OnViewTopic();
	afx_msg void OnTabChanged(NMHDR* header, LRESULT* result);
	afx_msg void OnCameraChanged();
	afx_msg void OnSelectSnapshot();
	afx_msg void OnFromView();
	afx_msg void OnToView();
	DECLARE_MESSAGE_MAP()

private:
	void UpdateHeader();
	void LoadViewPoint();
	bool CommitViewPoint();
	void ShowTab(int tab);
	void UpdateCameraControls();

	CBCFView* m_pane = nullptr;
	BCFComment* m_comment = nullptr;
	CButton m_viewTopic;
	CStatic m_headerInfo;
	CTabCtrl m_tabs;
	CStatic m_textLabel;
	CBCFEdit m_text;
	CStatic m_snapshot;
	CButton m_selectSnapshot;
	CButton m_captureSnapshot;
	CButton m_cameraGroup;
	CStatic m_cameraLabels[6];
	CComboBox m_camera;
	CBCFEdit m_cameraValues[6];
	CButton m_fromView;
	CButton m_toView;
};
