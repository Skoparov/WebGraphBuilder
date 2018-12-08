#include "WebGraphBuilder.h"

#include <cctype>
#include <regex>
#include <iostream>
#include <unordered_set>

namespace web_graph
{

bool GetExtension(const Url& url, std::string& extension)
{
	auto it = url.rfind('.');
	if (it != std::string::npos)
	{
		extension = url.substr(it);
		return true;
	}

	return false;
}

bool IsFile(const std::string& url)
{
	static const std::unordered_set<std::string> extentions
	{
		".jpg",
		".jpeg",
		".js",
		".ico",
		".js",
		".css",
		".png",
		".pdf",
		".rar",
		".zip",
		".doc",
		".docx",
		".xls",
		".xlsx",
		".pdf",
		".mp3",
		".djvu",
		".rtf",
		".ppt",
		".txt",
		".pptx",
		".gz",
		".gif",
		".xml",
		".tif",
		".tiff",
		".flv",
		".avi",
		".mp3",
		".mkv",
		".flac",
		".ogg",
		".mp4",
		".exe",
		".msi",
		".deb",
		".zip.001",
		".zip.002",
		".svg",
		".odt",
		".7z",
		".ppsx"
	};

	std::string extension;
	return GetExtension(url, extension) ?
		!!extentions.count(extension) :
		false;
}

constexpr bool IsAlNum(const char c) noexcept
{
	return ('a' <= c && c <= 'z') || ('0' <= c && c <= '9');
}

bool IsRootOrInvalid(const Url& url)
{
	static const std::string mailPrefix{ "mailto:" };

	return
	(
		url.empty() ||
		url == "/" ||
		(url.front() != '/' && !IsAlNum(url.front())) ||
		url.compare(0, mailPrefix.size(), mailPrefix) == 0
	);
}

void ToAbsoluteLink(Url& url, const Url& rootUrl)
{
	if (url.front() == '/')
	{
		// Relative link, concat with root
		url = rootUrl + url;
	}
}

bool IsHttpUrl(const Url& url)
{
	static const std::string httpPrefix{ "http:/" };
	static const std::string httpsPrefix{ "https:/" };
	return (url.compare(0, httpPrefix.size(), httpPrefix) == 0 ||
			url.compare(0, httpsPrefix.size(), httpsPrefix) == 0);
}

void StripUrlAdditions(Url& url)
{
	static const std::list<char> delimeters{ '#', ';', '&' };
	for (char c : delimeters)
	{
		auto it = url.find(c);
		if (it != std::string::npos)
		{
			url.erase(it, url.length() - it);
		}
	}
}

void StripWebPrefixes(Url& url)
{
	static const std::list<std::string> prefixes{ "http://", "https://", "www." };
	for (const std::string& prefix : prefixes)
	{
		if (url.find(prefix) == 0)
		{
			url.erase(0, prefix.length());
		}
	}
}

Url TrimUrl(const Url& url)
{
	std::string urlTrimmed{ url };
	if (url.back() == '/')
	{
		urlTrimmed.pop_back();
	}

	return urlTrimmed;
}

bool InDomain(const Url& url, const Url& strippedRootUrl)
{
	auto it = url.find(strippedRootUrl);
	return (
		it != std::string::npos &&
		it > 0 &&
		(url.at(it - 1) == '.' || url.at(it - 1) == '/'));
}

void DecodeUrl(web_graph::Url& url)
{
	Url decodedUrl;

	for (size_t i{ 0 }; i < url.length(); ++i)
	{
		if (url[i] == '%')
		{
			int charVal;
			sscanf(url.substr(i + 1, 2).c_str(), "%x", &charVal);
			decodedUrl += static_cast<char>(charVal);
			i += 2;
		}
		else
		{
			decodedUrl += url[i];
		}
	}

	url = decodedUrl;
}

void RemoveInvalidSymbols(std::string& url)
{
	url.erase(std::remove(url.begin(), url.end(), '"'), url.end());
	url.erase(std::remove(url.begin(), url.end(), '”'), url.end());
	url.erase(std::remove(url.begin(), url.end(), '\''), url.end());
	url.erase(std::remove(url.begin(), url.end(), '&'), url.end());
}

std::list<Url> GetValidHyperLinks(std::string&& html, const Url& rootUrl, const Url& strippedRootUrl)
{
	static const std::regex hl_regex{ "<a href=\"(.*?)\"", std::regex_constants::icase };
	std::list<Url> urls{
		std::sregex_token_iterator{ html.begin(), html.end(), hl_regex, 1 },
		std::sregex_token_iterator{} };

	auto it = urls.begin();
	while (it != urls.end())
	{
		Url& url = *it;
		std::transform(url.begin(), url.end(), url.begin(), ::tolower);

		if (IsRootOrInvalid(url) || IsFile(url))
		{
			urls.erase(it++);
		}
		else
		{
			ToAbsoluteLink(url, rootUrl);

			if (!IsHttpUrl(url) || !InDomain(url, strippedRootUrl))
			{
				urls.erase(it++);
				continue;
			}

			StripUrlAdditions(url);
			RemoveInvalidSymbols(url);
			DecodeUrl(url);

			++it;
		}
	}

	return urls;
}

//

AsyncWebGraphBuilder::AsyncWebGraphBuilder(const network::IWebPageDownloaderFactory& factory, size_t maxThreads)
{
	if (!maxThreads)
	{
		throw std::invalid_argument{ "Number of threads should be positive" };
	}

	for (size_t i{ 0 }; i < maxThreads; ++i)
	{
		m_freeDownloaders.emplace_back(factory.Create());
	}

}

AsyncWebGraphBuilder::~AsyncWebGraphBuilder()
{
	try
	{
		Stop();
	}
	catch (const std::exception& e)
	{
		std::cout << "Failed to stop web graph build: " << e.what() << "\n";
	}
}

bool AsyncWebGraphBuilder::SetProxy(const network::ProxySettings& proxySettings)
{
	std::lock_guard<std::mutex> l{ m_urlMutex };
	if (!m_running)
	{
		for (auto& downloader : m_freeDownloaders)
		{
			downloader->SetProxy(proxySettings);
		}

		return true;
	}

	return false;
}

std::future<std::unique_ptr<WebGraph>> AsyncWebGraphBuilder::Start(const Url& rootUrl)
{
	if (rootUrl.empty())
	{
		throw std::invalid_argument{ "Url should not be empty" };
	}

	std::lock_guard<std::mutex> l{ m_urlMutex };

	if (m_running)
	{
		throw std::logic_error{ "Already running" };
	}

	m_pagesToDownload = {};
	m_pagesToParse = {};
	m_graphCompleted = false;

	m_rootUrl = TrimUrl(rootUrl);
	DecodeUrl(m_rootUrl);
	RemoveInvalidSymbols(m_rootUrl);

	m_graph = std::make_unique<WebGraph>(CreateWebGraph(m_rootUrl));
	m_pagesToDownload.push(GetRoot(*m_graph));

	StripWebPrefixes(m_rootUrl);
	for (size_t i{ 0 }; i < m_freeDownloaders.size(); ++i)
	{
		m_threads.emplace_back(std::thread{ &AsyncWebGraphBuilder::DownloadCycle, this });
	}

	m_threads.emplace_back(std::thread{ &AsyncWebGraphBuilder::ParseCycle, this });

	m_downloadCv.notify_one();
	m_running = true;

#ifdef DEBUG
	m_outFile.open("web_graph_meta.txt");
#endif

	m_promise = std::promise<std::unique_ptr<WebGraph>>{};
	return m_promise.get_future();
}

bool AsyncWebGraphBuilder::IsRunning() const noexcept
{
	return m_running;
}

void AsyncWebGraphBuilder::Stop()
{
	m_needsToStop = true;
	m_downloadCv.notify_all();

	for (std::thread& t : m_threads)
	{
		if (t.joinable())
		{
			t.join();
		}
	}

	m_threads.clear();
	if (m_running)
	{
		m_promise.set_exception(
			std::make_exception_ptr(std::logic_error{ "Building aborted" }));

		m_running = false;
	}
}

void AsyncWebGraphBuilder::DownloadCycle()
{
	std::unique_ptr<network::IWebPageDownloader> downloader;
	WebPageNode* currNode{ nullptr };

	while (!m_graphCompleted && !m_needsToStop)
	{
		try
		{
			{
				std::unique_lock<std::mutex> l{ m_urlMutex };
				m_downloadCv.wait(l, [&]
				{
					return CanDownloadNextPage() || m_graphCompleted || m_needsToStop;
				});

				if (m_needsToStop || m_graphCompleted)
				{
					return;
				}

				downloader = std::move(m_freeDownloaders.front());
				m_freeDownloaders.pop_front();

				currNode = m_pagesToDownload.front();
				m_pagesToDownload.pop();
			}

			network::WebPageDownloadResult res = downloader->DownloadPage(GetNodeUrl(*currNode));

			std::lock_guard<std::mutex> l{ m_urlMutex };
			if (res.error.empty())
			{
				m_pagesToParse.push({ currNode, std::move(res.data) });
				m_parseCv.notify_one();
			}
			else
			{
				std::cerr << "Failed to download page " << GetNodeUrl(*currNode) << ": " << res.error << '\n';
			}

			m_freeDownloaders.push_back(std::move(downloader));

			UpdateGraphCompleted();
			if (m_graphCompleted)
			{
				m_downloadCv.notify_all();
				m_parseCv.notify_one();
			}
		}
		catch (const std::exception& e)
		{
			std::cerr << "Failed to download page " << GetNodeUrl(*currNode) << ": " << e.what() << '\n';
		}
	}
}

void AsyncWebGraphBuilder::ParseCycle()
{
	while (!m_needsToStop)
	{
		WebPageNode* pageNode{ nullptr };
		std::string pageData;

		try
		{
			{
				std::unique_lock<std::mutex> l{ m_urlMutex };
				m_parseCv.wait(l, [&]
				{
					return (!m_pagesToParse.empty() || m_graphCompleted || m_needsToStop);
				});

				if (m_needsToStop || m_graphCompleted)
				{
					m_promise.set_value(std::move(m_graph));
					m_running = false;
					return;
				}

				pageNode = m_pagesToParse.front().first;
				pageData = std::move(m_pagesToParse.front().second);
			}

			std::list<Url> urls{
				GetValidHyperLinks(std::move(pageData), GetNodeUrl(*GetRoot(*m_graph)), m_rootUrl) };

			std::lock_guard<std::mutex> l{ m_urlMutex };

			for (const Url& url : urls)
			{
				WebPageNode* node{ GetNode(*m_graph, url) };
				if (node)
				{
					// Already downloaded, just update links
					AddLink(*m_graph, *node, *pageNode);
				}
				else
				{
					WebPageNode& linkedNode = AddLink(*m_graph, url, *pageNode);
					m_pagesToDownload.push(&linkedNode);
					m_downloadCv.notify_one();
#ifdef DEBUG
					m_outFile
						<< std::this_thread::get_id() << " "
						<< m_freeDownloaders.size() << " "
						<< m_pagesToDownload.size() << " "
						<< GetNodeUrl(linkedNode) << << '\n';
#endif
				}
			}

			m_pagesToParse.pop();

			UpdateGraphCompleted();
			if (m_graphCompleted)
			{
				m_downloadCv.notify_all();
			}
		}
		catch (const std::exception& e)
		{
			std::cerr << "Failed to parse page " << GetNodeUrl(*pageNode) << ": " << e.what() << '\n';
		}
	}
}

bool AsyncWebGraphBuilder::CanDownloadNextPage() noexcept
{
	return (!m_pagesToDownload.empty() && !m_freeDownloaders.empty());
}

void AsyncWebGraphBuilder::UpdateGraphCompleted() noexcept
{
	m_graphCompleted =
		m_pagesToDownload.empty() && // no pages to download
		m_pagesToParse.empty(); // no pages to parse
		(m_freeDownloaders.size() == m_threads.size()); // all downloads finished
}

}