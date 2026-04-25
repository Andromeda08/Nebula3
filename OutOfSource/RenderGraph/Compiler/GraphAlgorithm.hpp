#pragma once

#include <optional>
#include <set>
#include <vector>

namespace rg
{
    class Node;

    struct BFS
    {
        /**
         * Run BFS from the specified root node.
         * @param pRoot Root node
         * @return Set of nodes (IDs) visited.
         */
         [[nodiscard]] static std::set<int32_t> execute(Node* pRoot) noexcept;
    };

    struct TopologicalSort
    {
        /**
         * Topologically sort the input nodes.
         * @param nodes Input nodes
         * @return Topological ordering of nodes with non-owning references.
         */
        [[nodiscard]] static std::optional<std::vector<Node*>> execute(const std::vector<Node*>& nodes) noexcept;
    };
}
