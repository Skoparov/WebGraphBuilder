#pragma once

#include "WebGraph.h"

namespace common
{

enum class Tag : web_graph::TagId { MarkedAsDeleted };

inline bool NodeMarkedAsDeleted(const web_graph::WebPageNode& node)
{
	return web_graph::HasTag(node, static_cast<web_graph::TagId>(Tag::MarkedAsDeleted));
}

inline void MarkNodeAsDeleted(web_graph::WebPageNode& node)
{
	web_graph::AddTag(node, static_cast<web_graph::TagId>(Tag::MarkedAsDeleted));
}

inline void MarkNodeAsNotDeleted(web_graph::WebPageNode& node)
{
	web_graph::DeleteTag(node, static_cast<web_graph::TagId>(Tag::MarkedAsDeleted));
}

}
