#pragma once

#include "WebGraph.h"

namespace analyze
{

// Num of nodes with at least 1 inbound and outbound link / total num of nodes
// Number of nodes included into information interaction
double CalcEdgesIndex(const web_graph::WebGraph& graph);

// Net density :
// num of edges / (node of nodes * (num of nodes - 1))) or 0 if nodes num <= 1
double CalcLinksIndex(const web_graph::WebGraph& graph);

// The degree of coherense of the graph
// N = set of nodes with in + out links num >= 2
// Proximity subgraph of node = subgraph made of the node and it's adjacent nodes
// Sum = sum of local link indexes of N
// Clustering coeff = Sum / sizeof(N)
double CalcClusteringCoeff(const web_graph::WebGraph& graph);

void GetNodesTypesNum(
	const web_graph::WebGraph& graph,
	size_t& inductorsNum,
	size_t& collectorsNum,
	size_t& mediatorsNum);

struct GraphAnalysisResult
{
	double edgesIndex;
	double linksIndex;
	double clusteringCoeff;
	size_t inductorNum;
	size_t collectorsNum;
	size_t mediatorsNum;
};

GraphAnalysisResult Analyze(const web_graph::WebGraph& graph);

// Mark nodes as deleted with the specified chance(should be within [0, 1])
void SimulateNodesDeletion(const web_graph::WebGraph& graph, double chance);

}// analyze