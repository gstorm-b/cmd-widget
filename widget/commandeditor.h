#ifndef COMMANDEDITORH_H
#define COMMANDEDITORH_H

#include <QWidget>
#include <functional>
#include <memory>
#include "command.h"
#include "commandmodel.h"

namespace rp {

class CommandEditorWidget : public QWidget {
  Q_OBJECT
public:
  explicit CommandEditorWidget(QWidget* parent=nullptr) : QWidget(parent) {}
  virtual ~CommandEditorWidget() = default;

  // Cài đặt ngữ cảnh: model + lệnh cần edit
  virtual void setContext(CommandModel* model, Command* cmd) = 0;

signals:
  void parametersChanged(Command* cmd);  // phát khi user sửa xong (apply)
};

// Factory kiểu: tạo editor cho type cụ thể
using EditorFactory = std::function<CommandEditorWidget*(QWidget* parent)>;

class EditorRegistry {
public:
  static void registerEditor(const QString& typeName, EditorFactory f);
  static CommandEditorWidget* create(const QString& typeName, QWidget* parent=nullptr);
};

} // namespace rp
#endif // COMMANDEDITORH_H
