#include "CurlWebPageDownloader.h"

#include <regex>
#include <stdexcept>

namespace network
{

static size_t WriteCallback(void* contents, size_t size, size_t count, void* userData)
{
	std::string* str{ reinterpret_cast<std::string*>(userData) };
	size_t len{ size * count };
	str->append(reinterpret_cast<char*>(contents), len);
	return len;
}

//

template<typename T>
void SetOptionOrThrow(CURL* curl, CURLoption option, T&& value )
{
	CURLcode res{ curl_easy_setopt(curl, option, std::forward<T>(value)) };
	if (res != CURLE_OK)
	{
		throw std::logic_error{ curl_easy_strerror(res) };
	}
}

CurlWebPageDownloader::CurlWebPageDownloader()
{
	m_curl.reset(curl_easy_init());
	if (!m_curl)
	{
		throw std::logic_error{ "Failed to init curl" };
	}

	SetOptionOrThrow(m_curl.get(), CURLOPT_WRITEFUNCTION, WriteCallback);
	SetOptionOrThrow(m_curl.get(), CURLOPT_SSL_VERIFYPEER, 0L);
	SetOptionOrThrow(m_curl.get(), CURLOPT_SSL_VERIFYHOST, 0L);
	SetOptionOrThrow(m_curl.get(), CURLOPT_USERAGENT, "libcurl-agent/1.0");
}

void CurlWebPageDownloader::SetProxy(const ProxySettings& proxySettings)
{
	if (proxySettings.proxyUrl.empty())
	{
		throw std::invalid_argument{ "Invalid proxy address" };
	}

	std::string proxy{ proxySettings.proxyUrl + ":" + std::to_string(proxySettings.proxyPort) };
	SetOptionOrThrow(m_curl.get(), CURLOPT_PROXY, proxy.c_str());

	if (!proxySettings.user.empty() && !proxySettings.password.empty())
	{
		std::string credentials{ proxySettings.user + ":" + proxySettings.password };
		SetOptionOrThrow(m_curl.get(), CURLOPT_PROXYUSERPWD, credentials.c_str());
	}
}

WebPageDownloadResult CurlWebPageDownloader::DownloadPage(const std::string& url)
{
	if (url.empty())
	{
		throw std::invalid_argument{ "Invalid url" };
	}

	WebPageDownloadResult result;

	CURLcode res{ curl_easy_setopt(m_curl.get(), CURLOPT_URL, url.c_str()) };
	if (res != CURLE_OK)
	{
		result.error = curl_easy_strerror(res);
		return result;
	}

	res = curl_easy_setopt(m_curl.get(), CURLOPT_WRITEDATA, &result.data);
	if (res != CURLE_OK)
	{
		result.error = curl_easy_strerror(res);
		return result;
	}

	res = curl_easy_perform(m_curl.get());
	if (res != CURLE_OK)
	{
		result.error = curl_easy_strerror(res);
		return result;
	}

	return result;
}

std::unique_ptr<IWebPageDownloader> CurlWebDownloaderFactory::Create() const
{
	return std::make_unique<CurlWebPageDownloader>();
}

}//namespace network