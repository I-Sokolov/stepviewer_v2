#include "stdafx.h"

#include "BCFCommentForm.h"
#include "BCFProjectSettingsDlg.h"
#include "BCFProjectForm.h"
#include "BCFTopicForm.h"
#include "BCFView.h"
#include "BCFViewControls.h"
#include "STEPViewerDoc.h"
#include "Resource.h"
#include "_ap_model_factory.h"
#include "_ptr.h"

#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;

class CBCFPopupMenu : public CMFCPopupMenu
{
public:
	virtual BOOL Create(CWnd* parent, int x, int y, HMENU menu, BOOL locked = FALSE,
		BOOL ownMessage = FALSE) override
	{
		const BOOL showAllCommands = CMFCMenuBar::IsShowAllCommands();
		CMFCMenuBar::SetShowAllCommands(TRUE);
		const BOOL created = CMFCPopupMenu::Create(parent, x, y, menu, locked, ownMessage);
		CMFCMenuBar::SetShowAllCommands(showAllCommands);
		return created;
	}
};

class CBCFMenuButton : public CMFCToolBarMenuButton
{
	DECLARE_SERIAL(CBCFMenuButton)

public:
	CBCFMenuButton(HMENU menu = nullptr)
		: CMFCToolBarMenuButton(static_cast<UINT>(-1), menu, -1)
	{
	}

	virtual CMFCPopupMenu* CreatePopupMenu() override
	{
		return new CBCFPopupMenu;
	}
};

IMPLEMENT_SERIAL(CBCFMenuButton, CMFCToolBarMenuButton, 1)

BEGIN_MESSAGE_MAP(CBCFView, CDockablePane)
	ON_WM_CREATE()
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
	ON_COMMAND(ID_BCF_FILE_NEW, &CBCFView::OnNewFile)
	ON_COMMAND(ID_BCF_FILE_OPEN, &CBCFView::OnOpenFile)
	ON_COMMAND(ID_BCF_FILE_SAVE, &CBCFView::OnSaveFile)
	ON_BN_CLICKED(IDC_PANE_PROJECT_SETTINGS, &CBCFView::OnProjectSettings)
	ON_COMMAND(ID_BCF_PANE_ADD_TOPIC, &CBCFView::OnAddTopic)
	ON_COMMAND(ID_BCF_PANE_DELETE_TOPIC, &CBCFView::OnDeleteTopic)
	ON_COMMAND(ID_BCF_PANE_TOPIC_DETAILS, &CBCFView::OnTopicDetails)
	ON_COMMAND(ID_BCF_VIEW_PROJECT, &CBCFView::OnViewProject)
	ON_COMMAND(ID_BCF_VIEW_TOPIC, &CBCFView::OnViewTopic)
	ON_COMMAND(ID_BCF_VIEW_COMMENT, &CBCFView::OnViewComment)
	ON_COMMAND(ID_BCF_PANE_SAVE_COMMENT, &CBCFView::OnSaveComment)
	ON_COMMAND(ID_BCF_PANE_DELETE_COMMENT, &CBCFView::OnDeleteComment)
	ON_UPDATE_COMMAND_UI(ID_BCF_FILE_SAVE, &CBCFView::OnUpdateProjectCommand)
	ON_UPDATE_COMMAND_UI(ID_BCF_PANE_ADD_TOPIC, &CBCFView::OnUpdateProjectCommand)
	ON_UPDATE_COMMAND_UI(IDC_PANE_PROJECT_SETTINGS, &CBCFView::OnUpdateProjectSettings)
	ON_UPDATE_COMMAND_UI(ID_BCF_PANE_DELETE_TOPIC, &CBCFView::OnUpdateTopicCommand)
	ON_UPDATE_COMMAND_UI(ID_BCF_PANE_TOPIC_DETAILS, &CBCFView::OnUpdateTopicCommand)
	ON_UPDATE_COMMAND_UI(ID_BCF_VIEW_PROJECT, &CBCFView::OnUpdateViewProject)
	ON_UPDATE_COMMAND_UI(ID_BCF_VIEW_TOPIC, &CBCFView::OnUpdateViewTopic)
	ON_UPDATE_COMMAND_UI(ID_BCF_VIEW_COMMENT, &CBCFView::OnUpdateViewComment)
	ON_UPDATE_COMMAND_UI(ID_BCF_PANE_SAVE_COMMENT, &CBCFView::OnUpdateCommentCommand)
	ON_UPDATE_COMMAND_UI(ID_BCF_PANE_DELETE_COMMENT, &CBCFView::OnUpdateCommentCommand)
END_MESSAGE_MAP()

CBCFView::CBCFView()
	: m_document(nullptr)
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

	if (!m_menuBar.Create(this, AFX_DEFAULT_TOOLBAR_STYLE, IDR_BCF_VIEW_MENU) ||
		!m_menu.LoadMenu(IDR_BCF_VIEW_MENU)) {
		return -1;
	}
	m_menuBar.SetPaneStyle((m_menuBar.GetPaneStyle() | CBRS_TOOLTIPS | CBRS_FLYBY) &
		~(CBRS_GRIPPER | CBRS_SIZE_DYNAMIC | CBRS_BORDER_TOP | CBRS_BORDER_BOTTOM | CBRS_BORDER_LEFT | CBRS_BORDER_RIGHT));
	m_menuBar.SetOwner(this);
	m_menuBar.SetRouteCommandsViaFrame(FALSE);
	m_menuBar.SetDefaultMenuResId(IDR_BCF_VIEW_MENU);
	m_menuBar.SetMenuButtonRTC(RUNTIME_CLASS(CBCFMenuButton));
	m_menuBar.CreateFromMenu(m_menu.GetSafeHmenu(), TRUE);
	m_menuBar.SetMessageWnd(this);

	CreateBCFStaticLabel(m_projectIdLabel, L"Project Id:", this);
	CreateBCFStaticLabel(m_projectNameLabel, L"Name:", this);
	m_projectSettings.Create(L"Settings...",
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
	if (!AskAndSaveModified()) {
		return;
	}
	ReleaseProject();

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

	BCFTopic* topic = m_project->AddTopic(nullptr, nullptr, nullptr);
	if (!topic) {
		ShowLog(true);
		ShowProject();
		Activate();
		return;
	}

	bool filesAdded = true;
	if (m_document) {
		for (_model* model : m_document->getModels()) {
			if (model && !topic->AddBimFile(ToUTF8(model->getPath()).c_str(), false)) {
				filesAdded = false;
			}
		}
	}

	ShowLog(!filesAdded);
	m_projectForm->Load(topic);
	ShowTopic(topic);
	Activate();
}

void CBCFView::OpenProject()
{
	CFileDialog dialog(TRUE, nullptr, L"", OFN_FILEMUSTEXIST | OFN_HIDEREADONLY, BCF_PACKAGES_FILTER);
	if (dialog.DoModal() != IDOK || !AskAndSaveModified()) {
		return;
	}
	ReleaseProject();
	m_project = BCFProject::Create();
	if (!m_project) {
		AfxMessageBox(L"Failed to initialize BCF project.", MB_OK | MB_ICONERROR);
		return;
	}
	m_email = AfxGetApp()->GetProfileString(L"BCF", L"User");
	m_project->SetOptions(ToUTF8(m_email).c_str(), true, true);

	m_filePath = dialog.GetPathName();
	if (!m_project->ReadFile(ToUTF8(m_filePath).c_str(), true)) {
		ShowLog(true);
		ReleaseProject();
		return;
	}
	LoadProjectInfo();
	UpdateCaption();
	ShowProject();
	Activate();
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
	m_document = nullptr;
}

void CBCFView::ReleaseProject()
{
	if (m_project) {
		ShowLog(false);
		m_project->Delete();
		m_project = nullptr;
	}
	m_filePath.Empty();
	m_bimModels.clear();
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
	m_projectForm->ShowWindow(form == ProjectForm ? SW_SHOW : SW_HIDE);
	m_topicForm->ShowWindow(form == TopicForm ? SW_SHOW : SW_HIDE);
	m_commentForm->ShowWindow(form == CommentForm ? SW_SHOW : SW_HIDE);
	AdjustLayout();
	RefreshCommandUI();
}

void CBCFView::LoadProjectInfo()
{
	if (!m_projectId->GetSafeHwnd()) {
		return;
	}
	m_projectId->SetWindowText(m_project ? FromUTF8(m_project->GetProjectId()) : CString());
	m_projectName->SetWindowText(m_project ? FromUTF8(m_project->GetName()) : CString());
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
	CString caption(L"BCF View");
	if (!m_filePath.IsEmpty()) {
		fs::path path(ToUTF8(m_filePath));
		caption.AppendFormat(L" - %s", FromUTF8(path.filename().string().c_str()).GetString());
	}
	SetWindowText(caption);
}

void CBCFView::OnAddTopic()
{
	if (!m_project) {
		return;
	}
	if (!m_projectForm->Commit()) {
		return;
	}
	BCFTopic* topic = m_project->AddTopic(nullptr, nullptr, nullptr);
	ShowLog(!topic);
	if (topic) {
		m_projectForm->Load(topic);
		ShowTopic(topic);
	}
}

void CBCFView::OnDeleteTopic()
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

void CBCFView::OnTopicDetails()
{
	ShowTopic(m_projectForm->GetSelectedTopic());
}

void CBCFView::OnViewProject() { ShowProject(); }
void CBCFView::OnViewTopic() { ShowTopic(m_projectForm->GetSelectedTopic()); }
void CBCFView::OnViewComment() { ShowComment(m_topicForm->GetSelectedComment()); }

void CBCFView::OnSaveComment()
{
	if (m_commentForm->Commit()) {
		m_topicForm->ReloadComments(m_commentForm->GetComment());
	}
}

void CBCFView::OnDeleteComment()
{
	BCFComment* comment = m_commentForm->GetComment();
	if (!comment || AfxMessageBox(L"Delete this comment?", MB_YESNO | MB_ICONWARNING) != IDYES) {
		return;
	}
	bool ok = comment->Remove();
	ShowLog(!ok);
	if (ok) {
		m_commentForm->Load(nullptr);
		m_topicForm->ReloadComments();
		ShowForm(TopicForm);
	}
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

_model* CBCFView::GetBimModel(BCFBimFile& file)
{
	auto found = m_bimModels.find(&file);
	if (found != m_bimModels.end()) {
		if (m_document) {
			const auto& models = m_document->getModels();
			if (std::find(models.begin(), models.end(), found->second) != models.end()) {
				return found->second;
			}
		}
		m_bimModels.erase(found);
	}
	if (!m_document) {
		return nullptr;
	}
	CString path = FromUTF8(file.GetReference());
	for (_model* candidate : m_document->getModels()) {
		if (candidate->getPath() == path) {
			m_bimModels[&file] = candidate;
			return candidate;
		}
	}
	if (!fs::exists(ToUTF8(path))) {
		CString message(L"Can not locate BIM file assigned to the topic.\n\n");
		message.AppendFormat(L"Reference: '%s'\n\nDo you want to locate the file manually?", path.GetString());
		if (AfxMessageBox(message, MB_YESNO | MB_ICONEXCLAMATION) != IDYES) {
			return nullptr;
		}
		CFileDialog dialog(TRUE, nullptr, L"", OFN_FILEMUSTEXIST, BIM_MODELS_FILTER);
		if (dialog.DoModal() != IDOK) {
			return nullptr;
		}
		path = dialog.GetPathName();
	}
	_model* model = _ap_model_factory::load(m_document, path, false,
		m_document->getModels().empty() ? nullptr : m_document->getModels()[0], false);
	if (model) {
		_ptr<_ap_model> apModel(model);
		if (apModel->getAP() == enumAP::IFC) {
			m_bimModels[&file] = model;
		}
		else {
			delete model;
			model = nullptr;
		}
	}
	return model;
}

void CBCFView::LoadBimFiles(BCFTopic& topic)
{
	if (!m_document) {
		return;
	}
	std::vector<_model*> activeModels;
	for (uint16_t i = 0; BCFBimFile* file = topic.GetBimFile(i); ++i) {
		if (_model* model = GetBimModel(*file)) {
			activeModels.push_back(model);
		}
	}
	m_document->enableModelsAddIfNeeded(activeModels);
}

void CBCFView::RefreshCommandUI()
{
	if (m_menuBar.GetSafeHwnd()) {
		m_menuBar.OnUpdateCmdUI(nullptr, FALSE);
		m_menuBar.Invalidate();
	}
}

void CBCFView::OnUpdateProjectCommand(CCmdUI* commandUI)
{
	commandUI->Enable(m_activeForm == ProjectForm && m_project != nullptr);
}

void CBCFView::OnUpdateProjectSettings(CCmdUI* commandUI)
{
	commandUI->Enable(m_project != nullptr);
}

void CBCFView::OnUpdateTopicCommand(CCmdUI* commandUI)
{
	commandUI->Enable(m_activeForm == ProjectForm && m_projectForm->GetSelectedTopic() != nullptr);
}

void CBCFView::OnUpdateCommentCommand(CCmdUI* commandUI)
{
	commandUI->Enable(m_activeForm == CommentForm && m_commentForm->GetComment() != nullptr);
}

void CBCFView::OnUpdateViewProject(CCmdUI* commandUI)
{
	commandUI->Enable(TRUE);
	commandUI->SetRadio(m_activeForm == ProjectForm);
}

void CBCFView::OnUpdateViewTopic(CCmdUI* commandUI)
{
	commandUI->Enable(m_projectForm->GetSelectedTopic() != nullptr);
	commandUI->SetRadio(m_activeForm == TopicForm);
}

void CBCFView::OnUpdateViewComment(CCmdUI* commandUI)
{
	const bool hasSelectedComment = m_topicForm->GetTopic() == m_projectForm->GetSelectedTopic() &&
		m_topicForm->GetSelectedComment() != nullptr;
	commandUI->Enable(hasSelectedComment);
	commandUI->SetRadio(m_activeForm == CommentForm);
}

void CBCFView::OnNewFile() { NewProject(); }
void CBCFView::OnOpenFile() { OpenProject(); }
void CBCFView::OnSaveFile() { SaveProject(); }

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
	if (!GetSafeHwnd() || !m_menuBar.GetSafeHwnd()) {
		return;
	}
	CRect client;
	GetClientRect(client);
	CSize menuSize = m_menuBar.CalcFixedLayout(FALSE, TRUE);
	m_menuBar.SetWindowPos(nullptr, client.left, client.top, client.Width(), menuSize.cy,
		SWP_NOACTIVATE | SWP_NOZORDER);

	CClientDC dc(this);
	CFont* font = m_projectIdLabel.GetFont();
	CFont* oldFont = font ? dc.SelectObject(font) : nullptr;
	TEXTMETRIC textMetrics = {};
	dc.GetTextMetrics(&textMetrics);
	const int textHeight = textMetrics.tmAscent + textMetrics.tmDescent + textMetrics.tmExternalLeading;
	const int rowHeight = textHeight + textHeight / 2;
	const int margin = rowHeight / 3;
	const int labelOffset = (rowHeight - textHeight) / 2;
	const int headerTop = client.top + menuSize.cy + margin;

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

	int headerHeight = rowHeight;
	const int minEditWidth = 3 * rowHeight;
	const int twoColumnWidth =
		4 * margin + idLabelWidth + projectIdWidth + nameLabelWidth + minEditWidth + settingsWidth;
	if (client.Width() >= twoColumnWidth) {
		m_projectIdLabel.MoveWindow(margin, headerTop + labelOffset, idLabelWidth, textHeight);
		m_projectId->MoveWindow(margin + idLabelWidth, headerTop + labelOffset,
			projectIdWidth, rowHeight - labelOffset);
		const int second = 2 * margin + idLabelWidth + projectIdWidth;
		m_projectNameLabel.MoveWindow(second, headerTop + labelOffset, nameLabelWidth, textHeight);
		m_projectName->MoveWindow(second + nameLabelWidth, headerTop + labelOffset,
			client.right - second - nameLabelWidth - settingsWidth - 2 * margin,
			rowHeight - labelOffset);
		m_projectSettings.MoveWindow(client.right - settingsWidth - margin,
			headerTop, settingsWidth, rowHeight);
	}
	else {
		const int labelWidth = max(idLabelWidth, nameLabelWidth);
		const int editWidth = max(rowHeight, client.Width() - 2 * margin - labelWidth);
		m_projectIdLabel.MoveWindow(margin, headerTop + labelOffset, labelWidth, textHeight);
		m_projectId->MoveWindow(margin + labelWidth, headerTop + labelOffset,
			min(projectIdWidth, editWidth), rowHeight - labelOffset);
		m_projectNameLabel.MoveWindow(margin, headerTop + rowHeight + labelOffset, labelWidth, textHeight);
		m_projectName->MoveWindow(margin + labelWidth, headerTop + rowHeight + labelOffset,
			max(rowHeight, editWidth - settingsWidth - margin), rowHeight - labelOffset);
		m_projectSettings.MoveWindow(client.right - settingsWidth - margin,
			headerTop + rowHeight, settingsWidth, rowHeight);
		headerHeight = 2 * rowHeight;
	}
	CRect formRect(client.left, headerTop + headerHeight + margin, client.right, client.bottom);
	m_projectForm->MoveWindow(formRect);
	m_topicForm->MoveWindow(formRect);
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
	if (m_activeForm == ProjectForm) {
		m_projectForm->SetFocus();
	}
	else if (m_activeForm == TopicForm) {
		m_topicForm->SetFocus();
	}
	else {
		m_commentForm->SetFocus();
	}
}
