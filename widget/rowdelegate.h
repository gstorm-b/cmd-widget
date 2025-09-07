#ifndef ROWDELEGATE_H
#define ROWDELEGATE_H

#include <QStyledItemDelegate>
#include "commandrowwidget.h"

namespace rp {

class RowDelegate : public QStyledItemDelegate {
  Q_OBJECT
public:
  explicit RowDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) {}

  QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override {
    Q_UNUSED(option);
    auto* w = new CommandRowWidget(parent);
    return w;
  }

  void setEditorData(QWidget* editor, const QModelIndex& index) const override {
    auto* row = qobject_cast<CommandRowWidget*>(editor);
    auto* m = const_cast<CommandModel*>(static_cast<const CommandModel*>(index.model()));
    row->setContext(index, m);
  }

  void updateEditorGeometry(QWidget* editor,
                            const QStyleOptionViewItem& option,
                            const QModelIndex& index) const override {
    Q_UNUSED(index);
    editor->setGeometry(option.rect);
  }

  QSize sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const {
    // fixed row height 36 px
    return QSize(0, 36);
  }
};

}
#endif // ROWDELEGATE_H
