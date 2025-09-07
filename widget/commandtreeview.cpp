#include "commandtreeview.h"
#include "commandrowwidget.h"
#include "hyprgcommand.h"
#include <QHeaderView>
#include <QMouseEvent>
#include <QAction>

namespace rp {

struct NodeEntry {
  int order;
  QModelIndex idx;
  QString label;
};

void enumerateDfs(rp::CommandModel* m, const QModelIndex& parent,
                  QVector<NodeEntry>& out, int depth, bool includeStart) {
  int rows = m->rowCount(parent);
  for (int r = 0; r < rows; ++r) {
    QModelIndex idx = m->index(r, 0, parent);
    auto* node = m->nodeFromIndex(idx);
    bool isStart = m->isStartNode(node);

    bool countThis = includeStart || !isStart;
    if (countThis) {
      rp::Command* c = m->commandFromIndex(idx);
      if (!c->isAllowChild()) {
        continue;
      }

      int cmdGlobalIndex = m->globalOrder(node, includeStart) + 1;
      QString indent(depth*2, QLatin1Char(' '));
      out.push_back(NodeEntry{ cmdGlobalIndex, idx, QStringLiteral("%1%2: %3 [%4]")
                                                .arg(indent)
                                                .arg(cmdGlobalIndex)
                                                .arg(c->typeName())
                                                .arg(c->commandName()) });
    }
    enumerateDfs(m, idx, out, depth + 1, includeStart);
  }
}

static bool isDescendant(rp::CommandNode* candidate, rp::CommandNode* potentialAncestor)
{
  if (!candidate || !potentialAncestor) return false;
  for (rp::CommandNode* p = candidate->parent(); p; p = p->parent())
    if (p == potentialAncestor) return true;
  return false;
}

CommandTreeView::CommandTreeView(QWidget* parent)
    : QTreeView(parent) {
    m_model = new CommandModel(this);
    setModel(m_model);

    // Tree appearance
    setHeaderHidden(true);            // no horizontal header
    setRootIsDecorated(true);         // show expanders for children
    setUniformRowHeights(true);

    // Single column with persistent QWidget editor per row
    auto* del = new RowDelegate(this);
    setItemDelegate(del);

    auto refreshAll = [this]{
      // gọi sau một vòng event để Qt ổn định lại geometry
      QMetaObject::invokeMethod(this, [this]{ refreshAllRows(); }, Qt::QueuedConnection);
    };

    connect(m_model, &QAbstractItemModel::rowsInserted, this,
            [=](auto,auto,auto){ refreshAll(); });
    connect(m_model, &QAbstractItemModel::rowsRemoved, this,
            [=](auto,auto,auto){ refreshAll(); });
    connect(m_model, &QAbstractItemModel::rowsMoved, this,
            [=](auto,auto,auto,auto,auto){ refreshAll(); });
    connect(m_model, &QAbstractItemModel::modelReset, this,
            [=]{ refreshAll(); });
    connect(m_model, &QAbstractItemModel::dataChanged, this,
            [=](auto,auto,auto){ refreshAll(); });

    // Open persistent editors for all existing and future rows
    openEditorsRecursively(QModelIndex());
    connect(m_model, &QAbstractItemModel::rowsInserted, this,
            [this](const QModelIndex& parent, int first, int last){
        for (int r = first; r <= last; ++r) {
            QModelIndex idx = m_model->index(r, 0, parent);
            openPersistentEditor(idx);
            if (auto* w = qobject_cast<CommandRowWidget*>(indexWidget(idx))) {
                connect(w, &CommandRowWidget::requestUp, this,
                      [this](const QModelIndex& i) {
                        if (m_model->moveUp(i)) {
                          if (Command* c = m_model->commandFromIndex(i)) {
                            emit commandMoved(c);
                          }
                        }
                });

                connect(w, &CommandRowWidget::requestDown, this,
                        [this](const QModelIndex& i) {
                          if (m_model->moveDown(i)) {
                            if (Command* c = m_model->commandFromIndex(i)) {
                              emit commandMoved(c);
                            }
                          }
                });

                connect(w, &CommandRowWidget::requestDelete, this,
                        [this](const QModelIndex& i) {
                          if (Command* c = m_model->commandFromIndex(i)) {
                            emit commandWillBeDeleted(c); m_model->removeCommand(i);
                          }
                });

                connect(w, &CommandRowWidget::rowClicked, this,
                        [this](Command* c) {
                          emit commandClicked(c);
                });
            }
        }
    });

    // Context menu
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested,
            this, &CommandTreeView::onCustomContextMenuRequested);

    // Register default command creators
    // registerCommandType("MoveL", []{ return std::make_shared<HyMoveLCommand>(); });
    // registerCommandType("If",    []{ return std::make_shared<HyIfCommand>(); });

    // Demo rows under Start
    // buildDemoData();
}

void CommandTreeView::registerCommandType(const QString& typeName, CommandFactory factory) {
    m_registry[typeName] = std::move(factory);
}

void CommandTreeView::addAtRoot(rp::CommandPtr cmd) {
  if (cmd) {
    m_model->insertChild(QModelIndex(), cmd);
  }
}

// add child at selecting node, if not select any node, command will add at root
void CommandTreeView::addChildAtSelection(rp::CommandPtr cmd) {
  if (!cmd) {
    return;
  }
  QModelIndex sel = currentIndex();
  if (sel.isValid()) {
    if (m_model->insertChild(sel, cmd)) {
      expand(sel);
    }
  } else {
    m_model->insertChild(QModelIndex(), cmd);
  }
}

void CommandTreeView::mousePressEvent(QMouseEvent* e) {
    QTreeView::mousePressEvent(e); // default selection handling
}

void CommandTreeView::onCustomContextMenuRequested(const QPoint& pos) {
    QModelIndex idx = indexAt(pos);
    if (!m_ctxMenu) m_ctxMenu = new QMenu(this);
    m_ctxMenu->clear();

    // user clicked out of row items
    if (!idx.isValid()) {
      userClickedOutsideRowItems();
      m_ctxMenu->popup(viewport()->mapToGlobal(pos));
      return;
    }

    if (m_model->isStartNode(m_model->nodeFromIndex(idx))) {
      return;
    }

    bool isAllowChild = m_model->commandFromIndex(idx)->isAllowChild();
    QMenu* sib = new QMenu(tr("Insert command above"), m_ctxMenu);
    QMenu* chd = nullptr;
    if (isAllowChild) {
      chd = new QMenu("Add child command", m_ctxMenu);
    }

    for (auto it = m_registry.cbegin(); it != m_registry.cend(); ++it) {
        const QString t = it.key();
        sib->addAction(t, [this, idx, t]{
            auto f = m_registry.value(t);
            if (!f) return;
            auto cmd = f();
            if (!cmd) return;
            if (m_model->insertSiblingAbove(idx, cmd)) emit commandInserted(cmd.get());
        });

        if (isAllowChild) {
          chd->addAction(t, [this, idx, t]{
            auto f = m_registry.value(t);
            if (!f) return;
            auto cmd = f();
            if (!cmd) return;
            QModelIndex pidx = idx.isValid() ? idx : QModelIndex();
            if (m_model->insertChild(pidx, cmd)) {
              if (pidx.isValid()) expand(pidx);
              emit commandInserted(cmd.get());
            }
          });
        }
    }

    m_ctxMenu->addMenu(sib);
    if (isAllowChild) {
      m_ctxMenu->addMenu(chd);
    }

    // if (!m_model->isStartNode(m_model->nodeFromIndex(idx))) {
      // QAction* moveInsideAct = m_ctxMenu->addAction(tr("Move inside"), [this, idx] {
      //   // list all node with global order
      //   QVector<NodeEntry> entries;
      //   // int counter = 0;
      //   bool includeStart = false;  // not allow move to start node
      //   enumerateDfs(m_model, QModelIndex(), entries, /*depth*/0, includeStart);

      //   if (entries.isEmpty()) return;

      //   // build show menu, use QInputDialog::getInt
      //   QMenu pick(this);
      //   QList<QAction*> acts;
      //   for (int e_idx=0;e_idx<entries.size();e_idx++) {
      //     QString text = entries[e_idx].label;
      //     QAction* a = pick.addAction(text);
      //     acts.push_back(a);
      //   }

      //   // for (const auto& e : entries) {
      //   //   // Không cho chọn chính nó hoặc hậu duệ của nó
      //   //   // (lọc thô: cho phép chọn, model sẽ chặn lần nữa)
      //   // }

      //   QAction* chosen = pick.exec(QCursor::pos());
      //   if (!chosen) return;

      //   int chosenIdx = acts.indexOf(chosen);
      //   if (chosenIdx < 0) return;

      //   QModelIndex dstParentIdx = entries[chosenIdx].idx;

      //   // move into selected node
      //   if (m_model->moveInto(idx, dstParentIdx)) {
      //     expand(dstParentIdx);
      //     refreshAllRows();
      //     if (rp::Command* c = m_model->commandFromIndex(idx)) {
      //       emit commandMoved(c);
      //     }
      //   }
      // });
      // Q_UNUSED(moveInsideAct);
    // }

    QMenu* moveInsideMenu = m_ctxMenu->addMenu("Move inside…");

    // 2.1) Thêm lựa chọn Move out (to root)
    QAction* moveOutAct = moveInsideMenu->addAction("Move out (to root)", [this, idx]{
      // chèn về cuối root (sau Start)
      if (m_model->moveToRoot(idx, /*atRow=*/-1)) {
        // mở lại view nếu cần và refresh thứ tự
        // (nếu bạn có hàm refreshAllRows như đã đề cập)
        // refreshAllRows();
        if (rp::Command* c = m_model->commandFromIndex(idx)) emit commandMoved(c);
      }
    });
    Q_UNUSED(moveOutAct);

    moveInsideMenu->addSeparator();

    QVector<NodeEntry> entries;
    bool includeStart = false;
    enumerateDfs(m_model, QModelIndex(), entries, /*depth*/0, includeStart);

    auto* srcNode = m_model->nodeFromIndex(idx);

    // Dựng menu lựa chọn
    for (const auto& e : entries) {
      auto* dstNode = m_model->nodeFromIndex(e.idx);
      // Ẩn chính nó
      if (dstNode == srcNode) continue;
      // Ẩn hậu duệ của nó
      if (isDescendant(dstNode, srcNode)) continue;

      QAction* a = moveInsideMenu->addAction(e.label, [this, idx, e]{
        if (m_model->moveInto(idx, e.idx)) {
          expand(e.idx);
          // refreshAllRows();
          if (rp::Command* c = m_model->commandFromIndex(idx)) emit commandMoved(c);
        }
      });

      // (tùy chọn) có thể disable mục là parent hiện tại (no-op)
      auto* srcParent = m_model->nodeFromIndex(m_model->parent(idx));
      if (dstNode == srcParent) a->setEnabled(false);
    }

    QAction* delAct = m_ctxMenu->addAction("Delete", [this, idx]{
      if (Command* c = m_model->commandFromIndex(idx)) {
        emit commandWillBeDeleted(c);
      }
      m_model->removeCommand(idx);
    });
    Q_UNUSED(delAct);

    m_ctxMenu->popup(viewport()->mapToGlobal(pos));
}

// void CommandTreeView::buildDemoData()
// {
//     QModelIndex startIdx = m_model->index(0, 0, QModelIndex());

//     auto m1 = std::make_shared<HyMoveLCommand>(); m1->x = 100; m1->y = 0; m1->z = 200; m1->speed = 150;
//     auto m2 = std::make_shared<HyMoveLCommand>(); m2->x = 200; m2->y = 50; m2->z = 150; m2->speed = 120;
//     auto ifc = std::make_shared<HyIfCommand>();
//     // ifc->cond = QStringLiteral("hasPart");

//     m_model->insertChild(startIdx, m1);
//     m_model->insertChild(startIdx, ifc);

//     QModelIndex ifIdx = m_model->index(1, 0, startIdx);
//     m_model->insertChild(ifIdx, m2);

//     expandAll();
// }

void CommandTreeView::openEditorsRecursively(const QModelIndex& parent) {
    int rows = m_model->rowCount(parent);
    for (int r = 0; r < rows; ++r) {
        QModelIndex idx = m_model->index(r, 0, parent);
        openPersistentEditor(idx);
        if (auto* w = qobject_cast<CommandRowWidget*>(indexWidget(idx))) {
          connect(w, &CommandRowWidget::requestUp, this,
                  [this](const QModelIndex& i) {
                    if (m_model->moveUp(i)) {
                      if (Command* c = m_model->commandFromIndex(i)) {
                        emit commandMoved(c);
                      }
                    }
          });

          connect(w, &CommandRowWidget::requestDown, this,
                  [this](const QModelIndex& i) {
                    if (m_model->moveDown(i)) {
                      if (Command* c = m_model->commandFromIndex(i)) {
                        emit commandMoved(c);
                      }
                    }
          });

          connect(w, &CommandRowWidget::requestDelete, this,
                  [this](const QModelIndex& i) {
                    if (Command* c = m_model->commandFromIndex(i)) {
                      emit commandWillBeDeleted(c);
                    }
                    m_model->removeCommand(i);
          });

          connect(w, &CommandRowWidget::rowClicked, this,
                  [this](Command* c) {
                    emit commandClicked(c);
          });
        }
        openEditorsRecursively(idx);
    }
}

void CommandTreeView::userClickedOutsideRowItems() {
  QMenu* sib_root = new QMenu(tr("Insert new command"), m_ctxMenu);

  for (auto it = m_registry.cbegin(); it != m_registry.cend(); ++it) {
    const QString t = it.key();
    sib_root->addAction(t, [this, t] {
      auto f = m_registry.value(t);
      if (!f) return;
      auto cmd = f();
      if (!cmd) return;
      if (m_model->insertChild(QModelIndex(), cmd)) emit commandInserted(cmd.get());
    });
  }

  m_ctxMenu->addMenu(sib_root);
}

void CommandTreeView::refreshAllRows()
{
  std::function<void(const QModelIndex&)> rec = [&](const QModelIndex& parent){
    int rows = m_model->rowCount(parent);
    for (int r = 0; r < rows; ++r) {
      QModelIndex idx = m_model->index(r, 0, parent);
      if (auto* w = qobject_cast<CommandRowWidget*>(indexWidget(idx))) {
        w->refresh();
      }
      rec(idx);
    }
  };
  rec(QModelIndex());
}

}
