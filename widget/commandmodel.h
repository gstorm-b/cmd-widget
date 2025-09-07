#ifndef COMMANDMODEL_H
#define COMMANDMODEL_H

#include <QAbstractItemModel>
#include <memory>
#include "commandnode.h"

namespace rp {

class CommandModel : public QAbstractItemModel {
  Q_OBJECT
public:
  enum Column { ColMain = 0, ColCount = 1 }; // single column

  explicit CommandModel(QObject* parent = nullptr);
  ~CommandModel() override;

  QModelIndex index(int row, int column, const QModelIndex& parent) const override;
  QModelIndex parent(const QModelIndex& child) const override;
  int rowCount(const QModelIndex& parent) const override;
  int columnCount(const QModelIndex& parent) const override { Q_UNUSED(parent); return ColCount; }

  QVariant data(const QModelIndex& index, int role) const override;
  Qt::ItemFlags flags(const QModelIndex& index) const override;

  // API for view/controller
  bool insertSiblingAbove(const QModelIndex& ref, CommandPtr cmd);
  bool insertChild(const QModelIndex& parentIndex, CommandPtr cmd, int atRow = -1);
  bool removeCommand(const QModelIndex& index);
  bool moveUp(const QModelIndex& index);
  bool moveDown(const QModelIndex& index);
  bool moveInto(const QModelIndex& srcIdx, const QModelIndex& dstParentIdx,
                int atRow = -1);
  bool moveToRoot(const QModelIndex& srcIdx, int atRow = -1);


  QModelIndex findIndexByCommand(const Command* c) const;
  Command* commandFromIndex(const QModelIndex& idx) const;
  CommandNode* nodeFromIndex(const QModelIndex& idx) const;
  QModelIndex indexFromNode(CommandNode* node, int column = 0) const;

  int globalOrder(const QModelIndex& idx, bool includeStart = false) const;
  int globalOrder(CommandNode* node, bool includeStart = false) const;

  bool isStartNode(const CommandNode* n) const;

private:
  std::unique_ptr<CommandNode> m_root; // invisible root
};
}

#endif // COMMANDMODEL_H
