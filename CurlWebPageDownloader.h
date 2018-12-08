#pragma once

#include "IWebPageDownloader.h"

#include <memory>
#include <curl/curl.h>

namespace network
{

class CurlWebPageDownloader : public IWebPageDownloader
{
public:
	CurlWebPageDownloader();
	CurlWebPageDownloader(CurlWebPageDownloader&& other) = default;
	CurlWebPageDownloader& operator=(CurlWebPageDownloader&&) = default;

	void SetProxy(const ProxySettings& proxySettings) override;
	WebPageDownloadResult DownloadPage(const std::string& url) override;

private:
	std::unique_ptr<CURL, void(*)(CURL*)> m_curl{
		nullptr,
		[](CURL* c) {curl_easy_cleanup(c); } };
};

struct CurlWebDownloaderFactory : public IWebPageDownloaderFactory
{
	std::unique_ptr<IWebPageDownloader> Create() const override;
};

}// namespace network