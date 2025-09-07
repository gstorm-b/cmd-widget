#ifndef HYPRGCOMMAND_H
#define HYPRGCOMMAND_H

#include <QString>
#include "widget/command.h"

namespace rp {

class HyIfCommand final : public BaseCommand {
public:
  HyIfCommand() : BaseCommand() {

  }

  QString typeName() const override {
    return QStringLiteral("If");
  }

  QString info() const override {
    return QStringLiteral("This is if command");
  }

  Type type() const override {
    return Type::Start;
  }

  const bool isAllowChild() const override {
    return true;
  }
};

class HyMoveLCommand final : public BaseCommand {
public:
  HyMoveLCommand() : BaseCommand() {

  }

  QString typeName() const override {
    return QStringLiteral("MoveL");
  }

  QString info() const override {
    return QStringLiteral("P=(%1,%2,%3) v=%4")
        .arg(x).arg(y).arg(z).arg(speed);
  }

  Type type() const override {
    return Type::MoveL;
  }

  const bool isAllowChild() const override {
    return false;
  }

public:
    double x{0}, y{0}, z{0};
    double speed{100};
};

}


#endif // HYPRGCOMMAND_H
