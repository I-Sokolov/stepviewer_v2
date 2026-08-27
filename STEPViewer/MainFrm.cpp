
// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "STEPViewer.h"
#include "STEPViewerDoc.h"

#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWndEx)

const int  iMaxUserToolbars = 10;
const UINT uiFirstUserToolBarId = AFX_IDW_CONTROLBAR_FIRST + 40;
const UINT uiLastUserToolBarId = uiFirstUserToolBarId + iMaxUserToolbars - 1;

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWndEx)
	ON_WM_CREATE()
	ON_COMMAND(ID_VIEW_CUSTOMIZE, &CMainFrame::OnViewCustomize)
	ON_REGISTERED_MESSAGE(AFX_WM_CREATETOOLBAR, &CMainFrame::OnToolbarCreateNew)
	ON_COMMAND_RANGE(ID_VIEW_APPLOOK_WIN_2000, ID_VIEW_APPLOOK_WINDOWS_7, &CMainFrame::OnApplicationLook)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_APPLOOK_WIN_2000, ID_VIEW_APPLOOK_WINDOWS_7, &CMainFrame::OnUpdateApplicationLook)
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_LOG,
};


// ------------------------------------------------------------------------------------------------
// Controller - MVC
CController* CMainFrame::GetController() const
{
	POSITION posDocTemplate = AfxGetApp()->GetFirstDocTemplatePosition();
	if (posDocTemplate == nullptr)
	{
		return nullptr;
	}

	CDocTemplate * pDocTemplate = AfxGetApp()->GetNextDocTemplate(posDocTemplate);
	if (pDocTemplate == nullptr)
	{
		return nullptr;
	}

	POSITION posDocument = pDocTemplate->GetFirstDocPosition();
	if (posDocument == nullptr)
	{
		return nullptr;
	}

	CDocument * pDocument = pDocTemplate->GetNextDoc(posDocument);
	ASSERT(pDocument);

	return dynamic_cast<CController*>(pDocument);
}

// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	// TODO: add member initialization code here
	theApp.m_nAppLook = theApp.GetInt(_T("ApplicationLook"), ID_VIEW_APPLOOK_VS_2008);
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWndEx::OnCreate(lpCreateStruct) == -1)
		return -1;

	BOOL bNameValid;

	if (!m_menuBar.Create(this))
	{
		ASSERT(FALSE);

		return -1;
	}

	m_menuBar.SetPaneStyle(m_menuBar.GetPaneStyle() | CBRS_SIZE_DYNAMIC | CBRS_TOOLTIPS | CBRS_FLYBY);

	// prevent the menu bar from taking the focus on activation
	CMFCPopupMenu::SetForceMenuFocus(FALSE);

	if (!m_toolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_toolBar.LoadToolBar(theApp.m_bHiColorIcons ? IDR_MAINFRAME_256 : IDR_MAINFRAME))
	{
		ASSERT(FALSE);

		return -1;
	}

	CString strToolBarName;
	bNameValid = strToolBarName.LoadString(IDS_TOOLBAR_STANDARD);
	ASSERT(bNameValid);
	m_toolBar.SetWindowText(strToolBarName);

	CString strCustomize;
	bNameValid = strCustomize.LoadString(IDS_TOOLBAR_CUSTOMIZE);
	ASSERT(bNameValid);
	m_toolBar.EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, strCustomize);

	// Allow user-defined toolbars operations:
	InitUserToolbars(nullptr, uiFirstUserToolBarId, uiLastUserToolBarId);

	if (!m_statusBar.Create(this)) {
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}
	m_statusBar.SetIndicators(indicators, sizeof(indicators) / sizeof(UINT));

	// Status indicator
	int nIndex = m_statusBar.CommandToIndex(ID_SEPARATOR);
	if (nIndex != -1) {
		m_statusBar.SetPaneInfo(nIndex, ID_SEPARATOR, SBPS_NORMAL, 200);
	}

	// Log indicator
	nIndex = m_statusBar.CommandToIndex(ID_INDICATOR_LOG);
	if (nIndex != -1) {
		m_statusBar.SetPaneText(nIndex, _T(""));
		m_statusBar.SetPaneInfo(nIndex, ID_INDICATOR_LOG, SBPS_STRETCH, 400);

		HICON hIcon = (HICON)LoadImage(
			AfxGetResourceHandle(),
			MAKEINTRESOURCE(IDI_ICON_INFO),
			IMAGE_ICON,
			16, 16, LR_DEFAULTCOLOR
		);
		if (hIcon) {
			m_statusBar.SetPaneIcon(nIndex, hIcon);
		}
	}

	// Initialize
	m_statusBar.SetLogHub(GetController()->getLogHub());

	// TODO: Delete these five lines if you don't want the toolbar and menubar to be dockable
	m_menuBar.EnableDocking(CBRS_ALIGN_ANY);
	m_toolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockPane(&m_menuBar);
	DockPane(&m_toolBar);


	// enable Visual Studio 2005 style docking window behavior
	CDockingManager::SetDockingMode(DT_SMART);
	// enable Visual Studio 2005 style docking window auto-hide behavior
	EnableAutoHidePanes(CBRS_ALIGN_ANY);

	// Load menu item image (not placed on any standard toolbars):
	CMFCToolBar::AddToolBarForImageCollection(IDR_MENU_IMAGES, theApp.m_bHiColorIcons ? IDB_MENU_IMAGES_24 : 0);

	// create docking windows
	if (!CreateDockingWindows())
	{
		ASSERT(FALSE);

		return -1;
	}

	m_structureView.EnableDocking(CBRS_ALIGN_ANY);
	m_designTreeView.EnableDocking(CBRS_ALIGN_ANY);
	m_schemaView.EnableDocking(CBRS_ALIGN_ANY);
	m_relationsView.EnableDocking(CBRS_ALIGN_ANY);

	DockPane(&m_structureView);

	CDockablePane* pTabbedBar = nullptr;
	m_designTreeView.AttachToTabWnd(&m_structureView, DM_SHOW, TRUE, &pTabbedBar);
	m_schemaView.AttachToTabWnd(pTabbedBar, DM_SHOW, TRUE, &pTabbedBar);
	m_relationsView.AttachToTabWnd(pTabbedBar, DM_SHOW, TRUE, &pTabbedBar);

	m_propertiesView.EnableDocking(CBRS_ALIGN_ANY);
	DockPane(&m_propertiesView);

	m_bcfView.EnableDocking(CBRS_ALIGN_ANY);
	DockPane(&m_bcfView);

	// set the visual manager and style based on persisted value
	OnApplicationLook(theApp.m_nAppLook);

	// Enable toolbar and docking window menu replacement
	EnablePaneMenu(TRUE, ID_VIEW_CUSTOMIZE, strCustomize, ID_VIEW_TOOLBAR);

	// enable quick (Alt+drag) toolbar customization
	CMFCToolBar::EnableQuickCustomization();

	if (CMFCToolBar::GetUserImages() == nullptr)
	{
		// load user-defined toolbar images
		if (m_userImages.Load(_T(".\\UserImages.bmp")))
		{
			CMFCToolBar::SetUserImages(&m_userImages);
		}
	}

	// enable menu personalization (most-recently used commands)
	// TODO: define your own basic commands, ensuring that each pulldown menu has at least one basic command.
	CList<UINT, UINT> lstBasicCommands;

	lstBasicCommands.AddTail(ID_FILE_NEW);
	lstBasicCommands.AddTail(ID_FILE_OPEN);
	lstBasicCommands.AddTail(ID_FILE_SAVE);
	lstBasicCommands.AddTail(ID_FILE_SAVE_AS);
	lstBasicCommands.AddTail(ID_EXPORT_AS_GLTF);
	lstBasicCommands.AddTail(ID_EXPORT_AS_GLTF_BINARY);
	lstBasicCommands.AddTail(ID_BCF_NEW);
	lstBasicCommands.AddTail(ID_BCF_OPEN);
	lstBasicCommands.AddTail(ID_BCF_ADDBIM);
	lstBasicCommands.AddTail(ID_APP_EXIT);
	lstBasicCommands.AddTail(ID_EDIT_CUT);
	lstBasicCommands.AddTail(ID_EDIT_PASTE);
	lstBasicCommands.AddTail(ID_EDIT_UNDO);
	lstBasicCommands.AddTail(ID_APP_ABOUT);
	lstBasicCommands.AddTail(ID_VIEW_STATUS_BAR);
	lstBasicCommands.AddTail(ID_VIEW_TOOLBAR);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_OFF_2003);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_VS_2005);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_OFF_2007_BLUE);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_OFF_2007_SILVER);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_OFF_2007_BLACK);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_OFF_2007_AQUA);
	lstBasicCommands.AddTail(ID_VIEW_APPLOOK_WINDOWS_7);
	lstBasicCommands.AddTail(ID_SORTING_SORTALPHABETIC);
	lstBasicCommands.AddTail(ID_SORTING_SORTBYTYPE);
	lstBasicCommands.AddTail(ID_SORTING_SORTBYACCESS);
	lstBasicCommands.AddTail(ID_SORTING_GROUPBYTYPE);

	// Projection
	lstBasicCommands.AddTail(ID_PROJECTION_PERSPECTIVE);
	lstBasicCommands.AddTail(ID_PROJECTION_ORTHOGRAPHIC);

	// View	
	lstBasicCommands.AddTail(ID_VIEW_TOP);
	lstBasicCommands.AddTail(ID_VIEW_LEFT);
	lstBasicCommands.AddTail(ID_VIEW_RIGHT);
	lstBasicCommands.AddTail(ID_VIEW_BOTTOM);
	lstBasicCommands.AddTail(ID_VIEW_FRONT);
	lstBasicCommands.AddTail(ID_VIEW_BACK);
	lstBasicCommands.AddTail(ID_VIEW_ISOMETRIC);
	lstBasicCommands.AddTail(ID_VIEW_ZOOM_OUT);
	lstBasicCommands.AddTail(ID_VIEW_RESET);
	lstBasicCommands.AddTail(ID_VIEW_CENTER_ALL_GEOMETRY);
	lstBasicCommands.AddTail(ID_VIEW_MODEL_CHECKER);
	lstBasicCommands.AddTail(ID_VIEW_IDS_CHECKER);

	// Show
	lstBasicCommands.AddTail(ID_SHOW_FACES);
	lstBasicCommands.AddTail(ID_SHOW_CONC_FACES_WIREFRAMES);
	lstBasicCommands.AddTail(ID_SHOW_LINES);
	lstBasicCommands.AddTail(ID_SHOW_POINTS);

	CMFCToolBar::SetBasicCommands(lstBasicCommands);

	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWndEx::PreCreateWindow(cs) )
		return FALSE;
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	cs.style = WS_OVERLAPPED | WS_CAPTION | FWS_ADDTOTITLE
		 | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_MAXIMIZE | WS_SYSMENU;

	return TRUE;
}

BOOL CMainFrame::CreateDockingWindows()
{
	CController* pController = GetController();

	BOOL bNameValid;

	// ********************************************************************************************
	// Model Structure View
	m_structureView.setController(pController);
	
	CString strCaption;
	bNameValid = strCaption.LoadString(IDS_FILE_VIEW);
	ASSERT(bNameValid);
	if (!m_structureView.Create(
		strCaption, 
		this, 
		CRect(0, 0, 200, 200), 
		TRUE, 
		ID_VIEW_FILEVIEW, 
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_LEFT| CBRS_FLOAT_MULTI))
	{
		ASSERT(FALSE);

		return FALSE;
	}
	// ********************************************************************************************

	// ********************************************************************************************
	// Design Tree View
	m_designTreeView.setController(pController);

	bNameValid = strCaption.LoadString(IDS_DESIGN_VIEW);
	ASSERT(bNameValid);
	if (!m_designTreeView.Create(
		strCaption,
		this,
		CRect(0, 0, 200, 200),
		TRUE,
		ID_VIEW_DESIGN,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_LEFT | CBRS_FLOAT_MULTI))
	{
		ASSERT(FALSE);

		return FALSE;
	}
	// ********************************************************************************************

	// ********************************************************************************************
	// Schema View
	m_schemaView.setController(pController);

	bNameValid = strCaption.LoadString(IDS_SCHEMA_VIEW);
	ASSERT(bNameValid);
	if (!m_schemaView.Create(
		strCaption, 
		this, 
		CRect(0, 0, 200, 200), 
		TRUE, 
		ID_VIEW_SCHEMA, 
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_LEFT | CBRS_FLOAT_MULTI))
	{
		ASSERT(FALSE);

		return FALSE;
	}
	// ********************************************************************************************

	// ********************************************************************************************
	// Relations/Attributes View
	m_relationsView.setController(pController);

	bNameValid = strCaption.LoadString(IDS_IFC_RELATIONS_VIEW);
	ASSERT(bNameValid);

	if (!m_relationsView.Create(
		strCaption, 
		this, 
		CRect(0, 0, 200, 200), 
		TRUE, 
		ID_VIEW_RELATIONS,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_LEFT | CBRS_FLOAT_MULTI))
	{
		ASSERT(FALSE);

		return FALSE;
	}
	// ********************************************************************************************

	// ********************************************************************************************
	// Create IFC Instance Properties View
	m_propertiesView.setController(pController);
	
	bNameValid = strCaption.LoadString(IDS_PROPERTIES_WND);
	ASSERT(bNameValid);

	if (!m_propertiesView.Create(
		strCaption, 
		this, 
		CRect(0, 0, 200, 200), 
		TRUE, 
		ID_VIEW_PROPERTIESWND, 
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_RIGHT | CBRS_FLOAT_MULTI))
	{
		ASSERT(FALSE);

		return FALSE;
	}
	// ********************************************************************************************

	m_bcfView.SetDocument(dynamic_cast<CMySTEPViewerDoc*>(pController));
	if (!m_bcfView.Create(
		L"BCF View",
		this,
		CRect(0, 0, 640, 600),
		TRUE,
		ID_VIEW_BCF_PROJECT_PANE,
		WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_RIGHT | CBRS_FLOAT_MULTI))
	{
		ASSERT(FALSE);
		return FALSE;
	}

	SetDockingWindowIcons(theApp.m_bHiColorIcons);
	return TRUE;
}

void CMainFrame::SetDockingWindowIcons(BOOL bHiColorIcons)
{
	HICON hModelStructureViewIcon = (HICON)::LoadImage(
		::AfxGetResourceHandle(), 
		MAKEINTRESOURCE(bHiColorIcons ? IDI_FILE_VIEW_HC : IDI_FILE_VIEW), 
		IMAGE_ICON, 
		::GetSystemMetrics(SM_CXSMICON), 
		::GetSystemMetrics(SM_CYSMICON), 
		0);
	m_structureView.SetIcon(hModelStructureViewIcon, FALSE);

	HICON hDesignTreeViewIcon = (HICON)::LoadImage(
		::AfxGetResourceHandle(),
		MAKEINTRESOURCE(bHiColorIcons ? IDI_CLASS_VIEW_HC : IDI_CLASS_VIEW),
		IMAGE_ICON,
		::GetSystemMetrics(SM_CXSMICON),
		::GetSystemMetrics(SM_CYSMICON),
		0);
	m_designTreeView.SetIcon(hDesignTreeViewIcon, FALSE);

	HICON hSchemaViewIcon = (HICON)::LoadImage(
		::AfxGetResourceHandle(), 
		MAKEINTRESOURCE(bHiColorIcons ? IDI_CLASS_VIEW_HC : IDI_CLASS_VIEW), 
		IMAGE_ICON, 
		::GetSystemMetrics(SM_CXSMICON), 
		::GetSystemMetrics(SM_CYSMICON), 
		0);
	m_schemaView.SetIcon(hSchemaViewIcon, FALSE);

	HICON hRelationsViewIcon = (HICON)::LoadImage(
		::AfxGetResourceHandle(), 
		MAKEINTRESOURCE(bHiColorIcons ? IDI_CLASS_VIEW_HC : IDI_CLASS_VIEW), 
		IMAGE_ICON, 
		::GetSystemMetrics(SM_CXSMICON), 
		::GetSystemMetrics(SM_CYSMICON), 
		0);
	m_relationsView.SetIcon(hRelationsViewIcon, FALSE);

	HICON hPropertiesViewIcon = (HICON)::LoadImage(
		::AfxGetResourceHandle(), 
		MAKEINTRESOURCE(bHiColorIcons ? IDI_PROPERTIES_WND_HC : IDI_PROPERTIES_WND), 
		IMAGE_ICON, 
		::GetSystemMetrics(SM_CXSMICON), 
		::GetSystemMetrics(SM_CYSMICON), 
		0);
	m_propertiesView.SetIcon(hPropertiesViewIcon, FALSE);

	HICON hBCFViewIcon = (HICON)::LoadImage(
		::AfxGetResourceHandle(),
		MAKEINTRESOURCE(IDI_BCF_VIEW),
		IMAGE_ICON,
		::GetSystemMetrics(SM_CXSMICON),
		::GetSystemMetrics(SM_CYSMICON),
		0);
	m_bcfView.SetIcon(hBCFViewIcon, FALSE);
}

// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWndEx::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWndEx::Dump(dc);
}
#endif //_DEBUG


// CMainFrame message handlers

void CMainFrame::OnViewCustomize()
{
	CMFCToolBarsCustomizeDialog* pDlgCust = new CMFCToolBarsCustomizeDialog(this, TRUE /* scan menus */);
	pDlgCust->EnableUserDefinedToolbars();
	pDlgCust->Create();
}

LRESULT CMainFrame::OnToolbarCreateNew(WPARAM wp,LPARAM lp)
{
	LRESULT lres = CFrameWndEx::OnToolbarCreateNew(wp,lp);
	if (lres == 0)
	{
		return 0;
	}

	CMFCToolBar* pUserToolbar = (CMFCToolBar*)lres;
	ASSERT_VALID(pUserToolbar);

	BOOL bNameValid;
	CString strCustomize;
	bNameValid = strCustomize.LoadString(IDS_TOOLBAR_CUSTOMIZE);
	ASSERT(bNameValid);

	pUserToolbar->EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, strCustomize);
	return lres;
}

void CMainFrame::OnApplicationLook(UINT id)
{
	CWaitCursor wait;

	theApp.m_nAppLook = id;

	switch (theApp.m_nAppLook)
	{
	case ID_VIEW_APPLOOK_WIN_2000:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManager));
		break;

	case ID_VIEW_APPLOOK_OFF_XP:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOfficeXP));
		break;

	case ID_VIEW_APPLOOK_WIN_XP:
		CMFCVisualManagerWindows::m_b3DTabsXPTheme = TRUE;
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));
		break;

	case ID_VIEW_APPLOOK_OFF_2003:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOffice2003));
		CDockingManager::SetDockingMode(DT_SMART);
		break;

	case ID_VIEW_APPLOOK_VS_2005:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerVS2005));
		CDockingManager::SetDockingMode(DT_SMART);
		break;

	case ID_VIEW_APPLOOK_VS_2008:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerVS2008));
		CDockingManager::SetDockingMode(DT_SMART);
		break;

	case ID_VIEW_APPLOOK_WINDOWS_7:
		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows7));
		CDockingManager::SetDockingMode(DT_SMART);
		break;

	default:
		switch (theApp.m_nAppLook)
		{
		case ID_VIEW_APPLOOK_OFF_2007_BLUE:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_LunaBlue);
			break;

		case ID_VIEW_APPLOOK_OFF_2007_BLACK:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_ObsidianBlack);
			break;

		case ID_VIEW_APPLOOK_OFF_2007_SILVER:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_Silver);
			break;

		case ID_VIEW_APPLOOK_OFF_2007_AQUA:
			CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_Aqua);
			break;
		}

		CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOffice2007));
		CDockingManager::SetDockingMode(DT_SMART);
	}

	RedrawWindow(nullptr, nullptr, RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_UPDATENOW | RDW_FRAME | RDW_ERASE);

	theApp.WriteInt(_T("ApplicationLook"), theApp.m_nAppLook);
}

void CMainFrame::OnUpdateApplicationLook(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(theApp.m_nAppLook == pCmdUI->m_nID);
}

BOOL CMainFrame::LoadFrame(UINT nIDResource, DWORD dwDefaultStyle, CWnd* pParentWnd, CCreateContext* pContext) 
{
	// base class does the real work

	if (!CFrameWndEx::LoadFrame(nIDResource, dwDefaultStyle, pParentWnd, pContext))
	{
		return FALSE;
	}

	// enable customization button for all user toolbars
	BOOL bNameValid;
	CString strCustomize;
	bNameValid = strCustomize.LoadString(IDS_TOOLBAR_CUSTOMIZE);
	ASSERT(bNameValid);

	for (int i = 0; i < iMaxUserToolbars; i ++)
	{
		CMFCToolBar* pUserToolbar = GetUserToolBarByIndex(i);
		if (pUserToolbar != nullptr)
		{
			pUserToolbar->EnableCustomizeButton(TRUE, ID_VIEW_CUSTOMIZE, strCustomize);
		}
	}

	return TRUE;
}
