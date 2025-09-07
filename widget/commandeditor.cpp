#include "commandeditor.h"

#include "commandeditor.h"
#include <QHash>

namespace rp {
static QHash<QString, EditorFactory>& REG() { static QHash<QString, EditorFactory> r; return r; }

void EditorRegistry::registerEditor(const QString& t, EditorFactory f) { REG().insert(t, std::move(f)); }
CommandEditorWidget* EditorRegistry::create(const QString& t, QWidget* parent) {
  auto it = REG().find(t);
  return it==REG().end() ? nullptr : it.value()(parent);
}

} // namespace rp
