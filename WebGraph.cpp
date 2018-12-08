#include "WebGraph.h"

namespace web_graph
{

struct WebPageNode
{
	WebPageNode(const Url& url) : url(url){}

	Url url;
	NodeLinks inbound_links;
	NodeLinks outbound_links;
	std::unordered_set<TagId> tags;
};

WebPageNodePtr CreateNode(const Url& url)
{
	return { new WebPageNode{ url }, [](WebPageNode* node) { delete node; } };
}

WebGraph::WebGraph(const Url& rootUrl)
{
	AddNode(*this, rootUrl);
}

// Interface

WebGraph CreateWebGraph() noexcept
{
	return {};
}

WebGraph CreateWebGraph(const Url& rootUrl)
{
	return WebGraph{ rootUrl };
}

Url MakeKey(const Url& url)
{
	Url key{ url };
	static const std::string webPrefix{ "www." };
	if (key.find(webPrefix) == 0)
	{
		key.erase(0, webPrefix.length());
	}

	if (key.back() == '/')
	{
		key.pop_back();
	}

	return key;
}

WebPageNode& AddNode(WebGraph& graph, const Url& url) noexcept
{
	auto newNode = CreateNode(url);
	WebPageNode* node{ newNode.get() };

	if (graph.m_nodes.empty())
	{
		graph.m_root = node;
	}

	graph.m_nodes.emplace(MakeKey(url), std::move(newNode));

	return *node;
}

WebPageNode* GetRoot(const WebGraph& graph) noexcept
{
	return graph.m_root;
}

size_t GetNodesNum(const WebGraph& graph) noexcept
{
	return graph.m_nodes.size();
}

size_t GetLinksNum(const WebGraph& graph) noexcept
{
	return graph.m_linksNum;
}

WebPageNode* GetNode(const WebGraph& graph, const Url& url) noexcept
{
	if (graph.m_root && graph.m_root->url == url)
	{
		return graph.m_root;
	}

	auto it = graph.m_nodes.find(MakeKey(url));
	return (it != graph.m_nodes.end()) ? it->second.get() : nullptr;
}

WebPageNode& AddLink(WebGraph& graph, const Url& url, WebPageNode& from)
{
	WebPageNode* to{ GetNode(graph, url) };
	if (!to)
	{
		to = &AddNode(graph, url);
	}

	return AddLink(graph, *to, from);
}

WebPageNode& AddLink(WebGraph& graph, WebPageNode& to, WebPageNode& from)
{
	++to.inbound_links[&from];
	++from.outbound_links[&to];
	++graph.m_linksNum;

	return to;
}

void DeleteLink(WebPageNode& node, const WebPageNode& nodeToDelete)
{
	if (&node != &nodeToDelete)
	{
		node.inbound_links.erase(&nodeToDelete);
		node.outbound_links.erase(&nodeToDelete);
	}
}

void DeleteNode(WebGraph& graph, const WebPageNode& nodeToDelete)
{
	for (auto& node : graph.m_nodes)
	{
		DeleteLink(*node.second, nodeToDelete);
	}

	if (&nodeToDelete == graph.m_root)
	{
		const NodeLinks& outboundLinks = graph.m_root->outbound_links;
		graph.m_root = !outboundLinks.empty() ?
			const_cast<WebPageNode*>(outboundLinks.begin()->first) : nullptr;
	}

	graph.m_nodes.erase(MakeKey(nodeToDelete.url));
}

const Url& GetNodeUrl(const WebPageNode& node) noexcept
{
	return node.url;
}

const NodeLinks& GetInboundNodeLinks(const WebPageNode& node) noexcept
{
	return node.inbound_links;
}

const NodeLinks& GetOutboundNodeLinks(const WebPageNode& node) noexcept
{
	return node.outbound_links;
}

const Nodes& GetNodes(const WebGraph& graph) noexcept
{
	return graph.m_nodes;
}

void AddTag(WebPageNode& node, TagId tag)
{
	node.tags.emplace(tag);
}

void DeleteTag(WebPageNode& node, TagId tag)
{
	node.tags.erase(tag);
}

bool HasTag(const WebPageNode& node, TagId tag)
{
	return !!node.tags.count(tag);
}

}// namespace web_graph