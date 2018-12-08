#pragma once

#include "WebGraph.h"

namespace graphml
{

void Serialize(const web_graph::WebGraph& graph, const std::string& outFilePath);
std::unique_ptr<web_graph::WebGraph> Deserialize(const std::string& filePath);

}// graphml