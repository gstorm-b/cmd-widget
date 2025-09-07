#ifndef MOVELEDITOR_H
#define MOVELEDITOR_H

#include "hyprgcommand.h"
#include "widget/commandeditor.h"
#include <QDoubleSpinBox>
#include <QFormLayout>

namespace rp {
class MoveLEditor : public CommandEditorWidget {
  Q_OBJECT
public:
  explicit MoveLEditor(QWidget* parent=nullptr) : CommandEditorWidget(parent) {
    auto* lay = new QFormLayout(this);
    sx = new QDoubleSpinBox(this);
    sy = new QDoubleSpinBox(this);
    sz = new QDoubleSpinBox(this);
    sv = new QDoubleSpinBox(this);
    sx->setRange(-1e6, 1e6); sy->setRange(-1e6, 1e6); sz->setRange(-1e6, 1e6);
    sv->setRange(0, 1e6);
    lay->addRow("X", sx); lay->addRow("Y", sy); lay->addRow("Z", sz); lay->addRow("Speed", sv);

    connect(sx, &QDoubleSpinBox::editingFinished, this, &MoveLEditor::apply);
    connect(sy, &QDoubleSpinBox::editingFinished, this, &MoveLEditor::apply);
    connect(sz, &QDoubleSpinBox::editingFinished, this, &MoveLEditor::apply);
    connect(sv, &QDoubleSpinBox::editingFinished, this, &MoveLEditor::apply);
  }

  void setContext(CommandModel* model, Command* cmd) override {
    m = model; c = dynamic_cast<HyMoveLCommand*>(cmd);
    if (!c) { setEnabled(false); return; }
    setEnabled(true);
    sx->setValue(c->x); sy->setValue(c->y); sz->setValue(c->z); sv->setValue(c->speed);
  }

private slots:
  void apply() {
    if (!m || !c) return;
    // (khuyến nghị) dùng QUndoStack; ở đây minh họa direct apply
    c->x = sx->value(); c->y = sy->value(); c->z = sz->value(); c->speed = sv->value();

    // Báo model để các row refresh
    if (auto idx = m->findIndexByCommand(c); idx.isValid())
      emit m->dataChanged(idx, idx, {Qt::DisplayRole}); // hoặc dùng refreshAllRows ở view

    emit parametersChanged(c);
  }

private:
  CommandModel* m{nullptr};
  HyMoveLCommand* c{nullptr};
  QDoubleSpinBox *sx{nullptr}, *sy{nullptr}, *sz{nullptr}, *sv{nullptr};
};

} // namespace rp

#endif // MOVELEDITOR_H
