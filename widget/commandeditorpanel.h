#ifndef COMMANDEDITORPANEL_H
#define COMMANDEDITORPANEL_H

#include <QStackedWidget>
#include <QMap>
#include "commandeditor.h"

namespace rp {
class CommandEditorPanel : public QStackedWidget {
  Q_OBJECT
public:
  explicit CommandEditorPanel(QWidget* parent=nullptr) : QStackedWidget(parent) {}

public slots:
  void editCommand(CommandModel* model, Command* cmd) {
    if (!cmd) { setCurrentWidget(blank()); return; }

    const QString t = cmd->typeName();
    CommandEditorWidget* ed = editors.value(t, nullptr);
    if (!ed) {
      ed = EditorRegistry::create(t, this);
      if (!ed) { setCurrentWidget(blank()); return; }
      editors.insert(t, ed);
      addWidget(ed);
      connect(ed, &CommandEditorWidget::parametersChanged, this, &CommandEditorPanel::parametersChanged);
    }
    ed->setContext(model, cmd);
    setCurrentWidget(ed);
  }

signals:
  void parametersChanged(Command* cmd);

private:
  QWidget* blank() {
    if (!empty) { empty = new QWidget(this); addWidget(empty); }
    return empty;
  }
  QWidget* empty{nullptr};
  QMap<QString, CommandEditorWidget*> editors;
};
} // namespace rp
#endif // COMMANDEDITORPANEL_H
