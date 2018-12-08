#pragma once

#include <string>
#include <memory>

namespace network
{

struct ProxySettings
{
	std::string proxyUrl;
	uint16_t proxyPort;
	std::string user;
	std::string password;
};

struct WebPageDownloadResult
{
	std::string data;
	std::string error;
};

class IWebPageDownloader
{
public:
	virtual void SetProxy(const ProxySettings& proxySettings) = 0;
	virtual WebPageDownloadResult DownloadPage(const std::string& url) = 0;
};

struct IWebPageDownloaderFactory
{
	virtual std::unique_ptr<IWebPageDownloader> Create() const = 0;
};

}// namespace network
