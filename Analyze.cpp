#include "Analyze.h"

#include <set>
#include <queue>
#include <functional>
#include <cassert>
#include <random>

#include "Common.h"

namespace analyze
{

/*using NodeFunctor = std::function<bool(web_graph::WebPageNode&)>;

template<typename Functor>
void ForEachInGraph(const web_graph::WebGraph& graph, Functor&& func)
{
	using namespace web_graph;

	std::set<WebPageNode*> visited;
	std::queue<WebPageNode*> nodesToProcess;

	WebPageNode* root = GetRoot(graph);
	if (root)
	{
		nodesToProcess.push(root);
		while (!nodesToProcess.empty())
		{
			WebPageNode* currNode{ nodesToProcess.front() };
			nodesToProcess.pop();

			if (!visited.count(currNode))
			{
				visited.emplace(currNode);
				if (!func(*currNode))
				{
					break;
				}
			}
		}
	}
}*/

double CalcEdgesIndex(const web_graph::WebGraph& graph)
{
	using namespace web_graph;

	double nodesWithOneOrMoreInOutLinksNum{ 0 };
	const Nodes& nodes = GetNodes(graph);
	for (const auto& node : nodes)
	{
		if (!common::NodeMarkedAsDeleted(*node.second))
		{
			if (!GetInboundNodeLinks(*node.second).empty() ||
				!GetOutboundNodeLinks(*node.second).empty())
			{
				++nodesWithOneOrMoreInOutLinksNum;
			}
		}
	}

	return nodesWithOneOrMoreInOutLinksNum / nodes.size();
}

////

double CalcLinksIndex(size_t linksNum, size_t nodesNum) noexcept
{
	return nodesNum >= 1?
		static_cast<double>(linksNum) / (nodesNum * (nodesNum - 1)) : 0;
}

double CalcLinksIndex(const web_graph::WebGraph& graph)
{
	using namespace web_graph;
	return CalcLinksIndex(GetLinksNum(graph), GetNodesNum(graph));
}

double CalcLinkIndexForNode(const web_graph::WebPageNode& node)
{
	// link index of subgraph comprised the node and it's adjacent nodes

	using namespace web_graph;

	const NodeLinks& inLinks = GetInboundNodeLinks(node);
	const NodeLinks& outLinks = GetOutboundNodeLinks(node);
	const size_t subgraphNodesNum{ inLinks.size() + outLinks.size() + 1 };

	size_t subgraphLinksNum{ 0 };

	for (const auto& inNodeLinksInfo : inLinks)
	{
		subgraphLinksNum += inNodeLinksInfo.second;
	}

	for (const auto& outNodeLinksInfo : outLinks)
	{
		subgraphLinksNum += outNodeLinksInfo.second;
	}

	return CalcLinksIndex(subgraphLinksNum, subgraphNodesNum);
}

double CalcClusteringCoeff(const web_graph::WebGraph& graph)
{
	using namespace web_graph;

	size_t nodesWithTotalLinksNotLessThan2_Num{ 0 };
	double linkIndexSum{ 0.0 };

	const Nodes& nodes = GetNodes(graph);
	for (const auto& node : nodes)
	{
		if (!common::NodeMarkedAsDeleted(*node.second))
		{
			if (GetInboundNodeLinks(*node.second).size() +
				GetOutboundNodeLinks(*node.second).size() >= 2)
			{
				++nodesWithTotalLinksNotLessThan2_Num;

				linkIndexSum += CalcLinkIndexForNode(*node.second);
			}
		}
	}

	return nodesWithTotalLinksNotLessThan2_Num?
		static_cast<double>(linkIndexSum) / nodesWithTotalLinksNotLessThan2_Num :
		0.0;
}

size_t GetNodeLinksNum(const web_graph::NodeLinks& links)
{
	size_t result{ 0 };
	for (const auto& linkInfo : links)
	{
		result += linkInfo.second;
	}

	return result;
}

bool IsInductor(size_t inboundLinksNum, size_t outboundLinksNum) noexcept
{
	return inboundLinksNum * 1.5 <= outboundLinksNum;
}

bool IsCollector(size_t inboundLinksNum, size_t outboundLinksNum) noexcept
{
	return outboundLinksNum * 1.5 <= inboundLinksNum;
}

void GetNodesTypesNum(
	const web_graph::WebGraph & graph,
	size_t & inductorsNum,
	size_t & collectorsNum,
	size_t & mediatorsNum)
{
	using namespace web_graph;

	const Nodes& nodes = GetNodes(graph);
	for (const auto& node : nodes)
	{
		if (!common::NodeMarkedAsDeleted(*node.second))
		{
			size_t inboundLinksNum{ GetNodeLinksNum(GetInboundNodeLinks(*node.second)) };
			size_t outboundLinksNum{ GetNodeLinksNum(GetOutboundNodeLinks(*node.second)) };

			if (IsInductor(inboundLinksNum, outboundLinksNum))
			{
				++inductorsNum;
			}
			else if (IsCollector(inboundLinksNum, outboundLinksNum))
			{
				++collectorsNum;
			}
			else
			{
				++mediatorsNum;
			}
		}
	}
}

GraphAnalysisResult Analyze(const web_graph::WebGraph& graph)
{
	GraphAnalysisResult result{};
	result.linksIndex = CalcLinksIndex(graph);
	result.edgesIndex = CalcEdgesIndex(graph);
	result.clusteringCoeff = CalcClusteringCoeff(graph);
	GetNodesTypesNum(graph, result.inductorNum, result.collectorsNum, result.mediatorsNum);

	return result;
}

bool ShouldBeDeleted(double chance)
{
	if (chance == 1.0)
	{
		return true;
	}

	static std::mt19937 rng{ std::random_device{}() };
	static std::uniform_int_distribution<std::mt19937::result_type> generator{ 0, 100 };

	double value = generator(rng);
	value /= 100;

	return value <= chance;
}

void SimulateNodesDeletion(const web_graph::WebGraph& graph, double chance)
{
	if (chance < 0.0 || chance > 1.0)
	{
		throw std::invalid_argument{ "Chance should be within [0, 1]" };
	}

	if (chance != 0.0)
	{
		using namespace web_graph;
		const Nodes& nodes = GetNodes(graph);

		for (auto& nodeData : nodes)
		{
			WebPageNode& node = *nodeData.second;
			bool alreadyMarkedAsDeleted{ common::NodeMarkedAsDeleted(node) };

			if (ShouldBeDeleted(chance))
			{
				if (!alreadyMarkedAsDeleted)
				{
					common::MarkNodeAsDeleted(node);
				}
			}
			else if(alreadyMarkedAsDeleted)
			{
				common::MarkNodeAsNotDeleted(node);
			}
		}
	}
}

}// analyze