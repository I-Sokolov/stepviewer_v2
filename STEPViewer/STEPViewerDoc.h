
// STEPViewerDoc.h : interface of the CMySTEPViewerDoc class
//


#pragma once

#include "Controller.h"
#include "ModelCheckDlg.h"
#include "BCF\BCFProjectDlg.h"

// ************d************************************************************************************
static TCHAR BCF_PACKAGES_FILTER[] = _T("BCF Packages (*.bcf; *.bcfzip)|*.bcf; *.bcfzip|All Files (*.*)|*.*||");
static TCHAR DESIGN_MODELS_FILTER[] = _T("Design Models (*.stp; *.step; *.stpz; *.ifc; *.ifczip)|*.stp; *.step; *.stpz; *.ifc; *.ifczip|All Files (*.*)|*.*||");
static TCHAR IFC_MODELS_FILTER[] = _T("IFC Models (*.ifc)|*.ifc|All Files (*.*)|*.*||");
static TCHAR BIM_MODELS_FILTER[] = _T("BIM Models (*.ifc)|*.ifc|All Files (*.*)|*.*||");
static TCHAR STEP_MODELS_FILTER[] = _T("STEP Models (*.step)|*.step|All Files (*.*)|*.*||");
static TCHAR CIS2_MODELS_FILTER[] = _T("CIS2 Models (*.stp)|*.stp|All Files (*.*)|*.*||");

// ************************************************************************************************
class CMySTEPViewerDoc
	: public CDocument
	, public CController
{

public: // Methods

	// _controller
	virtual void saveInstance(_instance* pInstance) override;

	void OpenModels(const vector<CString>& vecPaths);

protected: // create from serialization only
	CMySTEPViewerDoc();
	DECLARE_DYNCREATE(CMySTEPViewerDoc)

// Attributes
public:

// Operations
public:

// Overrides
public:
	virtual BOOL OnNewDocument();
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual void Serialize(CArchive& ar);
	virtual void DeleteContents();
	virtual void OnCloseDocument();
	virtual BOOL SaveModified(); // return TRUE if ok to continue
#ifdef SHARED_HANDLERS
	virtual void InitializeSearchContent();
	virtual void OnDrawThumbnail(CDC& dc, LPRECT lprcBounds);
#endif // SHARED_HANDLERS

// Implementation
public:
	virtual ~CMySTEPViewerDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	CModelCheckDlg		m_wndModelChecker;
	CBCFProjectDlg     m_wndBCFProjectDlg;

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()

#ifdef SHARED_HANDLERS
	// Helper function that sets search content for a Search Handler
	void SetSearchContent(const CString& value);
#endif // SHARED_HANDLERS
public:
	afx_msg void OnFileOpen();
	afx_msg void OnViewZoomOut();
	afx_msg void OnViewModelChecker();
	afx_msg void OnUpdateViewModelChecker(CCmdUI* pCmdUI);
	afx_msg void OnFileSave();
	afx_msg void OnUpdateFileSave(CCmdUI* pCmdUI);
	afx_msg void OnFileSaveAs();
	afx_msg void OnUpdateFileSaveAs(CCmdUI* pCmdUI);
	afx_msg void OnBcfAddbim();
	afx_msg void OnUpdateBcfAddbim(CCmdUI* pCmdUI);
	afx_msg void OnBcfNew();
	afx_msg void OnUpdateBcfNew(CCmdUI* pCmdUI);
	afx_msg void OnBcfOpen();
	afx_msg void OnUpdateBcfOpen(CCmdUI* pCmdUI);
	afx_msg void OnExportAsGltf();
	afx_msg void OnUpdateExportAsGltf(CCmdUI* pCmdUI);
	afx_msg void OnViewIdsChecker();
	afx_msg void OnUpdateViewIdsChecker(CCmdUI* pCmdUI);
	afx_msg void OnExportAsGltfBinary();
	afx_msg void OnUpdateExportAsGltfBinary(CCmdUI* pCmdUI);
};
