#ifndef COMMANDROWWIDGET_H
#define COMMANDROWWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QEvent>
#include <QMouseEvent>
#include <QStyle>
#include "commandmodel.h"

/**
 * Command row widget
 * [Order] [Title] [Modification buttons]
 * [Title] = [Type name] [Name] [Information]
*/


namespace rp {

class CommandRowWidget : public QWidget {
  Q_OBJECT
public:
  explicit CommandRowWidget(QWidget* parent = nullptr)
      : QWidget(parent) {
    setAutoFillBackground(false);
    auto* h = new QHBoxLayout(this);
    h->setContentsMargins(6, 0, 6, 0);
    h->setSpacing(6);

    /// order label text
    lblOrder = new QLabel("0", this);
    lblOrder->setMinimumWidth(24);
    QFont order_font = lblOrder->font();
    order_font.setBold(true);
    order_font.setItalic(true);
    lblOrder->setFont(order_font);

    /// command title label text
    lblTitle = new QLabel("", this);
    // lblTitle->setTextElideMode(Qt::ElideRight);

    btnUp   = new QPushButton(this);
    btnDown = new QPushButton(this);
    btnDel  = new QPushButton(this);

    btnDel->setIcon(style()->standardIcon(QStyle::SP_TrashIcon));
    btnUp->setIcon(style()->standardIcon(QStyle::SP_ArrowUp));
    btnDown->setIcon(style()->standardIcon(QStyle::SP_ArrowDown));

    btnUp->setText("");
    btnDown->setText("");
    btnDel->setText("");

    btnUp->setToolTip("Move up");
    btnDown->setToolTip("Move down");
    btnDel->setToolTip("Delete");

    h->addWidget(lblOrder, 0, Qt::AlignVCenter);
    h->addWidget(lblTitle, 1);    // stretch largest here
    h->addWidget(btnUp,   0);
    h->addWidget(btnDown, 0);
    h->addWidget(btnDel,  0);
    h->setStretch(1, 1); // ensure title/info label stretches the most

    connect(btnUp,   &QPushButton::clicked, this, [this]{
      emit requestUp(currentIndex);
    });
    connect(btnDown, &QPushButton::clicked, this, [this]{
      emit requestDown(currentIndex);
    });
    connect(btnDel,  &QPushButton::clicked, this, [this]{
      emit requestDelete(currentIndex);
    });

    // Clicking anywhere on the title selects/announces command
    lblTitle->installEventFilter(this);
  }

  void setContext(const QModelIndex& idx, CommandModel* model) {
    currentIndex = idx;
    m_model = model;
    refresh();
  }

  void refresh() {
    if (!currentIndex.isValid() || !m_model) {
      return;
    }

    auto* node = static_cast<rp::CommandNode*>(currentIndex.internalPointer());
    bool isStart = m_model->isStartNode(node);

    // int order = currentIndex.row();
    // get global index
    int order = m_model->globalOrder(node, /*includeStart=*/false);
    if (isStart) {
      lblOrder->setText("0"); // start node order string
    } else {
      lblOrder->setText(QString::number(order+1));
    }

    rp::Command* c = m_model->commandFromIndex(currentIndex);
    // [Type name] [Name] [Information]
    QString title = c ? ( c->typeName() + QString(" [%1]: %2")
                                             .arg(c->commandName())
                                             .arg(c->info()) )
                      : QString("Command error");

    // lblOrder->setText(QString::number(order));
    lblTitle->setText(title);

    rp::CommandNode* parent = node->parent();
    const int r = node->row();
    const int last = parent ? (parent->childCount() - 1) : 0;
    const bool parentIsRoot = (parent && parent->parent() == nullptr);

    bool canDelete = !isStart;
    bool canDown   = !isStart && (r < last);
    bool canUp     = !isStart && (parentIsRoot ? (r > 1) : (r > 0));

    btnUp->setEnabled(canUp);
    btnDown->setEnabled(canDown);
    btnDel->setEnabled(canDelete);
  }

  // // fix row height 36px
  // QSize sizeHint() const override {
  //   return QSize(QWidget::sizeHint().width(), 36);
  // }

signals:
  void requestUp(const QModelIndex& idx);
  void requestDown(const QModelIndex& idx);
  void requestDelete(const QModelIndex& idx);
  void rowClicked(Command* cmd);

protected:
  bool eventFilter(QObject* obj, QEvent* ev) override {
    if (obj == lblTitle && ev->type() == QEvent::MouseButtonRelease) {
      if (!m_model) return false;
      if (Command* c = m_model->commandFromIndex(currentIndex)) emit rowClicked(c);
    }
    return QWidget::eventFilter(obj, ev);
  }

private:
  QLabel* lblCmdName {nullptr};
  QLabel* lblTypeName {nullptr};
  QLabel* lblOrder {nullptr};
  QLabel* lblTitle {nullptr};
  QPushButton* btnUp {nullptr};
  QPushButton* btnDown {nullptr};
  QPushButton* btnDel {nullptr};

  QModelIndex currentIndex;
  CommandModel* m_model {nullptr};
};

}

#endif // COMMANDROWWIDGET_H
