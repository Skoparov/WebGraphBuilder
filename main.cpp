#include <iostream>
#include <fstream>

#include "CurlWebPageDownloader.h"
#include "WebGraphBuilder.h"
#include "GraphmlSerialization.h"
#include "Analyze.h"

static constexpr auto GraphmlExt = ".graphml";
static constexpr auto GraphFileName = "graph.graphml";
static constexpr auto AnalysisResultFileName = "analysisResult.txt";

enum SettingsPos
{
	PosMode = 1,
	PosWorkDir,
	PosAddress,
	PosDeletionChance = PosAddress,
	PosProxyAddr,
	PosProxyPort,
	PosProxyUserName,
	PosProxyPassword
};

enum class WorkMode{ Crawl, CrawlAndAnalyze, ReadAndAnalyze, SimulateAtackAndAnalyze };

WorkMode StrToMode( const std::string& mode)
{
	if (mode == "crawl")
	{
		return  WorkMode::Crawl;
	}
	else if (mode == "crawl_and_analyze")
	{
		return  WorkMode::CrawlAndAnalyze;
	}
	else if (mode == "read_and_analyze")
	{
		return  WorkMode::ReadAndAnalyze;
	}
	else if (mode == "simulate_atack_and_analyze")
	{
		return  WorkMode::SimulateAtackAndAnalyze;
	}

	throw std::invalid_argument{ "Invalid work mode" };
}

struct Settings
{
	WorkMode mode;
	std::string workDir;
	web_graph::Url url;
	std::string proxyAddr;
	uint16_t proxyPort;
	std::string proxyUser;
	std::string proxyPassw;
	double deletionChance;
};

void PrintUsage()
{
	std::cout <<
		"Usage: ./WebGraphBuilder %mode(crawl/crawl_and_analyze/read_and_analyze/simulate_deletion_and_analyze)"
		"%input_output_file %url %proxy %proxy_username %proxy_password";
}

Settings ParseArgs(int argc, char** argv)
{
	--argc;
	if (argc < PosWorkDir)
	{
		PrintUsage();
		throw std::invalid_argument{ "At least mode and work directory should be provided" };
	}

	Settings settings;
	settings.mode = StrToMode(argv[PosMode]);
	settings.workDir = argv[PosWorkDir];

	if (settings.mode == WorkMode::Crawl || settings.mode == WorkMode::CrawlAndAnalyze)
	{
		if (argc < PosAddress)
		{
			PrintUsage();
			throw std::logic_error{ "Url should be provided for crawl mode" };
		}

		settings.url = argv[PosAddress];

		if (argc >= PosProxyAddr)
		{
			if (argc < PosProxyPort)
			{
				PrintUsage();
				throw std::invalid_argument{ "Both proxy addr and port should be provided" };
			}

			settings.proxyAddr = argv[PosProxyAddr];
			settings.proxyPort = (uint16_t)std::stoul(argv[PosProxyPort]);
		}

		if (argc >= PosProxyUserName)
		{
			if (argc <PosProxyPassword)
			{
				PrintUsage();
				throw std::invalid_argument{ "Both proxy username and password should be provided" };
			}

			settings.proxyUser = argv[PosProxyUserName];
			settings.proxyPassw = argv[PosProxyPassword];
		}
	}
	else if (settings.mode == WorkMode::SimulateAtackAndAnalyze)
	{
		if (argc < PosDeletionChance)
		{
			PrintUsage();
			throw std::logic_error{ "Deletion chance should be provided for deletion mode" };
		}

		settings.deletionChance = std::stod(argv[PosDeletionChance]);
	}
	else
	{
		throw std::invalid_argument{ "Unknown workmode" };
	}

	return settings;
}

std::string MakePath(const std::string& dir, const std::string& fileName)
{
	std::string resultPath{ dir };
	if (!resultPath.empty() && resultPath.back() != '/')
	{
		resultPath += '/';
	}

	resultPath += fileName;
	return resultPath;
}

void WriteAnalysisResultToFile(const analyze::GraphAnalysisResult& result, const std::string& fileName)
{
	std::ofstream outFile{ fileName };
	if (!outFile.is_open())
	{
		throw std::runtime_error{ "Failed to open file" };
	}

	outFile
		<< "edgesIndex: " << result.edgesIndex << '\n'
		<< "linksIndex: " << result.linksIndex << '\n'
		<< "clusteringCoeff: " << result.clusteringCoeff << '\n'
		<< "inductors: " << result.inductorNum << '\n'
		<< "collectors: " << result.collectorsNum << '\n'
		<< "mediators: " << result.mediatorsNum << '\n';
}

std::string MakeNameAfterAttack(double deletionChance, size_t iteration)
{
	return std::string{ "graph_del_chance_" } +
		std::to_string(deletionChance) + "_" +
		std::to_string(iteration) +
		GraphmlExt;
}

int main(int argc, char** argv)
{
	try
	{
		Settings settings = ParseArgs(argc, argv);

		std::string graphFileName{ MakePath(settings.workDir, GraphFileName) };
		std::string analysisFileName{ MakePath(settings.workDir, AnalysisResultFileName) };

		// Create graph if necessary
		if (settings.mode == WorkMode::Crawl || settings.mode == WorkMode::CrawlAndAnalyze)
		{
			network::CurlWebDownloaderFactory factory;
			web_graph::AsyncWebGraphBuilder builder{ factory, std::thread::hardware_concurrency() };

			if (!settings.proxyAddr.empty())
			{
				builder.SetProxy(
				{ settings.proxyAddr, settings.proxyPort, settings.proxyUser, settings.proxyPassw });
			}

			auto future = builder.Start(settings.url);
			auto graphHandle = future.get();
			const web_graph::WebGraph& graph = *graphHandle;

			graphml::Serialize(graph, graphFileName);
			if (settings.mode == WorkMode::CrawlAndAnalyze)
			{
				WriteAnalysisResultToFile(analyze::Analyze(graph), analysisFileName);
			}
		}

		// Analyze graph if necessary
		if (settings.mode == WorkMode::ReadAndAnalyze || settings.mode == WorkMode::SimulateAtackAndAnalyze)
		{
			auto graph = graphml::Deserialize(graphFileName);
			WriteAnalysisResultToFile(analyze::Analyze(*graph), analysisFileName);
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}

	return 0;
}

