#pragma once

#include "bcfAPI.h"
#include "BCFSnapshotCtrl.h"
#include "BCFViewControls.h"

class CBCFView;

class CBCFCommentForm : public CWnd
{
public:
	BOOL Create(CBCFView* pane);
	void Load(BCFComment* comment);
	bool Commit();
	BCFComment* GetComment() const { return m_comment; }
	void FocusInitialControl();

protected:
	afx_msg void OnSize(UINT type, int cx, int cy);
	afx_msg void OnViewTopic();
	afx_msg void OnTabChanged(NMHDR* header, LRESULT* result);
	afx_msg void OnSelectSnapshot();
	afx_msg void OnGetFromView();
	afx_msg void OnApplyView();
	afx_msg void OnGrabSelected();
	afx_msg void OnSelectComponents();
	afx_msg void OnGrabVisible();
	afx_msg void OnVisibleFromSelection();
	afx_msg void OnSetVisible();
	DECLARE_MESSAGE_MAP()

private:
	void UpdateHeader();
	void LoadViewPoint();
	void ShowTab(int tab);
	void ReloadSelection();
	void ReloadVisibility();
	bool CommitVisibility();
	void ReloadColoring();
	CString GetComponentText(BCFComponent& component) const;

	CBCFView* m_pane = nullptr;
	BCFComment* m_comment = nullptr;
	CButton m_viewTopic;
	CStatic m_headerInfo;
	CTabCtrl m_tabs;
	CStatic m_textLabel;
	CBCFEdit m_text;
	CBCFSnapshotCtrl m_snapshot;
	CButton m_selectSnapshot;
	CButton m_cameraGroup;
	CBCFEdit m_cameraDetails;
	CButton m_getFromView;
	CButton m_applyView;
	CButton m_selectionGroup;
	CButton m_grabSelected;
	CButton m_selectComponents;
	CBCFEdit m_selectedComponents;
	CButton m_visibilityGroup;
	CButton m_grabVisible;
	CButton m_visibleFromSelection;
	CButton m_setVisible;
	CComboBox m_visibilityMode;
	CStatic m_showLabel;
	CButton m_showSpaces;
	CButton m_showBoundaries;
	CButton m_showOpenings;
	CBCFEdit m_visibilityExceptions;
	CButton m_coloringGroup;
	CBCFEdit m_coloringDetails;
};
