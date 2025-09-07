#include "commandmodel.h"
#include <QApplication>

namespace rp
{
static std::unique_ptr<CommandNode> makeRoot() {
  auto root = std::make_unique<CommandNode>(CommandPtr{});
  // Ensure Start at root row 0
  auto start = std::make_unique<CommandNode>(std::make_shared<StartCommand>(), root.get());
  root->appendChild(std::move(start));
  return root;
}

CommandModel::CommandModel(QObject* parent)
    : QAbstractItemModel(parent)
    , m_root(makeRoot()) {

}

CommandModel::~CommandModel() = default;

QModelIndex CommandModel::index(int row, int column, const QModelIndex& parentIdx) const {
  if (!hasIndex(row, column, parentIdx)) {
    return {};
  }

  CommandNode* parentNode = parentIdx.isValid()
                                ? static_cast<CommandNode*>(parentIdx.internalPointer())
                                : m_root.get();
  CommandNode* child = parentNode->child(row);
  if (!child) {
    return {};
  }
  return createIndex(row, column, child);
}

QModelIndex CommandModel::parent(const QModelIndex& childIdx) const {
  if (!childIdx.isValid()) {
    return {};
  }
  CommandNode* node = static_cast<CommandNode*>(childIdx.internalPointer());
  CommandNode* parent = node ? node->parent() : nullptr;
  if (!parent || parent == m_root.get()) {
    return {};
  }
  return createIndex(parent->row(), 0, parent);
}

int CommandModel::rowCount(const QModelIndex& parentIdx) const {
  CommandNode* parentNode = parentIdx.isValid()
                                ? static_cast<CommandNode*>(parentIdx.internalPointer())
                                : m_root.get();
  return parentNode ? parentNode->childCount() : 0;
}

QVariant CommandModel::data(const QModelIndex& idx, int role) const {
  if (!idx.isValid()) {
    return {};
  }
  auto* node = static_cast<CommandNode*>(idx.internalPointer());
  const bool isStart = isStartNode(node);

  if (role == Qt::DisplayRole) {
    // Not used by the row widget for painting, but helpful for debugging and accessibility
    if (node->command())
      return node->command()->typeName() + QStringLiteral(" — ") + node->command()->info();
  }
  else if (role == Qt::UserRole) {
    // Expose pointer-sized id if needed by delegates
    return QVariant::fromValue<void*>(node);
  }
  else if (role == Qt::ToolTipRole && isStart) {
    return QStringLiteral("Start node (pinned at index 0)");
  }
  return {};
}

Qt::ItemFlags CommandModel::flags(const QModelIndex& idx) const {
  if (!idx.isValid()) return Qt::NoItemFlags;
  // editable to allow persistent editor
  return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

bool CommandModel::insertSiblingAbove(const QModelIndex& ref, CommandPtr cmd) {
  if (!ref.isValid() || !cmd) {
    return false;
  }
  auto* refNode = static_cast<CommandNode*>(ref.internalPointer());
  auto* parent  = refNode->parent();
  if (!parent) {
    return false;
  }

  const int atRow = refNode->row();
  const bool refIsStart = isStartNode(refNode);
  int insertRow = refIsStart ? atRow + 1 : atRow;

  // beginInsertRows(parent(ref), insertRow, insertRow);
  beginInsertRows(ref.parent(), insertRow, insertRow);
  parent->insertChild(insertRow, std::make_unique<CommandNode>(std::move(cmd)));
  endInsertRows();
  return true;
}

bool CommandModel::insertChild(const QModelIndex& parentIndex, CommandPtr cmd, int atRow) {
  if (!cmd) {
    return false;
  }
  CommandNode* p = parentIndex.isValid()
                       ? static_cast<CommandNode*>(parentIndex.internalPointer())
                       : m_root.get();
  if (!p) {
    return false;
  }

  if (p->command() != nullptr) {
    if (!p->command()->isAllowChild()) {
      return false;
    }
  }

  int row = (atRow < 0) ? p->childCount() : atRow;

  beginInsertRows(parentIndex, row, row);
  p->insertChild(row, std::make_unique<CommandNode>(std::move(cmd)));
  endInsertRows();
  return true;
}

bool CommandModel::removeCommand(const QModelIndex& index) {
  if (!index.isValid()) return false;
  CommandNode* n = static_cast<CommandNode*>(index.internalPointer());
  if (isStartNode(n)) return false; // never remove Start
  CommandNode* p = n->parent();
  if (!p) return false;
  const int r = n->row();

  beginRemoveRows(parent(index), r, r);
  (void)p->takeChild(r);
  endRemoveRows();
  return true;
}

bool CommandModel::moveUp(const QModelIndex& index) {
  if (!index.isValid()) return false;
  auto* n = static_cast<CommandNode*>(index.internalPointer());
  if (isStartNode(n)) return false;
  auto* p = n->parent();
  if (!p) return false;
  const int r = n->row();
  if (r <= 0) return false; // first among siblings

  auto parentIdx = parent(index);
  if (!beginMoveRows(parentIdx, r, r, parentIdx, r - 1)) return false;
  p->moveChild(r, r - 1);
  endMoveRows();
  return true;
}

bool CommandModel::moveDown(const QModelIndex& index) {
  if (!index.isValid()) return false;
  auto* n = static_cast<CommandNode*>(index.internalPointer());
  if (isStartNode(n)) return false;
  auto* p = n->parent();
  if (!p) return false;
  const int r = n->row();
  const int cnt = p->childCount();
  if (r >= cnt - 1) return false; // last among siblings

  auto parentIdx = parent(index);
  if (!beginMoveRows(parentIdx, r, r, parentIdx, r + 2)) return false;
  p->moveChild(r, r + 1);
  endMoveRows();
  return true;
}

bool CommandModel::moveInto(const QModelIndex& srcIdx,
                            const QModelIndex& dstParentIdx, int atRow) {
  if (!srcIdx.isValid() || !dstParentIdx.isValid()) return false;

  auto* srcNode = nodeFromIndex(srcIdx);
  auto* dstParent = nodeFromIndex(dstParentIdx);
  if (!srcNode || !dstParent) return false;

  // Không cho move Start
  if (isStartNode(srcNode)) return false;

  // Không cho move vào chính nó hoặc hậu duệ của nó (tránh vòng)
  for (CommandNode* p = dstParent; p; p = p->parent())
    if (p == srcNode) return false;

  CommandNode* srcParent = srcNode->parent();
  if (!srcParent) return false;

  int srcRow = srcNode->row();
  int dstRow = (atRow < 0) ? dstParent->childCount() : atRow;

  QModelIndex srcParentIdx = parent(srcIdx);
  QModelIndex dstQParentIdx = dstParentIdx;

  // Qt sẽ lo offset khi cùng parent, nhưng với thao tác dữ liệu ta cần chỉnh dstRow nếu cùng parent và kéo xuống
  if (!beginMoveRows(srcParentIdx, srcRow, srcRow, dstQParentIdx, dstRow)) return false;

  // Thực sự di chuyển trong cây
  std::unique_ptr<CommandNode> moved = srcParent->takeChild(srcRow);

  if (srcParent == dstParent && dstRow > srcRow) dstRow -= 1; // sau khi take ra, chỉ số dịch xuống

  dstParent->insertChild(dstRow, std::move(moved));
  endMoveRows();
  return true;
}

bool CommandModel::moveToRoot(const QModelIndex& srcIdx, int atRow) {
  if (!srcIdx.isValid()) return false;
  auto* srcNode = nodeFromIndex(srcIdx);
  if (!srcNode) return false;

  // Không cho move Start
  if (isStartNode(srcNode)) return false;

  CommandNode* srcParent = srcNode->parent();
  if (!srcParent) return false; // đã ở root (parent==nullptr với root-invisible; srcParent==m_root.get() nghĩa là đang ở root)

  int srcRow = srcNode->row();

  // Đưa vào root (QModelIndex() là root)
  QModelIndex rootIdx; // invalid = root
  int dstRow;
  if (atRow < 0) {
    // chèn cuối root
    dstRow = m_root->childCount();
  } else {
    dstRow = atRow;
  }

  // Bảo toàn Start ở row 0: không chèn trước Start
  if (dstRow <= 0) dstRow = 1;

  QModelIndex srcParentIdx = parent(srcIdx);

  // beginMoveRows dùng dstRow theo quy ước Qt (điểm chèn trước vị trí dstRow)
  if (!beginMoveRows(srcParentIdx, srcRow, srcRow, rootIdx, dstRow)) return false;

  std::unique_ptr<CommandNode> moved = srcParent->takeChild(srcRow);

  // Nếu srcParent là root và đang kéo xuống sau vị trí cũ, cần chỉnh dstRow giảm 1.
  if (srcParent == m_root.get() && dstRow > srcRow) dstRow -= 1;

  m_root->insertChild(dstRow, std::move(moved));
  endMoveRows();
  return true;
}

static CommandNode* findNodeByCommand(CommandNode* n, const rp::Command* c) {
  if (!n) return nullptr;
  if (n->command() && n->command().get() == c) return n;
  for (int i = 0; i < n->childCount(); ++i) {
    if (auto* hit = findNodeByCommand(n->child(i), c)) return hit;
  }
  return nullptr;
}

QModelIndex CommandModel::findIndexByCommand(const rp::Command* c) const {
  if (!c) return {};
  // root’s children
  for (int i = 0; i < m_root->childCount(); ++i) {
    if (auto* hit = findNodeByCommand(m_root->child(i), c))
      return indexFromNode(hit);
  }
  return {};
}

Command* CommandModel::commandFromIndex(const QModelIndex& idx) const {
  auto* n = nodeFromIndex(idx);
  if (!n) return nullptr;
  return n->command().get();
}

CommandNode* CommandModel::nodeFromIndex(const QModelIndex& idx) const {
  return idx.isValid() ? static_cast<CommandNode*>(idx.internalPointer()) : nullptr;
}

QModelIndex CommandModel::indexFromNode(CommandNode* node, int column) const {
  if (!node || node == m_root.get()) return {};
  CommandNode* parent = node->parent();
  if (!parent) return {};
  return createIndex(node->row(), column, node);
}

int CommandModel::globalOrder(const QModelIndex& idx, bool includeStart) const {
  auto* n = nodeFromIndex(idx);
  return globalOrder(n, includeStart);
}

// int CommandModel::globalOrder(rp::CommandNode* target, bool includeStart) const {
//   if (!target) return -1;

//   int order = 0;
//   bool found = false;

//   // DFS pre-order, count all nodes from top to bottom
//   std::function<void(CommandNode*)> dfs = [&](CommandNode* n){
//     if (!n || found) return;

//     // ignore root
//     if (n != m_root.get()) {
//       bool isStartN = isStartNode(n);
//       if (isStartN) {
//         if (includeStart) {
//           if (n == target) { found = true; return; }
//           ++order;
//         }
//         // if not includeStart, not count in start node
//       } else {
//         if (n == target) { found = true; return; }
//         ++order;
//       }
//     }
//     for (int i = 0; i < n->childCount() && !found; ++i)
//       dfs(n->child(i));
//   };

//   dfs(m_root.get());

//   if (!found) return -1;

//   // Nếu target chính là node tìm thấy trước khi ++order thì hiện tại order đã là
//   // vị trí tiếp theo. Lúc ta gặp target, ta RETURN trước khi ++, vì vậy cần trừ 1
//   // trong trường hợp includeStart == true và target != m_root.
//   // Để đơn giản hơn, đổi chút logic: tăng order SAU khi kiểm tra target.
//   // => sửa lại khối trên như sau:
//   return order;
// }

int CommandModel::globalOrder(rp::CommandNode* target, bool includeStart) const {
  if (!target) return -1;

  int order = 0;
  int result = -1;

  std::function<void(CommandNode*)> dfs = [&](CommandNode* n){
    if (!n) return;

    if (n != m_root.get()) {
      bool isStartN = isStartNode(n);
      bool countThis = includeStart || !isStartN;
      if (countThis) {
        if (n == target) { result = order; /* không return ngay để dễ đọc */ }
        ++order;
      }
    }
    for (int i = 0; i < n->childCount(); ++i) {
      dfs(n->child(i));
    }
  };

  dfs(m_root.get());
  return result;
}

bool CommandModel::isStartNode(const CommandNode* n) const {
  if (!n) return false;
  const CommandNode* p = n->parent();
  if (!p || p != m_root.get()) return false;
  return (n->row() == 0) && n->command() && (n->command()->type() == Command::Type::Start);
}

}
