#pragma once

#include <string>
#include <memory>
#include <unordered_set>
#include <unordered_map>

namespace web_graph
{

struct WebPageNode;

using Url = std::string;
using WebPageNodePtr = std::unique_ptr<WebPageNode, void(*)(WebPageNode*)>;
using NodeLinkNum = size_t;
using NodeLinks = std::unordered_map<const WebPageNode*, NodeLinkNum>;
using Nodes = std::unordered_map<Url, WebPageNodePtr>;
using TagId = uint32_t;

struct WebGraph
{
	friend WebGraph CreateWebGraph() noexcept;
	friend WebGraph CreateWebGraph(const Url&);
	friend WebPageNode& AddNode(WebGraph&, const Url&) noexcept;
	friend WebPageNode* GetRoot(const WebGraph&) noexcept;
	friend WebPageNode* GetNode(const WebGraph&, const Url&) noexcept;
	friend size_t GetNodesNum(const WebGraph&) noexcept;
	friend size_t GetLinksNum(const WebGraph&) noexcept;
	friend WebPageNode& AddLink(WebGraph&, const Url&, WebPageNode&);
	friend WebPageNode& AddLink(WebGraph&, WebPageNode& to, WebPageNode& from);
	friend const Nodes& GetNodes(const WebGraph&) noexcept;
	friend void DeleteNode(WebGraph&, const WebPageNode&);

public:
	WebGraph(WebGraph&&) = default;
	WebGraph& operator=(WebGraph&&) = default;

private:
	WebGraph() = default;
	explicit WebGraph(const Url& rootUrl);

private:
	WebPageNode* m_root{ nullptr };
	Nodes m_nodes;
	size_t m_linksNum{ 0 };
};

WebGraph CreateWebGraph() noexcept;
WebGraph CreateWebGraph(const Url& rootUrl);
WebPageNode& AddNode(WebGraph&, const Url&) noexcept;
WebPageNode* GetRoot(const WebGraph&) noexcept;
size_t GetNodesNum(const WebGraph&) noexcept;
size_t GetLinksNum(const WebGraph&) noexcept;
WebPageNode* GetNode(const WebGraph&, const Url&) noexcept;
WebPageNode& AddLink(WebGraph&, const Url&, WebPageNode& from);
WebPageNode& AddLink(WebGraph&, WebPageNode& to, WebPageNode& from);
const Url& GetNodeUrl(const WebPageNode&) noexcept;
const NodeLinks& GetInboundNodeLinks(const WebPageNode&) noexcept;
const NodeLinks& GetOutboundNodeLinks(const WebPageNode&) noexcept;
const Nodes& GetNodes(const WebGraph&) noexcept;
void DeleteNode(WebGraph&, const WebPageNode&);
void AddTag(WebPageNode& node, TagId);
void DeleteTag(WebPageNode&, TagId);
bool HasTag(const WebPageNode&, TagId);

}//namepsace web_graph