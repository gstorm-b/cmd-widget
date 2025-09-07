#ifndef COMMANDTREEVIEW_H
#define COMMANDTREEVIEW_H

#include <QTreeView>
#include <QMenu>
#include <QMap>
#include <functional>
#include "commandmodel.h"
#include "rowdelegate.h"

namespace rp {

using CommandFactory = std::function<CommandPtr()>;

class CommandTreeView : public QTreeView {
  Q_OBJECT
public:
  explicit CommandTreeView(QWidget* parent = nullptr);


  CommandModel* model() const { return m_model; }

  void registerCommandType(const QString& typeName, CommandFactory factory);
  void addAtRoot(CommandPtr cmd);
  void addChildAtSelection(rp::CommandPtr cmd);

signals:
  void commandClicked(rp::Command* cmd);
  void commandInserted(rp::Command* newCmd);
  void commandWillBeDeleted(rp::Command* victim);
  void commandMoved(rp::Command* cmd);


protected:
  void mousePressEvent(QMouseEvent* e) override;


private slots:
  void onCustomContextMenuRequested(const QPoint& pos);
  void refreshAllRows();

private:
  // void buildDemoData();
  void openEditorsRecursively(const QModelIndex& parent);
  void userClickedOutsideRowItems();

  CommandModel* m_model {nullptr};
  QMenu* m_ctxMenu {nullptr};
  QMap<QString, CommandFactory> m_registry; // typeName -> factory
};

}
#endif // COMMANDTREEVIEW_H
