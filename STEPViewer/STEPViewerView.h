
// STEPViewerView.h : interface of the CMySTEPViewerView class
//

#pragma once

#include "bcfAPI.h"
#include "_mvc.h"
#include "_oglUtils.h"
#include "Controller.h"

// ************************************************************************************************
class CMySTEPViewerView 
	: public CView
	, public _view
{

private: // Members

	_oglView* m_pOpenGLView;

private: // Methods

	CController* getController();

public: // Methods

	// _view
	virtual void onModelLoaded();

protected: // create from serialization only
	CMySTEPViewerView();
	DECLARE_DYNCREATE(CMySTEPViewerView)

// Attributes
public:
	CMySTEPViewerDoc* GetDocument() const;

// Operations
public:
	void SetBCFView(BCFCamera camera, BCFPoint& viewPoint, BCFPoint& direction, BCFPoint& upVector, double viewToWorldScale, double fieldOfView, double aspectRatio);
	void ResetBCFView();
	bool GetBCFView(BCFCamera& camera, BCFPoint& viewPoint, BCFPoint& direction, BCFPoint& upVector, double& viewToWorldScale, double& fieldOfView, double& aspectRatio);

// Overrides
public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);

// Implementation
public:
	virtual ~CMySTEPViewerView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	afx_msg void OnFilePrintPreview();
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	DECLARE_MESSAGE_MAP()
public:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnDropFiles(HDROP hDropInfo);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnProjectionOrthographic();
	afx_msg void OnUpdateProjectionOrthographic(CCmdUI* pCmdUI);
	afx_msg void OnProjectionPerspective();
	afx_msg void OnUpdateProjectionPerspective(CCmdUI* pCmdUI);
	afx_msg void OnViewFront();
	afx_msg void OnViewTop();
	afx_msg void OnViewRight();
	afx_msg void OnViewBack();
	afx_msg void OnViewLeft();
	afx_msg void OnViewBottom();
	afx_msg void OnViewIsometric();
	afx_msg void OnInstancesSave();
	afx_msg void OnUpdateInstancesSave(CCmdUI* pCmdUI);
	afx_msg void OnInstancesEnable();
	afx_msg void OnUpdateInstancesEnable(CCmdUI* pCmdUI);
	afx_msg void OnOverrideMaterial();
	afx_msg void OnUpdateOverrideMaterial(CCmdUI* pCmdUI);
	afx_msg void OnResetMaterials();
	afx_msg void OnUpdateResetMaterials(CCmdUI* pCmdUI);
	afx_msg void OnInstancesZoomTo();
	afx_msg void OnUpdateInstancesZoomTo(CCmdUI* pCmdUI);
	afx_msg void OnShowFaces();
	afx_msg void OnUpdateShowFaces(CCmdUI* pCmdUI);
	afx_msg void OnShowConcFacesWireframes();
	afx_msg void OnUpdateShowConcFacesWireframes(CCmdUI* pCmdUI);
	afx_msg void OnShowLines();
	afx_msg void OnUpdateShowLines(CCmdUI* pCmdUI);
	afx_msg void OnShowPoints();
	afx_msg void OnUpdateShowPoints(CCmdUI* pCmdUI);
	afx_msg void OnViewReset();
};

#ifndef _DEBUG  // debug version in STEPViewerView.cpp
inline CMySTEPViewerDoc* CMySTEPViewerView::GetDocument() const
   { return reinterpret_cast<CMySTEPViewerDoc*>(m_pDocument); }
#endif

void SetBCFView(
	BCFCamera camera,
	BCFPoint& viewPoint,
	BCFPoint& direction,
	BCFPoint& upVector,
	double viewToWorldScale,
	double fieldOfView,
	double aspectRatio);
void ResetBCFView();
void GetBCFView(BCFCamera& camera, BCFPoint& viewPoint, BCFPoint& direction, BCFPoint& upVector, double& viewToWorldScale, double& fieldOfView, double& aspectRatio);