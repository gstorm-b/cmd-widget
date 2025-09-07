#ifndef COMMANDNODE_H
#define COMMANDNODE_H

#include "command.h"
#include <vector>
#include <memory>

namespace rp {

class CommandNode {
public:
  explicit CommandNode(CommandPtr cmd, CommandNode* parent = nullptr)
      : m_parent(parent), m_cmd(std::move(cmd)) {

  }

  CommandNode* parent() const {
    return m_parent;
  }

  int childCount() const {
    return static_cast<int>(m_children.size());
  }

  CommandNode* child(int row) const {
    return (row >= 0 && row < childCount()) ? m_children[row].get() : nullptr;
  }

  int row() const {
    if (!m_parent) {
      return 0;
    }

    for (int i = 0; i < m_parent->childCount(); ++i) {
      if (m_parent->m_children[i].get() == this) return i;
    }

    return 0;
  }

  void insertChild(int row, std::unique_ptr<CommandNode> node) {
    if (!node) {
      return;
    }

    if (row < 0 || row > childCount()) {
      row = childCount();
    }
    node->m_parent = this;
    m_children.insert(m_children.begin() + row, std::move(node));
  }

  void appendChild(std::unique_ptr<CommandNode> node) {
    if (!node) {
      return;
    }
    node->m_parent = this;
    m_children.emplace_back(std::move(node));
  }

  std::unique_ptr<CommandNode> takeChild(int row) {
    if (row < 0 || row >= childCount()) {
      return nullptr;
    }
    std::unique_ptr<CommandNode> n = std::move(m_children[row]);
    m_children.erase(m_children.begin() + row);
    n->m_parent = nullptr;
    return n;
  }

  bool moveChild(int from, int to) {
    if (from == to) {
      return true;
    }
    if (from < 0 || from >= childCount()) return false;
    if (to   < 0 || to   >= childCount()) return false;
    auto node = std::move(m_children[from]);
    m_children.erase(m_children.begin() + from);
    m_children.insert(m_children.begin() + to, std::move(node));
    return true;
  }

  CommandPtr& command() {
    return m_cmd;
  }

  const CommandPtr& command() const {
    return m_cmd;
  }

private:
  CommandNode* m_parent;
  std::vector<std::unique_ptr<CommandNode>> m_children;
  CommandPtr m_cmd; // nullptr allowed on the invisible root
};
}

#endif // COMMANDNODE_H
