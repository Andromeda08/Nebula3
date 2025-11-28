#include "GraphAlgorithm.hpp"

#include <queue>
#include "RenderGraph/Node.hpp"

namespace rg
{
    std::set<int32_t> BFS::execute(Node* pRoot) noexcept
    {
        std::set<int32_t> visited;
        std::queue<Node*> Q;

        Q.push(pRoot);
        visited.insert(pRoot->getId());

        while (!Q.empty())
        {
            const auto current = Q.front();
            Q.pop();

            for (const auto& w : current->getOutgoingEdges())
            {
                if (!visited.contains(w->getId()))
                {
                    visited.insert(w->getId());
                    Q.push(w);
                }
            }
        }

        return visited;
    }

    std::optional<std::vector<Node*>> TopologicalSort::execute(const std::vector<Node*>& nodes) noexcept
    {
        std::map<int32_t, int32_t> inDegrees;
        for (const auto& node : nodes)
        {
            inDegrees.emplace(node->getId(), node->getInDegree());
        }

        std::vector<Node*> T;
        std::queue<Node*>  Q;

        for (const auto& node : nodes)
        {
            if (inDegrees[node->getId()] == 0)
            {
                Q.push(node);
            }
        }

        while (!Q.empty())
        {
            auto v = Q.front();
            Q.pop();
            T.push_back(v);

            for (const auto& w : v->getOutgoingEdges())
            {
                auto wId = w->getId();

                inDegrees[wId]--;
                if (inDegrees[wId] == 0)
                {
                    const auto i = std::ranges::find_if(nodes, [wId](const auto& it){ return it->getId() == wId; });
                    Q.push(*i);
                }
            }
        }

        for (const auto& node : nodes)
        {
            if (inDegrees[node->getId()] != 0)
            {
                return std::nullopt;
            }
        }

        return T;
    }
}
