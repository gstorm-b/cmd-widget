#ifndef COMMAND_H
#define COMMAND_H

#include <QString>
#include <memory>

// namespace rp == robot program
namespace rp {

class Command {
public:
  enum class Type { Base, Start, If, MoveL, Custom };
  virtual ~Command() = default;

  virtual QString typeName() const = 0;
  virtual QString commandName() = 0;
  virtual void setCommandName(QString name) = 0;
  virtual QString info() const { return {}; }
  virtual Type type() const = 0;
  virtual const bool isAllowChild() const = 0;
};

class BaseCommand : public Command {
public:
  BaseCommand() {
    m_commandName = "cmd_" + QString::number(auto_cmd_increase_index++, 10);
  }

  virtual QString typeName() const override {
    return QStringLiteral("Base (not use)");
  }

  QString commandName() override {
    return m_commandName;
  }

  void setCommandName(QString name) override {
    m_commandName = name;
  }

  virtual QString info() const override {
    return QStringLiteral("Base command (do nothing)");
  }

  virtual Type type() const override {
    return Type::Base;
  }

  virtual const bool isAllowChild() const override {
    return false;
  }

protected:
  QString m_commandName;

private:
  static int auto_cmd_increase_index;
};

// ---------------- Example commands ----------------
class StartCommand final : public BaseCommand {
public:
  StartCommand() {
    m_commandName = "Start_point";
  }

  QString typeName() const override {
    return QStringLiteral("Start");
  }

  QString commandName() override {
    return QStringLiteral("Start point");
  }

  QString info() const override {
    return QStringLiteral("Program entry");
  }

  Type type() const override {
    return Type::Start;
  }

  const bool isAllowChild() const override {
    return false;
  }
};


// class MoveLCommand final : public Command {

// public:
//   double x{0}, y{0}, z{0};
//   double speed{100};

//   QString typeName() const override { return QStringLiteral("MoveL"); }
//   QString info() const override {
//     return QStringLiteral("P=(%1,%2,%3) v=%4").arg(x).arg(y).arg(z).arg(speed);
//   }
//   Type type() const override { return Type::MoveL; }
//   const bool isAllowChild() const override { return false };
// };

// class IfCommand final : public Command
// {
// public:
//   QString cond = QStringLiteral("flag == true");

//   QString typeName() const override { return QStringLiteral("If"); }
//   QString info() const override { return QStringLiteral("cond: ") + cond; }
//   Type type() const override { return Type::If; }
//   const bool isAllowChild() const override { return true };
// };

using CommandPtr = std::shared_ptr<Command>;

} // namespace rp

#endif // COMMAND_H
