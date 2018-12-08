#pragma once

#include <thread>
#include <queue>
#include <mutex>
#include <atomic>
#include <future>
#include <condition_variable>

#include "WebGraph.h"
#include "IWebPageDownloader.h"

#ifdef DEBUG
	#include <fstream>
#endif // DEBUG

namespace web_graph
{

class AsyncWebGraphBuilder
{
public:
	AsyncWebGraphBuilder(const network::IWebPageDownloaderFactory& factory, size_t maxThreads);
	~AsyncWebGraphBuilder();

	bool SetProxy(const network::ProxySettings& proxySettings);

	std::future<std::unique_ptr<WebGraph>> Start(const Url& rootUrl);
	bool IsRunning() const noexcept;
	void Stop();

private:
	void DownloadCycle();
	void ParseCycle();
	bool CanDownloadNextPage() noexcept;
	void UpdateGraphCompleted() noexcept;

private:
	std::unique_ptr<WebGraph> m_graph;
	std::queue<WebPageNode*> m_pagesToDownload;
	std::queue<std::pair<WebPageNode*, std::string>> m_pagesToParse;
	std::list<std::unique_ptr<network::IWebPageDownloader>> m_freeDownloaders;

	std::list<std::thread> m_threads;
	std::atomic_bool m_running{ false };
	std::atomic_bool m_needsToStop{ false };
	std::atomic_bool m_graphCompleted{ false };

	std::string m_rootUrl;
	std::promise<std::unique_ptr<WebGraph>> m_promise;

	mutable std::mutex m_urlMutex;
	std::condition_variable m_downloadCv;
	std::condition_variable m_parseCv;

#ifdef DEBUG
	std::ofstream m_outFile;
#endif // DEBUG
};

}// web_graph