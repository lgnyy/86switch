
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#include <Windows.h>
#include <winhttp.h>
#pragma comment(lib, "Winhttp.lib")

#include "../include/yos_http.h"

#include <stdio.h>
#define LOGI(fmt,...) fprintf(stderr, fmt, ##__VA_ARGS__)
#define LOGE(fmt,...) fprintf(stdout, fmt, ##__VA_ARGS__)

typedef struct yos_http_context
{
	HINTERNET hSession;
	HINTERNET hConnect;
	HINTERNET hRequest;
	LPWSTR url;
	LPWSTR custom_header;
	uint8_t ssl_enable;
	uint8_t* response_header;
	uint8_t* response_content;
	int32_t (*on_header_set_cookie_cb)(void* arg, const char* value);
	void* on_header_set_cookie_arg;
}yos_http_context_t;

static LPWSTR utf8_to_unicode(const uint8_t* str, uint32_t str_len)
{
	int w_len = MultiByteToWideChar(CP_UTF8, 0, (LPCCH)str, str_len, NULL, 0);
	LPWSTR w = (LPWSTR)malloc((w_len + 1) * sizeof(WCHAR));
	if (w != NULL)
	{
		MultiByteToWideChar(CP_UTF8, 0, (LPCCH)str, str_len, w, w_len + 1);
		w[w_len] = 0;
	}
	return w;
}

static uint8_t* unicode_to_utf8(LPCWSTR str, uint32_t str_len)
{
	int c_len = WideCharToMultiByte(CP_UTF8, 0, str, str_len, NULL, 0, NULL, NULL);
	LPSTR c = (LPSTR)malloc((c_len + 1));
	if (c != NULL)
	{
		WideCharToMultiByte(CP_UTF8, 0, str, str_len, c, c_len + 1, NULL, NULL);
		c[c_len] = 0;
	}
	return (uint8_t*)c;

}


yos_http_handle_t yos_http_create(const char* url, const char* ca_cert)
{
	yos_http_context_t* ctx = (yos_http_context_t*)malloc(sizeof(yos_http_context_t));
	if (ctx != NULL)
	{
		memset(ctx, 0, sizeof(yos_http_context_t));

		ctx->hSession = WinHttpOpen(NULL, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
		if (NULL == ctx->hSession)
		{
			LOGE("WinHttpOpen err=%d\n", GetLastError());
			free(ctx);
			return NULL;
		}

		ctx->url = utf8_to_unicode(url, -1);
	}
	return ctx;
}

int32_t yos_http_destory(yos_http_handle_t ctx)
{
	if (NULL != ctx->hRequest)
	{
		WinHttpCloseHandle(ctx->hRequest);
	}

	if (NULL != ctx->hConnect)
	{
		WinHttpCloseHandle(ctx->hConnect);
	}

	if (NULL != ctx->hSession)
	{
		WinHttpCloseHandle(ctx->hSession);
	}

	if (NULL != ctx->url)
	{
		free(ctx->url);
	}

	if (ctx->response_content != NULL)
	{
		free(ctx->response_content);
	}

	if (ctx->response_header != NULL)
	{
		free(ctx->response_header);
	}

	//memset(ctx, 0, sizeof(yos_http_context_t));
	free(ctx);
	return 0;
}


int32_t yos_http_set_timeout(yos_http_handle_t ctx, uint32_t conn_timeout, uint32_t rsp_timeout)
{
	if (!WinHttpSetTimeouts(ctx->hSession, conn_timeout, conn_timeout, rsp_timeout, rsp_timeout))
	{
		DWORD dwError = GetLastError();
		LOGE("WinHttpSetTimeouts err=%d\n", dwError);
		return HRESULT_FROM_WIN32(dwError);
	}

	return 0;
}

int32_t yos_http_set_url(yos_http_handle_t ctx, const char* url)
{
	if ((ctx == NULL) || (NULL == url))
	{
		return E_INVALIDARG;
	}

	if (ctx->url != NULL)
	{
		free(ctx->url);
	}
	ctx->url = utf8_to_unicode(url, -1);
	return 0;
}

int32_t yos_http_set_headers(yos_http_handle_t ctx, const char* headers)
{
	if ((ctx == NULL) || (NULL == headers))
	{
		return E_INVALIDARG;
	}

	if (ctx->custom_header != NULL)
	{
		free(ctx->custom_header);
	}
	ctx->custom_header = utf8_to_unicode(headers, -1);
	return 0;
}

int32_t yos_http_set_header(yos_http_handle_t ctx, const char* key, const char* value)
{
	if ((ctx == NULL) || (NULL == key) || (NULL == value))
	{
		return E_INVALIDARG;
	}
	char* kv = (char*)malloc(strlen(key) + 4 + strlen(value));
	if (kv == NULL)
	{
		return HRESULT_FROM_WIN32(E_OUTOFMEMORY);
	}
	sprintf("%s:%s", key, value);

	if (ctx->custom_header == NULL) 
	{
		ctx->custom_header = utf8_to_unicode(kv, -1);
	}
	else
	{
		LPWSTR tmp = utf8_to_unicode(kv, -1);
		LPWSTR tmp2 = (LPWSTR)malloc((wcslen(ctx->custom_header) + 4 + wcslen(tmp)) * 2);
		wcscpy(tmp2, ctx->custom_header);
		wcscat(tmp2, L"\r\n");
		wcscat(tmp2, tmp);
		free(ctx->custom_header);
		ctx->custom_header = tmp2;
	}
	free(kv);
	return 0;
}

int32_t yos_http_get_header(yos_http_handle_t ctx, const char* key, char** value)
{
	if ((ctx == NULL) || (NULL == ctx->hRequest) || (value == NULL))
	{
		return E_INVALIDARG;
	}

	LPWSTR key_u = WINHTTP_HEADER_NAME_BY_INDEX;

	DWORD dwInfoLevel = (key && key[0])? WINHTTP_QUERY_CUSTOM : WINHTTP_QUERY_RAW_HEADERS_CRLF;
	if (dwInfoLevel == WINHTTP_QUERY_CUSTOM)
	{
		key_u = utf8_to_unicode((const uint8_t*)key, strlen(key));
	}
	DWORD dwValuesSize = 0;
	WinHttpQueryHeaders(ctx->hRequest, dwInfoLevel, key_u, NULL, &dwValuesSize, WINHTTP_NO_HEADER_INDEX);

	// Allocate memory for the buffer.
	if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
	{
		WCHAR* lpValueBuffer = (WCHAR*)malloc(dwValuesSize);
		WinHttpQueryHeaders(ctx->hRequest, dwInfoLevel, key_u, lpValueBuffer, &dwValuesSize, WINHTTP_NO_HEADER_INDEX);

		*value = unicode_to_utf8(lpValueBuffer, dwValuesSize);
		free(lpValueBuffer);
		free(key_u);
		return 0;
	}

	free(key_u);
	return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
}

int32_t yos_http_set_on_header_set_cookie_cb(yos_http_handle_t ctx, int32_t(*cb)(void* arg, const char* value), void* arg)
{
	if ((ctx == NULL))
	{
		return E_INVALIDARG;
	}
	ctx->on_header_set_cookie_cb = cb;
	ctx->on_header_set_cookie_arg = arg;
	return 0;
}

int32_t yos_http_request(yos_http_handle_t ctx, const uint8_t* data, uint32_t data_len, uint8_t** resp, uint32_t* resp_len)
{
	int32_t rv = 0;

	if ((ctx == NULL) || (NULL == ctx->hSession))
	{
		return E_INVALIDARG;
	}

	if (ctx->response_content != NULL)
	{
		free(ctx->response_content);
		ctx->response_content = NULL;
	}

	if (ctx->response_header != NULL)
	{
		free(ctx->response_header);
		ctx->response_header = NULL;
	}

	do
	{
		if (NULL == ctx->hConnect)
		{
			WCHAR wchScheme[64], wchHostName[128];
			URL_COMPONENTS urlCom;
			memset(&urlCom, 0, sizeof(urlCom));
			urlCom.dwStructSize = sizeof(urlCom);
			urlCom.lpszScheme = wchScheme;
			urlCom.dwSchemeLength = ARRAYSIZE(wchScheme);
			urlCom.lpszHostName = wchHostName;
			urlCom.dwHostNameLength = ARRAYSIZE(wchHostName);

			if (!WinHttpCrackUrl(ctx->url, lstrlenW(ctx->url), ICU_ESCAPE, &urlCom))
			{
				rv = HRESULT_FROM_WIN32(GetLastError());
				break;
			}
			ctx->ssl_enable = (urlCom.nScheme == INTERNET_SCHEME_HTTPS) ? 1 : 0;

			ctx->hConnect = WinHttpConnect(ctx->hSession, urlCom.lpszHostName, urlCom.nPort, 0);
			if (NULL == ctx->hConnect)
			{
				DWORD dwError = GetLastError();
				LOGE("WinHttpConnect err=%d\n", dwError);
				rv = HRESULT_FROM_WIN32(dwError);
				break;
			}
		}

		if (NULL != ctx->hRequest)
		{
			WinHttpCloseHandle(ctx->hRequest);
		}

		// TransmiteDataToServerByPost
		{
			LPCWSTR path_w = wcschr(ctx->url + 8, '/');
			LPCWSTR method_w = (data_len>0) ? L"POST" : L"GET";
			DWORD flags = ctx->ssl_enable ? WINHTTP_FLAG_SECURE : 0;
			ctx->hRequest = WinHttpOpenRequest(ctx->hConnect, method_w, path_w, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
			DWORD dwError = GetLastError();
			if (NULL == ctx->hRequest)
			{
				LOGE("WinHttpOpenRequest err=%d\n", dwError);
				rv = HRESULT_FROM_WIN32(dwError);
				break;
			}
		}

		{
			DWORD dwFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA | SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE | SECURITY_FLAG_IGNORE_CERT_CN_INVALID | SECURITY_FLAG_IGNORE_CERT_DATE_INVALID;
			//WinHttpSetOption(hRequest, WINHTTP_OPTION_CONNECT_TIMEOUT, &dwTimeOut, sizeof(DWORD));
			WinHttpSetOption(ctx->hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &dwFlags, sizeof(dwFlags));
			//WinHttpSetOption(hRequest, WINHTTP_OPTION_CLIENT_CERT_CONTEXT, WINHTTP_NO_CLIENT_CERT_CONTEXT, 0);
		}

		// --ModifyRequestHeader
		if (ctx->custom_header != NULL)
		{
			//LPCWSTR pwszNewHeader = L"Content-Type: application/json; charset=utf-8";
			WinHttpAddRequestHeaders(ctx->hRequest, ctx->custom_header, lstrlenW(ctx->custom_header), WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);
		}
		//cm_httpclient_request_type_e method;    //request type
#if 0
		if (!WinHttpSendRequest(ctx->hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, pdata, data_len, data_len, 0))
		{
			LOGE("WinHttpSendRequest err=%d\n", GetLastError());
			rv = CM_HTTP_RET_CODE_SEND_DATA_FAIL;
			break;
		}
#else
		if (!WinHttpSendRequest(ctx->hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, data_len, 0))
		{
			DWORD dwError = GetLastError();
			LOGE("WinHttpSendRequest err=%d\n", dwError);
			rv = HRESULT_FROM_WIN32(dwError);;
			break;
		}

		if (data_len && !WinHttpWriteData(ctx->hRequest, data, data_len, NULL))
		{
			DWORD dwError = GetLastError();
			LOGE("WinHttpWriteData err=%d\n", dwError);
			rv = HRESULT_FROM_WIN32(dwError);
			break;
		}
#endif

		// ReceiveData
		if (!WinHttpReceiveResponse(ctx->hRequest, NULL))
		{
			DWORD dwError = GetLastError();
			LOGE("WinHttpReceiveResponse err=%d\n", dwError);
			rv = HRESULT_FROM_WIN32(dwError);
			break;
		}

		if (0) // test
		{
			DWORD dwHeadersSize = 0;
			WinHttpQueryHeaders(ctx->hRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF, WINHTTP_HEADER_NAME_BY_INDEX, NULL, &dwHeadersSize, WINHTTP_NO_HEADER_INDEX);

			// Allocate memory for the buffer.
			if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
			{
				WCHAR* lpHeadersBuffer = (WCHAR*)malloc(dwHeadersSize);

				// Now, use WinHttpQueryHeaders to retrieve the header.
				WinHttpQueryHeaders(ctx->hRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF, WINHTTP_HEADER_NAME_BY_INDEX, lpHeadersBuffer, &dwHeadersSize, WINHTTP_NO_HEADER_INDEX);

				ctx->response_header = unicode_to_utf8(lpHeadersBuffer, dwHeadersSize / sizeof(WCHAR));
				LOGI(ctx->response_header);
				free(lpHeadersBuffer);
			}
			else
			{
				LOGE("last err:%d\n", GetLastError());
			}
		}

		if (ctx->on_header_set_cookie_cb != NULL) {
			for (DWORD i = 0; ; i++){
				DWORD dwValuesSize = 0;
				WinHttpQueryHeaders(ctx->hRequest, WINHTTP_QUERY_SET_COOKIE, WINHTTP_HEADER_NAME_BY_INDEX, NULL, &dwValuesSize, &i);

				// Allocate memory for the buffer.
				if (GetLastError() == ERROR_INSUFFICIENT_BUFFER){
					WCHAR* lpValueBuffer = (WCHAR*)malloc(dwValuesSize);
					WinHttpQueryHeaders(ctx->hRequest, WINHTTP_QUERY_SET_COOKIE, WINHTTP_HEADER_NAME_BY_INDEX, lpValueBuffer, &dwValuesSize, &i);

					char* value_utf8 = unicode_to_utf8(lpValueBuffer, dwValuesSize);
					ctx->on_header_set_cookie_cb(ctx->on_header_set_cookie_arg, value_utf8);

					free(value_utf8);
					free(lpValueBuffer);
				}
			}
		}

		DWORD statusCode = 0;
		DWORD statusCodeSize = sizeof(DWORD);

		if (!WinHttpQueryHeaders(ctx->hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusCodeSize, WINHTTP_NO_HEADER_INDEX))
		{
			LOGE("WinHttpQueryHeaders(WINHTTP_QUERY_STATUS_CODE) err=%d\n", GetLastError());
			//return -1;
		}
		LOGI("statusCode=%d\n", statusCode);

		{
			DWORD i, dwRead;
			DWORD dwContentLength = 0;

			dwRead = sizeof(DWORD);
			if (!WinHttpQueryHeaders(ctx->hRequest, WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &dwContentLength, &dwRead, WINHTTP_NO_HEADER_INDEX))
			{
				LOGE("WinHttpQueryHeaders(WINHTTP_QUERY_CONTENT_LENGTH) err=%d\n", GetLastError());
				//break;
			}

			ctx->response_content = (dwContentLength > 0) ? (uint8_t*)malloc(dwContentLength + 1) : NULL;
			for (i = 0; ;)
			{
				dwRead = 0;
				if (!WinHttpQueryDataAvailable(ctx->hRequest, &dwRead))
				{
					DWORD dwError = GetLastError();
					LOGE("WinHttpQueryDataAvailable err=%d\n", dwError);
					rv = HRESULT_FROM_WIN32(dwError);
					break;
				}
				if (dwRead == 0)
				{
					dwContentLength = i;
					break;
				}

				if (dwContentLength - i < dwRead)
				{
					dwContentLength += dwRead;
					ctx->response_content = (uint8_t*)realloc(ctx->response_content, dwContentLength + 1);
				}

				if (ctx->response_content == NULL)
				{
					rv = HRESULT_FROM_WIN32(E_OUTOFMEMORY);
					break;
				}

				if (!WinHttpReadData(ctx->hRequest, ctx->response_content + i, dwContentLength - i, &dwRead))
				{
					DWORD dwError = GetLastError();
					LOGE("WinHttpReadData err=%d\n", dwError);
					rv = HRESULT_FROM_WIN32(dwError);
					break;
				}
				i += dwRead;
			}

			if (rv == 0)
			{
				if (resp != NULL)
				{
					*resp = ctx->response_content;
				}
				if (resp_len != NULL)
				{
					*resp_len = dwContentLength;
				}

				if (ctx->response_content != NULL)
				{
					ctx->response_content[dwContentLength] = '\x0';
				}
			}
		}
	} while (0);


	return rv;
}

void* yos_http_static_malloc(uint32_t size)
{
	return malloc(size);
}

void yos_http_static_free(void* ptr)
{
	free(ptr);
}

/** POST请求 */
int32_t yos_http_static_request(const char* url, const char* ca_cert, const char* headers, const uint8_t* data, uint32_t data_len, uint8_t** resp, uint32_t* resp_len)
{
	yos_http_handle_t handle = yos_http_create(url, ca_cert);
	if (handle == NULL)
	{
		return E_OUTOFMEMORY;
	}

	if (headers != NULL)
	{
		yos_http_set_headers(handle, headers);
	}

	int32_t rv = yos_http_request(handle, data, data_len, resp, resp_len);
	if ((rv == 0) && (resp != NULL))
	{
		handle->response_content = NULL; // External release
	}

	yos_http_destory(handle);
	return rv;
}
