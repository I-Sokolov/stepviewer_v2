#include "stdafx.h"

#include "UriDownloader.h"

#include <afxdlgs.h>
#include <winhttp.h>

#pragma comment(lib, "Winhttp.lib")

namespace
{
	const wchar_t* PROFILE_SECTION = L"BCF";
	const wchar_t* PROFILE_CACHE_FOLDER = L"UriCacheFolder";

	class CInternetHandle
	{
	public:
		explicit CInternetHandle(HINTERNET handle = nullptr) : m_handle(handle) {}
		~CInternetHandle()
		{
			if (m_handle) {
				WinHttpCloseHandle(m_handle);
			}
		}

		operator HINTERNET() const { return m_handle; }
		void Reset(HINTERNET handle)
		{
			if (m_handle) {
				WinHttpCloseHandle(m_handle);
			}
			m_handle = handle;
		}

	private:
		CInternetHandle(const CInternetHandle&);
		CInternetHandle& operator=(const CInternetHandle&);

		HINTERNET m_handle;
	};

	struct CUriParts
	{
		CString host;
		CString resource;
		CString filename;
		INTERNET_PORT port = 0;
		bool secure = false;
	};

	bool CrackUri(const CString& uri, CUriParts& parts)
	{
		URL_COMPONENTS components = {};
		components.dwStructSize = sizeof(components);
		components.dwSchemeLength = static_cast<DWORD>(-1);
		components.dwHostNameLength = static_cast<DWORD>(-1);
		components.dwUrlPathLength = static_cast<DWORD>(-1);
		components.dwExtraInfoLength = static_cast<DWORD>(-1);
		if (!WinHttpCrackUrl(uri, uri.GetLength(), 0, &components) ||
			(components.nScheme != INTERNET_SCHEME_HTTP &&
				components.nScheme != INTERNET_SCHEME_HTTPS)) {
			return false;
		}

		parts.host.SetString(components.lpszHostName, components.dwHostNameLength);
		parts.resource.SetString(components.lpszUrlPath, components.dwUrlPathLength);
		if (components.dwExtraInfoLength) {
			parts.resource.Append(components.lpszExtraInfo, components.dwExtraInfoLength);
		}
		parts.port = components.nPort;
		parts.secure = components.nScheme == INTERNET_SCHEME_HTTPS;

		CString path(components.lpszUrlPath, components.dwUrlPathLength);
		const int slash = max(path.ReverseFind(L'/'), path.ReverseFind(L'\\'));
		parts.filename = path.Mid(slash + 1);
		for (int i = 0; i < parts.filename.GetLength(); ++i) {
			if (wcschr(L"<>:\"/\\|?*", parts.filename[i])) {
				parts.filename.SetAt(i, L'_');
			}
		}
		if (parts.filename.IsEmpty() || parts.filename == L"." || parts.filename == L"..") {
			parts.filename = L"download";
		}
		return !parts.host.IsEmpty();
	}

	bool OpenRequest(const CUriParts& uri, LPCWSTR verb, CInternetHandle& session,
		CInternetHandle& connection, CInternetHandle& request)
	{
		session.Reset(WinHttpOpen(
			L"STEP Viewer BCF",
			WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
			WINHTTP_NO_PROXY_NAME,
			WINHTTP_NO_PROXY_BYPASS,
			0));
		if (!session) {
			return false;
		}

		connection.Reset(WinHttpConnect(session, uri.host, uri.port, 0));
		if (!connection) {
			return false;
		}

		request.Reset(WinHttpOpenRequest(
			connection,
			verb,
			uri.resource,
			nullptr,
			WINHTTP_NO_REFERER,
			WINHTTP_DEFAULT_ACCEPT_TYPES,
			uri.secure ? WINHTTP_FLAG_SECURE : 0));
		if (!request ||
			!WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
				WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
			!WinHttpReceiveResponse(request, nullptr)) {
			return false;
		}

		DWORD status = 0;
		DWORD size = sizeof(status);
		return WinHttpQueryHeaders(request,
			WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
			WINHTTP_HEADER_NAME_BY_INDEX,
			&status,
			&size,
			WINHTTP_NO_HEADER_INDEX) &&
			status >= 200 && status < 300;
	}

	bool GetRemoteLastModified(const CUriParts& uri, FILETIME& modified)
	{
		CInternetHandle session;
		CInternetHandle connection;
		CInternetHandle request;
		if (!OpenRequest(uri, L"HEAD", session, connection, request)) {
			return false;
		}

		SYSTEMTIME systemTime = {};
		DWORD size = sizeof(systemTime);
		return WinHttpQueryHeaders(request,
			WINHTTP_QUERY_LAST_MODIFIED | WINHTTP_QUERY_FLAG_SYSTEMTIME,
			WINHTTP_HEADER_NAME_BY_INDEX,
			&systemTime,
			&size,
			WINHTTP_NO_HEADER_INDEX) &&
			SystemTimeToFileTime(&systemTime, &modified);
	}

	CString GetCachePath(const CString& folder, const CString& filename)
	{
		CString path(folder);
		if (!path.IsEmpty() && path[path.GetLength() - 1] != L'\\') {
			path += L'\\';
		}
		path += filename;
		return path;
	}

	bool IsCurrentCacheFile(const CString& path, const CUriParts& uri)
	{
		WIN32_FILE_ATTRIBUTE_DATA attributes = {};
		FILETIME remoteTime = {};
		return GetFileAttributesEx(path, GetFileExInfoStandard, &attributes) &&
			GetRemoteLastModified(uri, remoteTime) &&
			CompareFileTime(&attributes.ftLastWriteTime, &remoteTime) >= 0;
	}

	bool Download(const CUriParts& uri, const CString& destination, CString& error)
	{
		CInternetHandle session;
		CInternetHandle connection;
		CInternetHandle request;
		if (!OpenRequest(uri, L"GET", session, connection, request)) {
			error.Format(L"Cannot open URI for download (error %lu).", GetLastError());
			return false;
		}

		CString folder(destination);
		const int slash = folder.ReverseFind(L'\\');
		folder = slash >= 0 ? folder.Left(slash) : CString();
		wchar_t temporary[MAX_PATH] = {};
		if (folder.IsEmpty() || !GetTempFileName(folder, L"bcf", 0, temporary)) {
			error.Format(L"Cannot create a temporary cache file (error %lu).", GetLastError());
			return false;
		}

		HANDLE output = CreateFile(temporary, GENERIC_WRITE, 0, nullptr, TRUNCATE_EXISTING,
			FILE_ATTRIBUTE_NORMAL, nullptr);
		if (output == INVALID_HANDLE_VALUE) {
			error.Format(L"Cannot open the temporary cache file (error %lu).", GetLastError());
			DeleteFile(temporary);
			return false;
		}

		bool ok = true;
		BYTE buffer[64 * 1024];
		for (;;) {
			DWORD read = 0;
			if (!WinHttpReadData(request, buffer, sizeof(buffer), &read)) {
				error.Format(L"Failed while downloading the file (error %lu).", GetLastError());
				ok = false;
				break;
			}
			if (!read) {
				break;
			}
			DWORD written = 0;
			if (!WriteFile(output, buffer, read, &written, nullptr) || written != read) {
				error.Format(L"Cannot write the cache file (error %lu).", GetLastError());
				ok = false;
				break;
			}
		}

		if (ok) {
			SYSTEMTIME systemTime = {};
			DWORD size = sizeof(systemTime);
			FILETIME modified = {};
			if (WinHttpQueryHeaders(request,
					WINHTTP_QUERY_LAST_MODIFIED | WINHTTP_QUERY_FLAG_SYSTEMTIME,
					WINHTTP_HEADER_NAME_BY_INDEX,
					&systemTime,
					&size,
					WINHTTP_NO_HEADER_INDEX) &&
				SystemTimeToFileTime(&systemTime, &modified)) {
				SetFileTime(output, nullptr, nullptr, &modified);
			}
		}
		CloseHandle(output);

		if (ok && !MoveFileEx(temporary, destination,
				MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
			error.Format(L"Cannot replace the cached file (error %lu).", GetLastError());
			ok = false;
		}
		if (!ok) {
			DeleteFile(temporary);
		}
		return ok;
	}
}

CUriDownloader::CUriDownloader(CWnd* parent)
	: m_parent(parent)
{
}

bool CUriDownloader::IsUri(const CString& path)
{
	CUriParts parts;
	return CrackUri(path, parts);
}

CString CUriDownloader::GetLocalPath(const CString& uri) const
{
	CUriParts parts;
	if (!CrackUri(uri, parts)) {
		return uri;
	}

	CWinApp* app = AfxGetApp();
	const CString savedFolder = app
		? app->GetProfileString(PROFILE_SECTION, PROFILE_CACHE_FOLDER)
		: CString();
	if (!savedFolder.IsEmpty()) {
		const CString cachedPath = GetCachePath(savedFolder, parts.filename);
		if (IsCurrentCacheFile(cachedPath, parts)) {
			return cachedPath;
		}
	}

	CString question;
	question.Format(L"The BIM file is referenced by URI:\n\n%s\n\nDo you want to download it?",
		uri.GetString());
	if (AfxMessageBox(question, MB_YESNO | MB_ICONQUESTION) != IDYES) {
		return CString();
	}

	CFolderPickerDialog folderDialog(savedFolder,
		OFN_PATHMUSTEXIST | OFN_DONTADDTORECENT, m_parent);
	if (folderDialog.DoModal() != IDOK) {
		return CString();
	}

	const CString folder = folderDialog.GetPathName();
	if (app && !app->WriteProfileString(PROFILE_SECTION, PROFILE_CACHE_FOLDER, folder)) {
		AfxMessageBox(L"Cannot save the BCF URI cache folder.", MB_OK | MB_ICONWARNING);
	}

	const CString cachedPath = GetCachePath(folder, parts.filename);
	if (IsCurrentCacheFile(cachedPath, parts)) {
		return cachedPath;
	}

	CString error;
	if (!Download(parts, cachedPath, error)) {
		CString message;
		message.Format(L"Failed to download:\n\n%s\n\n%s", uri.GetString(), error.GetString());
		AfxMessageBox(message, MB_OK | MB_ICONERROR);
		return CString();
	}
	return cachedPath;
}
