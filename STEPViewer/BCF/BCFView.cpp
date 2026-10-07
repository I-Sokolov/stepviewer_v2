#include "stdafx.h"

#include "BCFViewPointMgr.h"
#include "BCFCommentForm.h"
#include "BCFProjectSettingsDlg.h"
#include "BCFProjectForm.h"
#include "BCFTopicForm.h"
#include "BCFView.h"
#include "BCFViewControls.h"
#include "UriDownloader.h"
#include "STEPViewerDoc.h"
#include "Resource.h"
#include "_ap_model_factory.h"
#include "_ptr.h"

#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;

BEGIN_MESSAGE_MAP(CBCFView, CDockablePane)
	ON_WM_CREATE()
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
	ON_BN_CLICKED(IDC_PANE_PROJECT_SETTINGS, &CBCFView::OnProjectSettings)
	ON_UPDATE_COMMAND_UI(IDC_PANE_PROJECT_SETTINGS, &CBCFView::OnUpdateProjectSettings)
END_MESSAGE_MAP()

CBCFView::CBCFView()
	: m_stepViewerDoc(nullptr)
	, m_project(nullptr)
	, m_activeForm(ProjectForm)
	, m_projectId(new CBCFEdit)
	, m_projectName(new CBCFEdit)
	, m_projectForm(new CBCFProjectForm)
	, m_topicForm(new CBCFTopicForm)
	, m_commentForm(new CBCFCommentForm)
{
}

CBCFView::~CBCFView()
{
	ReleaseProject();
	delete m_commentForm;
	delete m_topicForm;
	delete m_projectForm;
	delete m_projectName;
	delete m_projectId;
}

int CBCFView::OnCreate(LPCREATESTRUCT createStruct)
{
	if (CDockablePane::OnCreate(createStruct) == -1) {
		return -1;
	}

	SetFont(m_dialogFont.CreatePointFont(80, L"MS Shell Dlg")
		? &m_dialogFont
		: &afxGlobalData.fontRegular);

	if (!m_emptyMessage.Create(L"Use File menu to open or create BCF file",
			WS_CHILD | SS_CENTER | SS_CENTERIMAGE, CRect(), this,
			IDC_PANE_EMPTY_MESSAGE)) {
		return -1;
	}
	SetBCFControlFont(m_emptyMessage, this);
	CreateBCFStaticLabel(m_projectIdLabel, L"BCF Project Id:", this);
	CreateBCFStaticLabel(m_projectNameLabel, L"Name:", this);
	m_projectSettings.Create(L"Settings",
		WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_TABSTOP | BS_PUSHBUTTON,
		CRect(), this, IDC_PANE_PROJECT_SETTINGS);
	SetBCFControlFont(m_projectSettings, this);
	
	m_projectId->Create(WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_READONLY,
		CRect(), this, IDC_PANE_PROJECT_ID);
	m_projectName->Create(WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
		CRect(), this, IDC_PANE_PROJECT_NAME);

	SetBCFControlFont(*m_projectId, this);
	SetBCFControlFont(*m_projectName, this);
	
	if (!m_projectForm->Create(this) || !m_topicForm->Create(this) || !m_commentForm->Create(this)) {
		return -1;
	}
	
	LoadProjectInfo();
	ShowForm(ProjectForm);
	return 0;
}

BOOL CBCFView::OnCommand(WPARAM wParam, LPARAM lParam)
{
	const BOOL handled = CDockablePane::OnCommand(wParam, lParam);
	if (HIWORD(wParam) == EN_KILLFOCUS &&
		LOWORD(wParam) == IDC_PANE_PROJECT_NAME &&
		m_projectName->GetModify()) {
		if (CommitProjectInfo()) {
			m_projectName->SetModify(FALSE);
		}
	}
	return handled;
}

BOOL CBCFView::OnEraseBkgnd(CDC* dc)
{
	CRect client;
	GetClientRect(client);
	dc->FillRect(client, CBrush::FromHandle(::GetSysColorBrush(COLOR_3DFACE)));
	return TRUE;
}

void CBCFView::Activate()
{
	ShowPane(TRUE, FALSE, TRUE);
	SetFocus();
}

void CBCFView::NewProject()
{
	if (!CloseActiveProjectForOperation(L"create a new BCF project")) {
		return;
	}

	m_project = BCFProject::Create();
	if (!m_project) {
		AfxMessageBox(L"Failed to initialize BCF project.", MB_OK | MB_ICONERROR);
		return;
	}

	m_email = AfxGetApp()->GetProfileString(L"BCF", L"User");
	m_project->SetOptions(ToUTF8(m_email).c_str(), true, true);

	if (!CBCFProjectSettingsDlg::LoadEnumerationProfile(*m_project)) {
		AfxMessageBox(L"Failed to load saved BCF project enumerations.", MB_OK | MB_ICONERROR);
	}

	CBCFProjectSettingsDlg dialog(*m_project, m_email, this);
	if (dialog.DoModal() != IDOK) {
		ReleaseProject();
		ShowProject();
		return;
	}

	m_email = dialog.GetUser();
	AfxGetApp()->WriteProfileString(L"BCF", L"User", m_email);
	if (!m_project->SetOptions(ToUTF8(m_email).c_str(), true, true) ||
		!CBCFProjectSettingsDlg::SaveEnumerationProfile(*m_project)) {
		AfxMessageBox(L"Failed to save new BCF project settings.", MB_OK | MB_ICONERROR);
	}

	m_filePath.Empty();
	LoadProjectInfo();
	UpdateCaption();

	BCFTopic* topic = CreateTopic();
	if (!topic) {
		ShowProject();
		Activate();
		return;
	}

	bool filesAdded = true;
	if (m_stepViewerDoc) {
		std::vector<CString> files;
		for (_model* model : m_stepViewerDoc->getModels()) {
			if (model) {
				files.push_back(model->getPath());
			}
		}
		if (!files.empty()) {
			CBCFIncudeLoadedModels includeDialog(files, this);
			if (includeDialog.DoModal() == IDOK) {
				for (_model* model : m_stepViewerDoc->getModels()) {
					if (model && !topic->AddBimFile(
							ToUTF8(model->getPath()).c_str(), includeDialog.IsExternal())) {
						filesAdded = false;
					}
				}
			}
		}
	}

	if (topic) {
		if (auto doc = GetDocument()) {
			if (auto comment = topic->AddComment()) {
				comment->SetText("My view");
				const bool ok = CBCFViewPointMgr(*doc)
					.SaveCurrentViewToComent(*comment);
			}
		}
	}

	m_projectForm->Load(topic);
	ShowTopic(topic);
	Activate();
	m_topicForm->FocusInitialControl(true);

	ShowLog(!filesAdded || !topic);
}

bool CBCFView::OpenProject(LPCTSTR filePath)
{
	if (!CloseActiveProjectForOperation(L"open another BCF project")) {
		return false;
	}

	CString filePathDlg;
	if (!filePath || !*filePath) {
		CFileDialog dialog(TRUE, nullptr, L"", OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, BCF_PACKAGES_FILTER);
		if (dialog.DoModal() != IDOK) {
			return false;
		}
		filePathDlg = dialog.GetPathName();
		filePath = filePathDlg;
    }

	m_project = BCFProject::Create();
	if (!m_project) {
		AfxMessageBox(L"Failed to initialize BCF project.", MB_OK | MB_ICONERROR);
		return false;
	}

	m_filePath = filePath;
	if (!m_project->ReadFile(ToUTF8(m_filePath).c_str(), true)) {
		ShowLog(true);
		ReleaseProject();
		return false;
	}

	m_email = AfxGetApp()->GetProfileString(L"BCF", L"User");
	m_project->SetOptions(ToUTF8(m_email).c_str(), true, true);

	AfxGetApp()->AddToRecentFileList(m_filePath);

	LoadProjectInfo();
	UpdateCaption();
	ShowProject();
	Activate();

    //show topic if only one topic exists in project
	if (auto topic1 = m_project->GetTopic(0)) {
		if (!m_project->GetTopic(1)) {
			m_projectForm->Load(topic1);
			ShowTopic(topic1);
		}
	}

	return true;
}

bool CBCFView::CloseActiveProjectForOperation(LPCTSTR operation)
{
	if (!m_project) {
		return true;
	}

	CString message;
	message.Format(
		L"The active BCF project must be closed before you %s.\n\nClose it now?",
		operation);
	if (AfxMessageBox(message, MB_YESNO | MB_ICONQUESTION) != IDYES) {
		return false;
	}

	CloseProject(true);
	return m_project == nullptr;
}

bool CBCFView::CommitCurrent()
{
	if (!CommitProjectInfo()) {
		return false;
	}
	if (m_activeForm == ProjectForm) {
		return true;
	}
	if (m_activeForm == TopicForm) {
		return m_topicForm->Commit();
	}
	return m_commentForm->Commit();
}

bool CBCFView::SaveProject()
{
	if (!m_project || !CommitCurrent()) {
		return false;
	}
	CFileDialog dialog(FALSE, L"bcf", m_filePath, OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY,
		L"BCF files (*.bcf)|*.bcf|BCF packages (*.bcfzip)|*.bcfzip|All Files (*.*)|*.*||");
	if (dialog.DoModal() != IDOK) {
		return false;
	}
	CString path = dialog.GetPathName();
	bool ok = m_project->WriteFile(ToUTF8(path).c_str(), BCFVer_3_0);
	ShowLog(!ok);
	if (ok) {
		m_filePath = path;
		UpdateCaption();
		m_projectForm->Load();
	}
	return ok;
}

bool CBCFView::AskAndSaveModified()
{
	if (!CommitCurrent()) {
		return false;
	}
	if (!m_project || !m_project->IsModified()) {
		return true;
	}
	int answer = AfxMessageBox(L"BCF pane project is modified. Do you want to save it?",
		MB_YESNOCANCEL | MB_ICONQUESTION);
	return answer == IDYES ? SaveProject() : answer == IDNO;
}

void CBCFView::CloseProject(bool prompt)
{
	if (!prompt || AskAndSaveModified()) {
		ReleaseProject();
		ShowProject();
	}
}

void CBCFView::OnCloseMainDocument()
{
	CloseProject(false);
	m_stepViewerDoc = nullptr;
}

void CBCFView::ReleaseProject()
{
	//close project
	if (m_project) {
		ShowLog(false);
		m_project->Delete();
		m_project = nullptr;
	}
	m_filePath.Empty();

	//cleanup models loaded from topics
	if (m_stepViewerDoc) {
		std::set<const _model*> removeModels;
		for (auto& pair : m_bimRef2Path) {
			if (pair.second) {
				if (auto model = m_stepViewerDoc->getModel(pair.second)) {
					removeModels.insert(model);
				}
			}
		}
		if (!removeModels.empty()) {
			m_stepViewerDoc->removeModels(removeModels, true);
        }
	}
	m_bimRef2Path.clear();

    //load empty project info
	LoadProjectInfo();
	UpdateCaption();
	m_topicForm->Load(nullptr);
	m_commentForm->Load(nullptr);
}

void CBCFView::ShowProject()
{
	if (!CommitProjectInfo()) {
		return;
	}
	if (m_activeForm == TopicForm && !m_topicForm->Commit()) {
		return;
	}
	if (m_activeForm == CommentForm) {
		if (!m_commentForm->Commit()) {
			return;
		}
		m_topicForm->ReloadComments(m_commentForm->GetComment());
	}
	m_projectForm->Load();
	ShowForm(ProjectForm);
}

void CBCFView::ShowTopic(BCFTopic* topic)
{
	if (!topic || !CommitProjectInfo()) {
		return;
	}
	if (m_activeForm == ProjectForm) {
		if (!m_projectForm->Commit()) {
			return;
		}
	}
	else if (m_activeForm == CommentForm) {
		BCFComment* comment = m_commentForm->GetComment();
		if (!m_commentForm->Commit()) {
			return;
		}
		if (m_topicForm->GetTopic() == topic) {
			m_topicForm->ReloadComments(comment);
			ShowForm(TopicForm);
			return;
		}
	}
	m_topicForm->Load(topic);
	ShowForm(TopicForm);
}

void CBCFView::ShowComment(BCFComment* comment)
{
	if (!comment || !CommitProjectInfo() || !m_topicForm->Commit()) {
		return;
	}
	m_commentForm->Load(comment);
	ShowForm(CommentForm);
}

void CBCFView::ShowForm(Form form)
{
	m_activeForm = form;
	const bool hasProject = m_project != nullptr;
	if (IsWindow(m_projectForm->GetSafeHwnd()))
		m_projectForm->ShowWindow (hasProject && form == ProjectForm ? SW_SHOW : SW_HIDE);
	if (IsWindow(m_topicForm->GetSafeHwnd()))
		m_topicForm->ShowWindow (hasProject && form == TopicForm ? SW_SHOW : SW_HIDE);
	if (IsWindow (m_commentForm->GetSafeHwnd()))
		m_commentForm->ShowWindow (hasProject && form == CommentForm ? SW_SHOW : SW_HIDE);
	if (IsWindow (GetSafeHwnd ()))	{
		AdjustLayout ();
		if (hasProject && form == TopicForm) {
			m_topicForm->FocusInitialControl();
		}
		else if (hasProject && form == CommentForm) {
			m_commentForm->FocusInitialControl();
		}
		}
}

void CBCFView::LoadProjectInfo()
{
	if (!m_projectId->GetSafeHwnd()) {
		return;
	}
	m_projectId->SetWindowText(m_project ? FromUTF8(m_project->GetProjectId()) : CString());
	m_projectName->SetWindowText(m_project ? FromUTF8(m_project->GetName()) : CString());
	m_projectName->SetModify(FALSE);
	const int projectCommand = m_project ? SW_SHOW : SW_HIDE;
	m_projectIdLabel.ShowWindow(projectCommand);
	m_projectId->ShowWindow(projectCommand);
	m_projectNameLabel.ShowWindow(projectCommand);
	m_projectName->ShowWindow(projectCommand);
	m_projectSettings.ShowWindow(projectCommand);
	m_emptyMessage.ShowWindow(m_project ? SW_HIDE : SW_SHOW);
	if (!m_project) {
		m_projectForm->ShowWindow(SW_HIDE);
		m_topicForm->ShowWindow(SW_HIDE);
		m_commentForm->ShowWindow(SW_HIDE);
	}
	AdjustLayout();
}

bool CBCFView::CommitProjectInfo()
{
	if (!m_project) {
		return true;
	}
	CString name;
	m_projectName->GetWindowText(name);
	name.Trim();
	if (name == FromUTF8(m_project->GetName())) {
		return true;
	}
	bool ok = m_project->SetName(ToUTF8(name).c_str());
	ShowLog(!ok);
	return ok;
}

void CBCFView::UpdateCaption()
{
	if (!GetSafeHwnd()) {
		return;
	}

	CString caption(L"BCF View");
	if (!m_filePath.IsEmpty()) {
		fs::path path(ToUTF8(m_filePath));
		caption.AppendFormat(L" - %s", FromUTF8(path.filename().string().c_str()).GetString());
	}
	SetWindowText(caption);
}

void CBCFView::AddTopic()
{
	if (!m_project) {
		return;
	}
	if (!m_projectForm->Commit()) {
		return;
	}
	BCFTopic* topic = CreateTopic();
	if (topic) {
		m_projectForm->Load(topic);
		ShowTopic(topic);
		m_topicForm->FocusInitialControl(true);
	}
}

BCFTopic* CBCFView::CreateTopic()
{
	if (!m_project) {
		return nullptr;
	}
	BCFExtensions& extensions = m_project->GetExtensions();
	const char* topicType = extensions.GetElement(BCFTopicTypes, 0);
	const char* topicStatus = extensions.GetElement(BCFTopicStatuses, 0);
	if (!topicType || !topicStatus) {
		AfxMessageBox(
			L"Topic Types and Topic Statuses must contain at least one value.",
			MB_OK | MB_ICONERROR);
		return nullptr;
	}
	BCFTopic* topic = m_project->AddTopic(
		topicType, "<< Enter topic title >>", topicStatus);
	
	ShowLog(!topic);
	return topic;
}

void CBCFView::DeleteTopic()
{
	BCFTopic* topic = m_projectForm->GetSelectedTopic();
	if (!topic) {
		return;
	}
	CString question;
	question.Format(L"Delete topic \"%s\"?", FromUTF8(topic->GetTitle()).GetString());
	if (AfxMessageBox(question, MB_YESNO | MB_ICONWARNING) == IDYES) {
		bool ok = topic->Remove();
		ShowLog(!ok);
		if (ok) {
			m_projectForm->Load();
		}
	}
}

void CBCFView::ShowTopicDetails()
{
	ShowTopic(m_projectForm->GetSelectedTopic());
}

void CBCFView::ShowLog(bool knownError)
{
	const char* message = m_project ? m_project->GetErrors() : nullptr;
	if (knownError && (!message || !*message)) {
		message = "Unknown BCF error";
	}
	if (message && *message) {
		AfxMessageBox(FromUTF8(message), knownError ? MB_ICONERROR : MB_ICONWARNING);
	}
}

CString CBCFView::GetBimFilePath(BCFBimFile& file)
{
	if (!m_stepViewerDoc) {
		return L"";
	}

	//
	//get local path to the BIM file, include resolving URI or ask user to locate it
	//
	auto refPath8 = file.GetReference();
	if (!refPath8 || !*refPath8) {
		refPath8 = file.GetFilename();
	}

	CString refPath = FromUTF8(refPath8);

	auto found = m_bimRef2Path.find(refPath);
	if (found != m_bimRef2Path.end()) {

		if (found->second.IsEmpty()) {
			//assume user already asked No to "Do you want to locate the file manually?"
			return L"";
		}

		//check if the model is in the Viewer
		if (m_stepViewerDoc->getModel(found->second)) {
			return found->second;
		}

		//something is not sync, below will re-load it to viewer
		m_bimRef2Path.erase(found); 
	}

	//
	// locate and load
	CString filePath = refPath;

	if (CUriDownloader::IsUri(filePath)) {
		filePath = CUriDownloader(this).GetLocalPath(refPath);
	}

	if (filePath.IsEmpty() || !fs::exists(ToUTF8(filePath))) {
		CString message;
		message.Format(L"Can not locate BIM file assigned to the topic: '%s'\n\nDo you want to locate the file manually?", refPath.GetString());

		if (AfxMessageBox(message, MB_YESNO | MB_ICONEXCLAMATION) != IDYES) {
			m_bimRef2Path[refPath] = L""; //avoid repeated asking
			return L"";
		}

        //user asked to locate the file manually
		CFileDialog dialog(TRUE, nullptr, L"", OFN_FILEMUSTEXIST, BIM_MODELS_FILTER);
		if (dialog.DoModal() != IDOK) {
			return L"";
		}

		filePath = dialog.GetPathName();
	}

	//
	//
	m_bimRef2Path[refPath] = filePath;

    //check if the model is already loaded in the Viewer
	if (!m_stepViewerDoc->getModel(filePath)) {

		auto model = _ap_model_factory::load(
			m_stepViewerDoc, filePath, false,
			m_stepViewerDoc->getModels().empty() ? nullptr : m_stepViewerDoc->getModels()[0],
			false);

		if (model) {
			m_stepViewerDoc->addModel(model);
		}

	}

	return filePath;
}

void CBCFView::SetBimFilesToView(BCFTopic& topic)
{
	if (!m_stepViewerDoc) {
		return;
	}

	std::vector<std::wstring> activeModels;
	std::vector<_model*> newModels;
	for (uint16_t i = 0; BCFBimFile * file = topic.GetBimFile(i); ++i) {
		std::wstring modelPath = GetBimFilePath(*file);
		activeModels.push_back(modelPath);
	}

	m_stepViewerDoc->enableModels(&activeModels);
}

void CBCFView::OnUpdateProjectSettings(CCmdUI* commandUI)
{
	commandUI->Enable(m_project != nullptr);
}

void CBCFView::OnProjectSettings()
{
	if (!m_project) {
		return;
	}
	CBCFProjectSettingsDlg dialog(*m_project, m_email, this);
	if (dialog.DoModal() == IDOK) {

		m_email = dialog.GetUser();
		AfxGetApp()->WriteProfileString(L"BCF", L"User", m_email);
		const bool ok = m_project->SetOptions(ToUTF8(m_email).c_str(), true, true);

		ShowLog(!ok);

		if (ok) {
			if (m_filePath.IsEmpty() &&
				!CBCFProjectSettingsDlg::SaveEnumerationProfile(*m_project)) {
				AfxMessageBox(L"Failed to save BCF project enumeration defaults.",
					MB_OK | MB_ICONERROR);
			}
			if (m_topicForm->GetTopic()) {
				m_topicForm->Load(m_topicForm->GetTopic());
			}
		}
	}
}

void CBCFView::AdjustLayout()
{
	if (!GetSafeHwnd()) {
		return;
	}
	CRect client;
	GetClientRect(client);

	CClientDC dc(this);
	CFont* font = m_projectIdLabel.GetFont();
	CFont* oldFont = font ? dc.SelectObject(font) : nullptr;
	TEXTMETRIC textMetrics = {};
	dc.GetTextMetrics(&textMetrics);
	const int textHeight = textMetrics.tmAscent + textMetrics.tmDescent + textMetrics.tmExternalLeading;
	const int rowHeight = textHeight + textHeight / 5;
	const int margin = rowHeight / 3;
	const int labelOffset = (rowHeight - textHeight) / 2;
	const int headerTop = client.top + margin;

	CString idLabelText;
	CString nameLabelText;
	CString projectIdText;
	CString settingsText;
	m_projectIdLabel.GetWindowText(idLabelText);
	m_projectNameLabel.GetWindowText(nameLabelText);
	m_projectId->GetWindowText(projectIdText);
	m_projectSettings.GetWindowText(settingsText);
	const int idLabelWidth = dc.GetTextExtent(idLabelText).cx + margin;
	const int nameLabelWidth = dc.GetTextExtent(nameLabelText).cx + margin;
	const DWORD projectIdMargins = m_projectId->GetMargins();
	const int projectIdWidth = max(rowHeight,
		static_cast<int>(dc.GetTextExtent(projectIdText).cx) +
		LOWORD(projectIdMargins) + HIWORD(projectIdMargins));
	const int settingsWidth = static_cast<int>(dc.GetTextExtent(settingsText).cx) + 2 * margin;
	if (oldFont) {
		dc.SelectObject(oldFont);
	}

	if (!m_project) {
		m_emptyMessage.MoveWindow(client);
		RedrawWindow(nullptr, nullptr,
			RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
		return;
	}

	const int settingsLeft = client.right - margin - settingsWidth;

	int left = margin;
	m_projectIdLabel.MoveWindow(
		left, headerTop + labelOffset, idLabelWidth, textHeight);
	left += idLabelWidth;
	m_projectId->MoveWindow(
		left, headerTop + labelOffset, projectIdWidth, rowHeight - labelOffset);
	left += projectIdWidth + margin;
	m_projectNameLabel.MoveWindow(
		left, headerTop + labelOffset, nameLabelWidth, textHeight);
	left += nameLabelWidth;
	m_projectName->MoveWindow(
		left, headerTop + labelOffset,
		max(rowHeight, settingsLeft - margin - left), rowHeight - labelOffset);

	m_projectSettings.MoveWindow(
		settingsLeft, headerTop, settingsWidth, rowHeight);

	CRect formRect(client.left,
		headerTop + rowHeight + margin, client.right, client.bottom);
	m_projectForm->MoveWindow(formRect);
	m_topicForm->MoveWindow(formRect);
	if (IsWindow(m_commentForm->GetSafeHwnd()))
		m_commentForm->MoveWindow(formRect);
	RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
}

void CBCFView::OnSize(UINT type, int cx, int cy)
{
	CDockablePane::OnSize(type, cx, cy);
	AdjustLayout();
}

void CBCFView::OnSetFocus(CWnd*)
{
	if (!m_project) {
		return;
	}
	if (m_activeForm == ProjectForm) {
		m_projectForm->SetFocus();
	}
	else if (m_activeForm == TopicForm) {
		m_topicForm->FocusInitialControl();
	}
	else {
		m_commentForm->FocusInitialControl();
	}
}
