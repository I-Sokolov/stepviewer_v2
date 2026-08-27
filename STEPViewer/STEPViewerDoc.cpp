
// STEPViewerDoc.cpp : implementation of the CMySTEPViewerDoc class
//

#include "stdafx.h"
// SHARED_HANDLERS can be defined in an ATL project implementing preview, thumbnail
// and search filter handlers and allows sharing of document code with that project.
#ifndef SHARED_HANDLERS
#include "STEPViewer.h"
#endif

#include "STEPViewerDoc.h"
#include <propkey.h>

#include "IDSCheckerDialog.h"

#include "_ap_model_factory.h"
#ifdef _GLTF_SUPPORT
#include "_ap2gltf.h"
#include "_ap2glb.h"
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// ************************************************************************************************
/*virtual*/ void CMySTEPViewerDoc::saveInstance(_instance* pInstance) /*override*/
{
	if (pInstance == nullptr) {
		return;
	}

	CString strValidFileName = validateFileName(dynamic_cast<_ap_instance*>(pInstance)->getName()).c_str();

#ifdef _GLTF_SUPPORT
	TCHAR szFilters[] = _T("BIN Files (*.bin)|*.bin|TTL Files (*.ttl)|*.ttl|glTF Files (*.gltf)|*.gltf|glTF Binary Files (*.glb)|*.glb||");
#else
	TCHAR szFilters[] = _T("BIN Files (*.bin)|*.bin||");
#endif
	CFileDialog dlgFile(FALSE, _T("bin"), strValidFileName,
		OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY, szFilters);

	if (dlgFile.DoModal() != IDOK) {
		return;
	}

	auto pAPInstance = dynamic_cast<_ap_instance*>(pInstance);
	if (pAPInstance == nullptr) {
		ASSERT(FALSE);
		return;
	}

	if (getModels().size() != 1) {
		ASSERT(FALSE);
		return;
	}

#ifdef _GLTF_SUPPORT
	fs::path pathModel = (LPCWSTR)dlgFile.GetPathName();
	string strExtension = pathModel.extension().string();
	std::transform(strExtension.begin(), strExtension.end(), strExtension.begin(), ::tolower);

	if (strExtension == ".gltf") {
		_c_log log(nullptr);

		_ap2gltf::_exporter exporter(getModels()[0], (LPCSTR)CW2A(dlgFile.GetPathName()), true);
		exporter.setLog(&log);
		exporter.writeModelMetadataJSON() = false;
		exporter.execute({ pAPInstance->getSdaiInstance() });
	}
	else if (strExtension == ".glb") {
		_c_log log(nullptr);

		_ap2glb::_exporter exporter(getModels()[0], (LPCSTR)CW2A(dlgFile.GetPathName()));
		exporter.setLog(&log);
		exporter.writeModelMetadataJSON() = false;
		exporter.execute({ pAPInstance->getSdaiInstance() });
	}
	else if ((strExtension == ".bin") || (strExtension == ".ttl")) {
		auto pModel = getOwlModelByInstance(pAPInstance->getOwlModel());
		_ptr<_ifc_model> ifcModel(pModel, false);
		if (ifcModel) {
			OwlInstance owlMapConversionTransformationInstance = ifcModel->createMapConversionTransformation();
			pAPInstance->saveInstance((LPCWSTR)dlgFile.GetPathName(), owlMapConversionTransformationInstance);
			if (owlMapConversionTransformationInstance != 0) {
				RemoveInstance(owlMapConversionTransformationInstance);
			}
		}
		else {
			pAPInstance->saveInstance((LPCWSTR)dlgFile.GetPathName());
		}
	}
	else {
		ASSERT(FALSE);
	}
#else
	pAPInstance->saveInstance((LPCWSTR)dlgFile.GetPathName());
#endif
}

void CMySTEPViewerDoc::OpenModels(const vector<CString>& vecPaths)
{
	setModel(nullptr);

	if ((vecPaths.size() == 1) && m_wndBCFProjectDlg.IsBCF(vecPaths[0])) {
		m_wndBCFProjectDlg.Open(vecPaths[0]);

		// Title
		CString strTitle = AfxGetAppName();
		strTitle += L" - ";
		strTitle += vecPaths[0];
		AfxGetMainWnd()->SetWindowTextW(strTitle);

		// MRU
		AfxGetApp()->AddToRecentFileList(vecPaths[0]);
		return;
	}

	m_wndBCFProjectDlg.Close();

	vector<_model*> vecModels;

	if (vecPaths.size() == 1) {
		fs::path pathModel = (LPCWSTR)vecPaths[0];
		string strExtension = pathModel.extension().string();
		std::transform(strExtension.begin(), strExtension.end(), strExtension.begin(), ::tolower);

		if (strExtension == ".ifczip") {
			vecModels = _ap_model_factory::loadIFCZIP(this, (LPCWSTR)vecPaths[0]);
		}
		else if (strExtension == ".stpz") {
			vecModels = _ap_model_factory::loadSTEPGZip(this, (LPCWSTR)vecPaths[0]);
		}
		else {
			auto pModel = _ap_model_factory::load(this, (LPCWSTR)vecPaths[0], false, nullptr, false);
			if (pModel) {
				vecModels.push_back(pModel);
			}
		}

		if (!vecModels.empty()) {
			setModels(vecModels);
		}

		// Title
		CString strTitle = AfxGetAppName();
		strTitle += L" - ";
		strTitle += vecPaths[0];
		AfxGetMainWnd()->SetWindowTextW(strTitle);

		// MRU
		AfxGetApp()->AddToRecentFileList(vecPaths[0]);
		return;
	}

	vector<CString> vecIfcPaths;
	for (auto strPath : vecPaths) {
		fs::path pathModel = (LPCWSTR)strPath;
		string strExtension = pathModel.extension().string();
		std::transform(strExtension.begin(), strExtension.end(), strExtension.begin(), ::tolower);

		if (strExtension == ".ifc") {
			vecIfcPaths.push_back(strPath);
		}
	}

	if (vecIfcPaths.empty()) {
		AfxMessageBox(L"No IFC files found in the selection.", MB_ICONERROR | MB_OK);
		return;
	}

	for (auto strPath : vecIfcPaths) {
		auto pModel = _ap_model_factory::load(this, strPath, vecIfcPaths.size() > 1, !vecModels.empty() ? vecModels.front() : nullptr, false);
		vecModels.push_back(pModel);
	}

	if (!vecModels.empty()) {
		setModels(vecModels);

		// Title
		CString strTitle = AfxGetAppName();
		strTitle += L" - ";
		strTitle += vecIfcPaths[0];
		strTitle += vecModels.size() > 1 ? L", ..." : L"";
		AfxGetMainWnd()->SetWindowTextW(strTitle);

		// MRU
		AfxGetApp()->AddToRecentFileList(vecIfcPaths[0]);
	}
}


// ************************************************************************************************
// CMySTEPViewerDoc

IMPLEMENT_DYNCREATE(CMySTEPViewerDoc, CDocument)

BEGIN_MESSAGE_MAP(CMySTEPViewerDoc, CDocument)
	ON_COMMAND(ID_FILE_OPEN, &CMySTEPViewerDoc::OnFileOpen)
	ON_COMMAND(ID_VIEW_ZOOM_OUT, &CMySTEPViewerDoc::OnViewZoomOut)
	ON_COMMAND(ID_VIEW_MODEL_CHECKER, &CMySTEPViewerDoc::OnViewModelChecker)
	ON_UPDATE_COMMAND_UI(ID_VIEW_MODEL_CHECKER, &CMySTEPViewerDoc::OnUpdateViewModelChecker)
	ON_COMMAND(ID_FILE_SAVE, &CMySTEPViewerDoc::OnFileSave)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE, &CMySTEPViewerDoc::OnUpdateFileSave)
	ON_COMMAND(ID_FILE_SAVE_AS, &CMySTEPViewerDoc::OnFileSaveAs)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_AS, &CMySTEPViewerDoc::OnUpdateFileSaveAs)
	ON_COMMAND(ID_BCF_ADDBIM, &CMySTEPViewerDoc::OnBcfAddbim)
	ON_UPDATE_COMMAND_UI(ID_BCF_ADDBIM, &CMySTEPViewerDoc::OnUpdateBcfAddbim)
	ON_COMMAND(ID_BCF_NEW, &CMySTEPViewerDoc::OnBcfNew)
	ON_UPDATE_COMMAND_UI(ID_BCF_NEW, &CMySTEPViewerDoc::OnUpdateBcfNew)
	ON_COMMAND(ID_BCF_OPEN, &CMySTEPViewerDoc::OnBcfOpen)
	ON_UPDATE_COMMAND_UI(ID_BCF_OPEN, &CMySTEPViewerDoc::OnUpdateBcfOpen)
	ON_COMMAND(ID_EXPORT_AS_GLTF, &CMySTEPViewerDoc::OnExportAsGltf)
	ON_UPDATE_COMMAND_UI(ID_EXPORT_AS_GLTF, &CMySTEPViewerDoc::OnUpdateExportAsGltf)
	ON_COMMAND(ID_VIEW_IDS_CHECKER, &CMySTEPViewerDoc::OnViewIdsChecker)
	ON_UPDATE_COMMAND_UI(ID_VIEW_IDS_CHECKER, &CMySTEPViewerDoc::OnUpdateViewIdsChecker)
	ON_COMMAND(ID_EXPORT_AS_GLTF_BINARY, &CMySTEPViewerDoc::OnExportAsGltfBinary)
	ON_UPDATE_COMMAND_UI(ID_EXPORT_AS_GLTF_BINARY, &CMySTEPViewerDoc::OnUpdateExportAsGltfBinary)
END_MESSAGE_MAP()


// CMySTEPViewerDoc construction/destruction

CMySTEPViewerDoc::CMySTEPViewerDoc()
	: m_wndBCFProjectDlg(*this)
{}

CMySTEPViewerDoc::~CMySTEPViewerDoc()
{}

BOOL CMySTEPViewerDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	setModel(nullptr);
	m_wndBCFProjectDlg.Close();

	return TRUE;
}

BOOL CMySTEPViewerDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	if (!CDocument::OnOpenDocument(lpszPathName))
		return FALSE;

	if (m_wndBCFProjectDlg.IsBCF(lpszPathName)) {
		m_wndBCFProjectDlg.Open(lpszPathName);

		return TRUE;
	}

	fs::path pathModel = lpszPathName;
	string strExtension = pathModel.extension().string();
	std::transform(strExtension.begin(), strExtension.end(), strExtension.begin(), ::tolower);

	if (strExtension == ".ifczip") {
		auto vecModels = _ap_model_factory::loadIFCZIP(this, lpszPathName);
		setModels(vecModels);
	}
	else if (strExtension == ".stpz") {
		auto vecModels = _ap_model_factory::loadSTEPGZip(this, lpszPathName);
		setModels(vecModels);
	}
	else {
		auto pModel = _ap_model_factory::load(this, lpszPathName, false, nullptr, false);
		setModel(pModel);
	}

	m_wndBCFProjectDlg.Close();

	// Title
	CString strTitle = AfxGetAppName();
	strTitle += L" - ";
	strTitle += lpszPathName;
	AfxGetMainWnd()->SetWindowTextW(strTitle);

	// MRU
	AfxGetApp()->AddToRecentFileList(lpszPathName);

	return TRUE;
}

// CMySTEPViewerDoc serialization

void CMySTEPViewerDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring()) {
		// TODO: add storing code here
	}
	else {
		// TODO: add loading code here
	}
}

#ifdef SHARED_HANDLERS

// Support for thumbnails
void CMySTEPViewerDoc::OnDrawThumbnail(CDC& dc, LPRECT lprcBounds)
{
	// Modify this code to draw the document's data
	dc.FillSolidRect(lprcBounds, RGB(255, 255, 255));

	CString strText = _T("TODO: implement thumbnail drawing here");
	LOGFONT lf;

	CFont* pDefaultGUIFont = CFont::FromHandle((HFONT)GetStockObject(DEFAULT_GUI_FONT));
	pDefaultGUIFont->GetLogFont(&lf);
	lf.lfHeight = 36;

	CFont fontDraw;
	fontDraw.CreateFontIndirect(&lf);

	CFont* pOldFont = dc.SelectObject(&fontDraw);
	dc.DrawText(strText, lprcBounds, DT_CENTER | DT_WORDBREAK);
	dc.SelectObject(pOldFont);
}

// Support for Search Handlers
void CMySTEPViewerDoc::InitializeSearchContent()
{
	CString strSearchContent;
	// Set search contents from document's data. 
	// The content parts should be separated by ";"

	// For example:  strSearchContent = _T("point;rectangle;circle;ole object;");
	SetSearchContent(strSearchContent);
}

void CMySTEPViewerDoc::SetSearchContent(const CString& value)
{
	if (value.IsEmpty()) {
		RemoveChunk(PKEY_Search_Contents.fmtid, PKEY_Search_Contents.pid);
	}
	else {
		CMFCFilterChunkValueImpl* pChunk = nullptr;
		ATLTRY(pChunk = new CMFCFilterChunkValueImpl);
		if (pChunk != nullptr) {
			pChunk->SetTextValue(PKEY_Search_Contents, value, CHUNK_TEXT);
			SetChunkValue(pChunk);
		}
	}
}

#endif // SHARED_HANDLERS

// CMySTEPViewerDoc diagnostics

#ifdef _DEBUG
void CMySTEPViewerDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CMySTEPViewerDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


// CMySTEPViewerDoc commands

void CMySTEPViewerDoc::OnFileOpen()
{
	CFileDialog dlgFile(TRUE, nullptr, _T(""), OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY | OFN_ALLOWMULTISELECT, DESIGN_MODELS_FILTER);
	if (dlgFile.DoModal() != IDOK) {
		return;
	}

	vector<CString> vecModels;
	POSITION pos(dlgFile.GetStartPosition());
	while (pos != nullptr) {
		CString strFileName = dlgFile.GetNextPathName(pos);
		vecModels.push_back(strFileName);

		// MRU
		AfxGetApp()->AddToRecentFileList(vecModels[0]);
	}

	OpenModels(vecModels);

	// Title
	CString strTitle = AfxGetAppName();
	strTitle += L" - ";
	strTitle += vecModels[0];
	strTitle += vecModels.size() > 1 ? L", ..." : L"";
	AfxGetMainWnd()->SetWindowTextW(strTitle);
}

void CMySTEPViewerDoc::OnViewZoomOut()
{
	zoomOut();
}

void CMySTEPViewerDoc::DeleteContents()
{
	m_wndModelChecker.Hide(true);
	__super::DeleteContents();
}

void CMySTEPViewerDoc::OnCloseDocument()
{
	m_wndBCFProjectDlg.Close();

	__super::OnCloseDocument();
}

BOOL CMySTEPViewerDoc::SaveModified()
{
	if (!m_wndBCFProjectDlg.SaveModified()) {
		return FALSE;
	}

	return __super::SaveModified();
}

void CMySTEPViewerDoc::OnViewModelChecker()
{
	if (m_wndModelChecker.IsVisible()) {
		m_wndModelChecker.Hide(false);
	}
	else {
		m_wndModelChecker.Show();
	}
}

void CMySTEPViewerDoc::OnUpdateViewModelChecker(CCmdUI* pCmdUI)
{
	auto visible = m_wndModelChecker.IsVisible();
	pCmdUI->SetCheck(visible);
}

void CMySTEPViewerDoc::OnFileSave()
{
	_ptr<_ap_model> apModel(getModels()[0]);

	CString strFiler;
	CString strExtension;
	if (apModel->getAP() == enumAP::STEP) {
		strFiler = STEP_MODELS_FILTER;
		strExtension = L"ifc";
	}
	else if (apModel->getAP() == enumAP::IFC) {
		strFiler = IFC_MODELS_FILTER;
		strExtension = L"step";
	}
	else  if (apModel->getAP() == enumAP::CIS2) {
		strFiler = CIS2_MODELS_FILTER;
		strExtension = L"stp";
	}
	else {
		ASSERT(FALSE);

		return;
	}

	CFileDialog dlgFile(FALSE, strExtension, apModel->getPath(), OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY, strFiler);
	if (dlgFile.DoModal() != IDOK) {
		return;
	}

	sdaiSaveModelBNUnicode(apModel->getSdaiModel(), (LPCWSTR)dlgFile.GetPathName());
}

void CMySTEPViewerDoc::OnUpdateFileSave(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(getModels().size() == 1);
}

void CMySTEPViewerDoc::OnFileSaveAs()
{
	OnFileSave();
}

void CMySTEPViewerDoc::OnUpdateFileSaveAs(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(getModels().size() == 1);
}

void CMySTEPViewerDoc::OnBcfAddbim()
{
	CFileDialog dlgFile(TRUE, nullptr, _T(""), OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_ALLOWMULTISELECT, BIM_MODELS_FILTER);
	if (dlgFile.DoModal() != IDOK) {
		return;
	}

	POSITION pos(dlgFile.GetStartPosition());
	while (pos != nullptr) {
		CString strPath = dlgFile.GetNextPathName(pos);

		auto pModel = _ap_model_factory::load(this, strPath, false, !getModels().empty() ? getModels()[0] : nullptr, false);
		if (pModel->getAP() != enumAP::IFC) {
			delete pModel;

			continue;
		}

		m_wndBCFProjectDlg.GetActiveTopic()->AddBimFile(ToUTF8(pModel->getPath()).c_str(), false);

		// MRU
		AfxGetApp()->AddToRecentFileList(strPath);
	}
}

void CMySTEPViewerDoc::OnUpdateBcfAddbim(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_wndBCFProjectDlg.GetActiveTopic() != NULL);
}

void CMySTEPViewerDoc::OnBcfNew()
{
	m_wndBCFProjectDlg.Open(NULL);

	setModel(nullptr);
}

void CMySTEPViewerDoc::OnUpdateBcfNew(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!m_wndBCFProjectDlg.GetSafeHwnd() || !m_wndBCFProjectDlg.IsWindowVisible());
}

void CMySTEPViewerDoc::OnBcfOpen()
{
	CFileDialog dlgFile(TRUE, nullptr, _T(""), OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY, BCF_PACKAGES_FILTER);
	if (dlgFile.DoModal() != IDOK) {
		return;
	}

	CString strPath = dlgFile.GetPathName();
	if (m_wndBCFProjectDlg.IsBCF(strPath)) {
		if (!m_wndBCFProjectDlg.SaveModified()) {
			return;
		}
		m_wndBCFProjectDlg.Open(strPath);
	}
}

void CMySTEPViewerDoc::OnUpdateBcfOpen(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(TRUE);
}

void CMySTEPViewerDoc::OnExportAsGltf()
{
#ifdef _GLTF_SUPPORT
	fs::path pthInputFile = getModels()[0]->getPath();

	TCHAR szFilters[] = _T("glTF Files (*.gltf)|*.gltf||");
	CFileDialog dlgFile(FALSE, _T("gltf"), pthInputFile.wstring().c_str(),
		OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY, szFilters);

	if (dlgFile.DoModal() != IDOK) {
		return;
	}

	_c_log log(nullptr);

	_ap2gltf::_exporter exporter(getModels()[0], (LPCSTR)CW2A(dlgFile.GetPathName()), true);
	exporter.setLog(&log);
	exporter.writeModelMetadataJSON() = false;
	exporter.execute();
#endif
}

void CMySTEPViewerDoc::OnUpdateExportAsGltf(CCmdUI* pCmdUI)
{
#ifdef _GLTF_SUPPORT
	pCmdUI->Enable(getModels().size() == 1);
#else
	pCmdUI->Enable(FALSE);
#endif
}

void CMySTEPViewerDoc::OnExportAsGltfBinary()
{
	fs::path pthInputFile = getModels()[0]->getPath();

	TCHAR szFilters[] = _T("glTF Binary Files (*.glb)|*.glb||");
	CFileDialog dlgFile(FALSE, _T("glb"), pthInputFile.wstring().c_str(),
		OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY, szFilters);

	if (dlgFile.DoModal() != IDOK) {
		return;
	}

	_c_log log(nullptr);

	_ap2glb::_exporter exporter(getModels()[0], (LPCSTR)CW2A(dlgFile.GetPathName()));
	exporter.setLog(&log);
	exporter.writeModelMetadataJSON() = false;
	exporter.execute();
}

void CMySTEPViewerDoc::OnUpdateExportAsGltfBinary(CCmdUI* pCmdUI)
{
#ifdef _GLTF_SUPPORT
	pCmdUI->Enable(getModels().size() == 1);
#else
	pCmdUI->Enable(FALSE);
#endif
}

void CMySTEPViewerDoc::OnViewIdsChecker()
{
	CIDSCheckerDialog dlgIDSChecker(getModels()[0], ::AfxGetMainWnd());
	dlgIDSChecker.DoModal();
}

void CMySTEPViewerDoc::OnUpdateViewIdsChecker(CCmdUI* pCmdUI)
{
	BOOL bEnable = FALSE;
	if (getModels().size() == 1) {
		bEnable = _ptr<_ap_model>(getModels()[0])->getAP() == enumAP::IFC;
	}
	pCmdUI->Enable(bEnable);
}