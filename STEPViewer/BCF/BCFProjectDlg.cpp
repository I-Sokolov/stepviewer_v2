#include "stdafx.h"

#include "STEPViewer.h"
#include "STEPViewerDoc.h"
#include "_ap_model_factory.h"
#include "BCF\BCFProjectDlg.h"
#include "BCF\BCFTopicDlg.h"

#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

namespace
{
	const wchar_t* PROFILE_SECTION = L"BCFProjectView";
	const int COLUMN_COUNT = 10;
	const int DEFAULT_COLUMN_WIDTHS[COLUMN_COUNT] = { 190, 180, 90, 90, 90, 130, 80, 90, 130, 130 };
}

IMPLEMENT_DYNAMIC(CBCFProjectDlg, CDialogEx)

BEGIN_MESSAGE_MAP(CBCFProjectDlg, CDialogEx)
	ON_WM_CLOSE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_ACTIVATE()
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_BCF_TOPIC_LIST, &CBCFProjectDlg::OnItemChangedTopics)
	ON_NOTIFY(NM_DBLCLK, IDC_BCF_TOPIC_LIST, &CBCFProjectDlg::OnDoubleClickTopics)
	ON_BN_CLICKED(IDC_BCF_TOPIC_DETAILS, &CBCFProjectDlg::OnClickedTopicDetails)
	ON_BN_CLICKED(IDC_BCF_NEW_TOPIC, &CBCFProjectDlg::OnClickedNewTopic)
	ON_BN_CLICKED(IDC_BCF_DELETE_TOPIC, &CBCFProjectDlg::OnClickedDeleteTopic)
	ON_BN_CLICKED(IDC_BCF_SAVE_PROJECT, &CBCFProjectDlg::OnClickedSaveProject)
	ON_BN_CLICKED(IDC_BCF_CLOSE_PROJECT, &CBCFProjectDlg::OnClickedCloseDialog)
	ON_EN_KILLFOCUS(IDC_BCF_EMAIL, &CBCFProjectDlg::OnKillFocusProjectInfo)
	ON_EN_KILLFOCUS(IDC_BCF_PROJECT_NAME, &CBCFProjectDlg::OnKillFocusProjectInfo)
END_MESSAGE_MAP()

CBCFProjectDlg::CBCFProjectDlg(CMySTEPViewerDoc& doc)
	: CDialogEx(IDD_BCF_PROJECT_DLG, AfxGetMainWnd())
	, m_doc(doc)
	, m_project(NULL)
	, m_initialized(false)
{
}

CBCFProjectDlg::~CBCFProjectDlg()
{
	Close();
}

void CBCFProjectDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_BCF_EMAIL, m_email);
	DDX_Text(pDX, IDC_BCF_PROJECT_ID, m_projectId);
	DDX_Text(pDX, IDC_BCF_PROJECT_NAME, m_projectName);
	DDX_Control(pDX, IDC_BCF_TOPIC_LIST, m_topics);
}

BOOL CBCFProjectDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_topics.SetExtendedStyle(m_topics.GetExtendedStyle() | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
	const wchar_t* columns[COLUMN_COUNT] = {
		L"GUID", L"Title", L"Type", L"Stage", L"Status",
		L"Assigned", L"Priority", L"Due", L"Created", L"Modified"
	};
	for (int i = 0; i < COLUMN_COUNT; i++) {
		m_topics.InsertColumn(i, columns[i], LVCFMT_LEFT, DEFAULT_COLUMN_WIDTHS[i]);
	}
	RestoreColumnWidths();

	EnableDynamicLayout();
	if (auto layout = GetDynamicLayout()) {
		if (layout->Create(this)) {
			layout->AddItem(IDC_BCF_PROJECT_NAME, CMFCDynamicLayout::MoveNone(), CMFCDynamicLayout::SizeHorizontal(100));
			layout->AddItem(IDC_BCF_TOPIC_LIST, CMFCDynamicLayout::MoveNone(), CMFCDynamicLayout::SizeHorizontalAndVertical(100, 100));
			layout->AddItem(IDC_BCF_TOPIC_DETAILS, CMFCDynamicLayout::MoveVertical(100), CMFCDynamicLayout::SizeNone());
			layout->AddItem(IDC_BCF_EMAIL, CMFCDynamicLayout::MoveVertical(100), CMFCDynamicLayout::SizeNone());
			layout->AddItem(IDC_BCF_EMAIL_LABEL, CMFCDynamicLayout::MoveVertical(100), CMFCDynamicLayout::SizeNone());
			layout->AddItem(IDC_BCF_NEW_TOPIC, CMFCDynamicLayout::MoveVertical(100), CMFCDynamicLayout::SizeNone());
			layout->AddItem(IDC_BCF_DELETE_TOPIC, CMFCDynamicLayout::MoveVertical(100), CMFCDynamicLayout::SizeNone());
			layout->AddItem(IDC_BCF_SAVE_PROJECT, CMFCDynamicLayout::MoveHorizontalAndVertical(100, 100), CMFCDynamicLayout::SizeNone());
			layout->AddItem(IDC_BCF_CLOSE_PROJECT, CMFCDynamicLayout::MoveHorizontalAndVertical(100, 100), CMFCDynamicLayout::SizeNone());
		}
	}

	RestorePlacement();
	m_initialized = true;
	LoadProject();
	return TRUE;
}

bool CBCFProjectDlg::IsBCF(LPCTSTR filePath) const
{
	if (!filePath) {
		return false;
	}
	auto len = wcslen(filePath);
	return (len >= 4 && _wcsicmp(filePath + len - 4, L".bcf") == 0)
		|| (len >= 7 && _wcsicmp(filePath + len - 7, L".bcfzip") == 0);
}

void CBCFProjectDlg::Open(LPCTSTR filePath)
{
	Close();
	m_filePath = filePath ? filePath : L"";
	m_project = BCFProject::Create();
	if (!m_project) {
		AfxMessageBox(L"Failed to initialize BCF project.", MB_OK | MB_ICONERROR);
		return;
	}

	m_email = AfxGetApp()->GetProfileString(L"BCF", L"User");
	m_project->SetOptions(ToUTF8(m_email).c_str(), true, true);
	if (!m_filePath.IsEmpty() && !m_project->ReadFile(ToUTF8(m_filePath).c_str(), true)) {
		ShowLog(true);
		Close();
		return;
	}

	if (!GetSafeHwnd()) {
		Create(IDD_BCF_PROJECT_DLG, AfxGetMainWnd());
	}
	else {
		LoadProject();
	}
	ShowWindow(SW_SHOW);
	SetForegroundWindow();
}

void CBCFProjectDlg::Close()
{
	if (GetSafeHwnd() && m_initialized) {
		UpdateProjectInfo();
		SavePlacement();
		SaveColumnWidths();
		ShowWindow(SW_HIDE);
	}
	if (m_project) {
		ShowLog(false);
		if (!m_project->Delete()) {
			ShowLog(true);
		}
		m_project = NULL;
	}
	m_filePath.Empty();
}

void CBCFProjectDlg::LoadProject()
{
	if (!GetSafeHwnd()) {
		return;
	}
	if (!m_project) {
		m_projectId.Empty();
		m_projectName.Empty();
		UpdateData(FALSE);
		m_topics.DeleteAllItems();
		UpdateButtons();
		return;
	}

	m_email = AfxGetApp()->GetProfileString(L"BCF", L"User");
	m_projectId = FromUTF8(m_project->GetProjectId());
	m_projectName = FromUTF8(m_project->GetName());
	UpdateData(FALSE);
	RefreshTopics();

	CString title;
	title.Format(L"BCF - %s", m_filePath.IsEmpty() ? L"<New>" : m_filePath.GetString());
	SetWindowText(title);
}

void CBCFProjectDlg::RefreshTopics(BCFTopic* selectTopic)
{
	if (!m_project) {
		return;
	}
	if (!selectTopic) {
		selectTopic = GetActiveTopic();
	}

	m_topics.SetRedraw(FALSE);
	m_topics.DeleteAllItems();
	int selected = -1;
	for (uint16_t i = 0; ; i++) {
		auto topic = m_project->GetTopic(i);
		if (!topic) {
			break;
		}
		int item = m_topics.InsertItem(m_topics.GetItemCount(), FromUTF8(topic->GetGuid()));
		m_topics.SetItemText(item, 1, FromUTF8(topic->GetTitle()));
		m_topics.SetItemText(item, 2, FromUTF8(topic->GetTopicType()));
		m_topics.SetItemText(item, 3, FromUTF8(topic->GetStage()));
		m_topics.SetItemText(item, 4, FromUTF8(topic->GetTopicStatus()));
		m_topics.SetItemText(item, 5, FromUTF8(topic->GetAssignedTo()));
		m_topics.SetItemText(item, 6, FromUTF8(topic->GetPriority()));
		m_topics.SetItemText(item, 7, FromUTF8(topic->GetDueDate()));
		CString created = FormatDateTime(topic->GetCreationDate());
		CString createdAuthor = FromUTF8(topic->GetCreationAuthor());
		if (!createdAuthor.IsEmpty()) {
			created.AppendFormat(L" - %s", createdAuthor.GetString());
		}
		m_topics.SetItemText(item, 8, created);

		CString modified = FormatDateTime(topic->GetModifiedDate());
		CString modifiedAuthor = FromUTF8(topic->GetModifiedAuthor());
		if (!modifiedAuthor.IsEmpty()) {
			modified.AppendFormat(L" - %s", modifiedAuthor.GetString());
		}
		m_topics.SetItemText(item, 9, modified);
		m_topics.SetItemData(item, reinterpret_cast<DWORD_PTR>(topic));
		if (topic == selectTopic) {
			selected = item;
		}
	}
	m_topics.SetRedraw(TRUE);
	m_topics.Invalidate();

	if (selected < 0 && m_topics.GetItemCount() > 0) {
		selected = 0;
	}
	if (selected >= 0) {
		m_topics.SetItemState(selected, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
		m_topics.EnsureVisible(selected, FALSE);
	}
	UpdateButtons();
}

CString CBCFProjectDlg::FormatDateTime(const char* value)
{
	if (!value || !*value) {
		return CString();
	}

	SYSTEMTIME time = {};
	if (sscanf_s(value, "%4hu-%2hu-%2huT%2hu:%2hu:%2hu",
		&time.wYear, &time.wMonth, &time.wDay,
		&time.wHour, &time.wMinute, &time.wSecond) != 6) {
		return FromUTF8(value);
	}

	FILETIME fileTime;
	if (!SystemTimeToFileTime(&time, &fileTime)) {
		return FromUTF8(value);
	}

	wchar_t date[64] = {};
	wchar_t clock[64] = {};
	if (!GetDateFormatEx(LOCALE_NAME_USER_DEFAULT, DATE_SHORTDATE, &time, NULL, date, _countof(date), NULL)
		|| !GetTimeFormatEx(LOCALE_NAME_USER_DEFAULT, TIME_NOSECONDS, &time, NULL, clock, _countof(clock))) {
		return FromUTF8(value);
	}

	CString result(date);
	result += L" ";
	result += clock;
	return result;
}

BCFTopic* CBCFProjectDlg::GetActiveTopic()
{
	if (!m_topics.GetSafeHwnd()) {
		return NULL;
	}

	POSITION position = m_topics.GetFirstSelectedItemPosition();
	if (!position) {
		return NULL;
	}
	int item = m_topics.GetNextSelectedItem(position);
	return reinterpret_cast<BCFTopic*>(m_topics.GetItemData(item));
}

_model* CBCFProjectDlg::GetBimModel(BCFBimFile& file)
{
	auto found = m_mapBimFiles.find(&file);
	if (found != m_mapBimFiles.end()) {
		return found->second;
	}

	_model* model = nullptr;
	CString path = FromUTF8(file.GetReference());
	for (auto candidate : m_doc.getModels()) {
		if (candidate->getPath() == path) {
			model = candidate;
			break;
		}
	}

	if (!model && !fs::exists(ToUTF8(path))) {
		CString message(L"Can not locate BIM file assigned to the topic.\n\n");
		if (!path.IsEmpty()) {
			message += L"Reference: '" + path + L"'\n";
		}
		CString name = FromUTF8(file.GetFilename());
		if (!name.IsEmpty()) {
			message += L"Name: '" + name + L"'\n";
		}
		CString ifcProjectGuid = FromUTF8(file.GetIfcProject());
		if (!ifcProjectGuid.IsEmpty()) {
			message += L"IfcProject.GlobalId: '" + ifcProjectGuid + L"'\n";
		}
		message += L"\nDo you want to locate the file manually?";

		if (AfxMessageBox(message, MB_YESNO | MB_ICONEXCLAMATION) == IDYES) {
			CFileDialog dialog(TRUE, nullptr, _T(""), OFN_FILEMUSTEXIST, BIM_MODELS_FILTER);
			if (dialog.DoModal() != IDOK) {
				return nullptr;
			}
			path = dialog.GetPathName();
		}
	}

	if (!model && fs::exists(ToUTF8(path))) {
		model = _ap_model_factory::load(&m_doc, path, false,
			m_doc.getModels().empty() ? nullptr : m_doc.getModels()[0], false);
		if (model) {
			_ptr<_ap_model> apModel(model);
			if (apModel->getAP() == enumAP::IFC) {
				m_mapBimFiles[&file] = model;
			}
			else {
				delete model;
				model = nullptr;
			}
		}
	}

	return model;
}

void CBCFProjectDlg::UpdateProjectInfo()
{
	if (!m_project || !GetSafeHwnd()) {
		return;
	}
	UpdateData();
	m_email.Trim();
	m_projectName.Trim();
	AfxGetApp()->WriteProfileString(L"BCF", L"User", m_email);
	m_project->SetOptions(ToUTF8(m_email).c_str(), true, true);
	if (m_projectName != FromUTF8(m_project->GetName())) {
		m_project->SetName(ToUTF8(m_projectName).c_str());
	}
}

bool CBCFProjectDlg::SaveProject()
{
	if (!m_project) {
		return false;
	}
	UpdateProjectInfo();

	LPCTSTR filter = _T("BCF files (*.bcf)|*.bcf|BCF packages (*.bcfzip)|*.bcfzip|All Files (*.*)|*.*||");
	CFileDialog dialog(FALSE, L"bcf", m_filePath, OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY, filter);
	if (dialog.DoModal() != IDOK) {
		return false;
	}

	CString path = dialog.GetPathName();
	bool ok = m_project->WriteFile(ToUTF8(path).c_str(), BCFVer_3_0);
	ShowLog(!ok);
	if (ok) {
		m_filePath = path;
		LoadProject();
	}
	return ok;
}

bool CBCFProjectDlg::SaveModified()
{
	UpdateProjectInfo();
	if (!m_project || !m_project->IsModified()) {
		return true;
	}
	auto answer = AfxMessageBox(L"BCF project is modified. Do you want to save it?", MB_YESNOCANCEL | MB_ICONQUESTION);
	if (answer == IDYES) {
		return SaveProject();
	}
	return answer == IDNO;
}

void CBCFProjectDlg::OnClickedTopicDetails()
{
	auto topic = GetActiveTopic();
	if (m_project && topic) {
		UpdateProjectInfo();
		CBCFTopicDlg topicDlg(*this, *topic);
		topicDlg.DoModal();
		RefreshTopics(topic);
	}
}

void CBCFProjectDlg::OnClickedNewTopic()
{
	if (!m_project) {
		return;
	}
	UpdateProjectInfo();
	auto topic = m_project->AddTopic(NULL, NULL, NULL);
	ShowLog(!topic);
	if (topic) {
		RefreshTopics(topic);
		CBCFTopicDlg topicDlg(*this, *topic);
		topicDlg.DoModal();
		RefreshTopics(topic);
	}
}

void CBCFProjectDlg::OnClickedDeleteTopic()
{
	auto topic = GetActiveTopic();
	if (!topic) {
		return;
	}
	CString question;
	question.Format(L"Delete topic \"%s\"?", FromUTF8(topic->GetTitle()).GetString());
	if (AfxMessageBox(question, MB_YESNO | MB_ICONWARNING) == IDYES) {
		if (!topic->Remove()) {
			ShowLog(true);
		}
		else {
			RefreshTopics();
		}
	}
}

void CBCFProjectDlg::UpdateButtons()
{
	BOOL hasTopic = GetActiveTopic() != NULL;
	GetDlgItem(IDC_BCF_TOPIC_DETAILS)->EnableWindow(hasTopic);
	GetDlgItem(IDC_BCF_DELETE_TOPIC)->EnableWindow(hasTopic);
}

void CBCFProjectDlg::ShowLog(bool knownError)
{
	const char* message = m_project ? m_project->GetErrors() : NULL;
	if (knownError && (!message || !*message)) {
		message = "Unknown BCF error";
	}
	if (message && *message) {
		AfxMessageBox(FromUTF8(message), knownError ? MB_ICONERROR : MB_ICONWARNING);
	}
}

void CBCFProjectDlg::RestorePlacement()
{
	auto app = AfxGetApp();
	int width = app->GetProfileInt(PROFILE_SECTION, L"Width", 0);
	int height = app->GetProfileInt(PROFILE_SECTION, L"Height", 0);
	if (width <= 0 || height <= 0) {
		return;
	}
	int x = app->GetProfileInt(PROFILE_SECTION, L"X", 0);
	int y = app->GetProfileInt(PROFILE_SECTION, L"Y", 0);
	CRect rect(x, y, x + width, y + height);
	CRect workArea;
	SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);
	if (rect.IntersectRect(rect, workArea) && rect.Width() >= 300 && rect.Height() >= 200) {
		SetWindowPos(NULL, x, y, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
	}
}

void CBCFProjectDlg::SavePlacement()
{
	if (IsIconic() || IsZoomed()) {
		return;
	}
	CRect rect;
	GetWindowRect(rect);
	auto app = AfxGetApp();
	app->WriteProfileInt(PROFILE_SECTION, L"X", rect.left);
	app->WriteProfileInt(PROFILE_SECTION, L"Y", rect.top);
	app->WriteProfileInt(PROFILE_SECTION, L"Width", rect.Width());
	app->WriteProfileInt(PROFILE_SECTION, L"Height", rect.Height());
}

void CBCFProjectDlg::RestoreColumnWidths()
{
	for (int i = 0; i < COLUMN_COUNT; i++) {
		CString key;
		key.Format(L"Column%d", i);
		int width = AfxGetApp()->GetProfileInt(PROFILE_SECTION, key, DEFAULT_COLUMN_WIDTHS[i]);
		m_topics.SetColumnWidth(i, width < 20 ? 20 : width);
	}
}

void CBCFProjectDlg::SaveColumnWidths()
{
	for (int i = 0; i < COLUMN_COUNT; i++) {
		CString key;
		key.Format(L"Column%d", i);
		AfxGetApp()->WriteProfileInt(PROFILE_SECTION, key, m_topics.GetColumnWidth(i));
	}
}

void CBCFProjectDlg::OnItemChangedTopics(NMHDR*, LRESULT* result)
{
	UpdateButtons();
	*result = 0;
}

void CBCFProjectDlg::OnDoubleClickTopics(NMHDR*, LRESULT* result)
{
	OnClickedTopicDetails();
	*result = 0;
}

void CBCFProjectDlg::OnClickedSaveProject()
{
	SaveProject();
}

void CBCFProjectDlg::OnClickedCloseDialog()
{
	OnClose();
}

void CBCFProjectDlg::OnKillFocusProjectInfo()
{
	UpdateProjectInfo();
}

void CBCFProjectDlg::OnClose()
{
	if (SaveModified()) {
		Close();
	}
}

void CBCFProjectDlg::OnDestroy()
{
	if (m_initialized) {
		SavePlacement();
		SaveColumnWidths();
	}
	CDialogEx::OnDestroy();
}

void CBCFProjectDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);
}

void CBCFProjectDlg::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	CDialogEx::OnActivate(nState, pWndOther, bMinimized);
	if (nState != WA_INACTIVE && m_project && m_initialized) {
		RefreshTopics();
	}
}

void CBCFProjectDlg::OnCancel()
{
	OnClose();
}

void CBCFProjectDlg::OnOK()
{
	OnClickedTopicDetails();
}
