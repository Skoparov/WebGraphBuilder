#include "GraphmlSerialization.h"

#include <regex>
#include <fstream>

#include "Common.h"

namespace graphml
{

void Serialize(const web_graph::WebGraph& graph, const std::string& outFilePath)
{
	using namespace common;
	using namespace web_graph;

	std::ofstream outFile{ outFilePath };
	if (!outFile.is_open())
	{
		throw std::runtime_error{ "Failed to open file" };
	}

	outFile << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
		<< "<graphml xmlns=\"http://graphml.graphdrawing.org/xmlns\">\n"
		<< "    <graph id=\"WebSiteGraph\" edgedefault=\"directed\">\n";

	const Nodes& nodes = GetNodes(graph);
	for (const auto& node : nodes)
	{
		const WebPageNode* currNode{ node.second.get() };
		if (!NodeMarkedAsDeleted(*currNode))
		{
			outFile << "        <node id=\"" << GetNodeUrl(*node.second) << "\"/>\n";
		}
	}

	for (const auto& node : nodes)
	{
		const WebPageNode& currNode = *node.second.get();
		if (!NodeMarkedAsDeleted(currNode))
		{
			auto outLinkNodes = GetOutboundNodeLinks(*node.second);
			for (auto outNodeLinksInfo : outLinkNodes)
			{
				if (!NodeMarkedAsDeleted(*outNodeLinksInfo.first))
				{
					for (size_t i{ 0 }; i < outNodeLinksInfo.second; ++i)
					{
						outFile << "        <edge source=\"" << GetNodeUrl(*node.second) << "\""
							<< " target=\"" << GetNodeUrl(*outNodeLinksInfo.first) << "\"/>\n";
					}
				}
			}
		}
	}

	outFile << "    </graph>\n" << "</graphml>";
}

void AddNode(std::unique_ptr<web_graph::WebGraph>& graph, const std::smatch& match)
{
	using namespace web_graph;

	if (match.size() != 2)
	{
		throw std::invalid_argument{ "Corrupted graphml: invalid node match" };
	}

	std::string nodeUrl{ match[1] };
	if (graph)
	{
		AddNode(*graph, nodeUrl);
	}
	else
	{
		graph = std::make_unique<WebGraph>(CreateWebGraph(nodeUrl));
	}
}

void AddLink(web_graph::WebGraph& graph, const std::smatch& match)
{
	using namespace web_graph;

	if (match.size() != 3)
	{
		throw std::invalid_argument{ "Corrupted graphml: invalid link match" };
	}

	std::string fromUrl{ match[1] };
	WebPageNode* fromNode{ GetNode(graph, fromUrl) };
	if (!fromNode)
	{
		throw std::runtime_error{ "Corrupted graphml: source node not found" };
	}

	std::string toUrl{ match[2] };
	WebPageNode* toNode{ GetNode(graph, toUrl) };
	if (!toNode)
	{
		throw std::runtime_error{ "Corrupted graphml: dest node not found" };
	}

	AddLink(graph, *toNode, *fromNode);
}

std::unique_ptr<web_graph::WebGraph> Deserialize(const std::string & filePath)
{
	using namespace web_graph;

	std::ifstream inFile{ filePath };
	if (!inFile.is_open())
	{
		return false;
	}

	static const std::regex nodeRegex{ "<node id=\"(\\S+)\"/>" };
	static const std::regex edgeRegex{ "<edge source=\"(\\S+)\" target=\"(\\S+)\"/>" };
	std::smatch match;

	std::string line;
	bool edgesStarted{ false };

	std::unique_ptr<web_graph::WebGraph> graph;
	while (std::getline(inFile, line))
	{
		if (!edgesStarted && std::regex_search(line, match, nodeRegex))
		{
			AddNode(graph, match);
		}
		else if (std::regex_search(line, match, edgeRegex))
		{
			if (!graph)
			{
				throw std::runtime_error{ "Corrupted graphml : no nodes added but egges found" };
			}

			if (!edgesStarted)
			{
				edgesStarted = true;
			}

			AddLink(*graph, match);
		}
	}

	return graph;
}

}// graphml