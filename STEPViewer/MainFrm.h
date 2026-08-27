
// MainFrm.h : interface of the CMainFrame class
//

#pragma once

#include "Controller.h"
#include "StructureView.h"
#include "PropertiesWnd.h"
#include "DesignTreeView.h"
#include "SchemaView.h"
#include "RelationsView.h"
#include "CustomStatusBar.h"
#include "BCF\BCFView.h"

class CMainFrame : public CFrameWndEx
{
	
private: // Methods

	// --------------------------------------------------------------------------------------------
	// Controller - MVC
	CController* GetController() const;


protected: // create from serialization only
	CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)

// Attributes
public:

// Operations
public:

// Overrides
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL LoadFrame(UINT nIDResource, DWORD dwDefaultStyle = WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, CWnd* pParentWnd = nullptr, CCreateContext* pContext = nullptr);

// Implementation
public:
	virtual ~CMainFrame();
	CBCFView& GetBCFView() { return m_bcfView; }
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // control bar embedded members
	CMFCMenuBar         m_menuBar;
	CMFCToolBar         m_toolBar;
	CCustomStatusBar    m_statusBar;
	CMFCToolBarImages   m_userImages;
	CStructureView      m_structureView;
	CDesignTreeView		m_designTreeView;
	CSchemaView			m_schemaView;
	CRelationsView		m_relationsView;
	CPropertiesWnd      m_propertiesView;
	CBCFView     m_bcfView;


// Generated message map functions
protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnViewCustomize();
	afx_msg LRESULT OnToolbarCreateNew(WPARAM wp, LPARAM lp);
	afx_msg void OnApplicationLook(UINT id);
	afx_msg void OnUpdateApplicationLook(CCmdUI* pCmdUI);
	DECLARE_MESSAGE_MAP()

	BOOL CreateDockingWindows();
	void SetDockingWindowIcons(BOOL bHiColorIcons);
};
