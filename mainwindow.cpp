#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "hyprgcommand.h"
#include "moveleditor.h"

#include "widget/commandeditor.h"
#include "moveleditor.h"

MainWindow::MainWindow(
    QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
  ui->setupUi(this);

  ui->treeView->registerCommandType("MoveL", []{ return std::make_shared<rp::HyMoveLCommand>(); });
  ui->treeView->registerCommandType("If",    []{ return std::make_shared<rp::HyIfCommand>(); });

  rp::EditorRegistry::registerEditor("MoveL", [](QWidget* p){ return new rp::MoveLEditor(p); });

  connect(ui->treeView, &rp::CommandTreeView::commandClicked,
          this, &MainWindow::CommandClicked);

  // connect(ui->treeView, &rp::CommandTreeView::commandClicked,
  //         ui->stackedWidget, [this, ui->stackedWidget, model=ui->treeView->model()](rp::Command* c){
  //           panel->editCommand(model, c);
  //         });

  // ui->treeView->setStyleSheet(R"(
  //   QTreeView::branch:has-children:!has-siblings:closed,
  //   QTreeView::branch:closed:has-children {
  //       border-image: none;
  //       image: url(:/icons/plus.png);
  //   }
  //   QTreeView::branch:open:has-children:!has-siblings,
  //   QTreeView::branch:open:has-children {
  //       border-image: none;
  //       image: url(:/icons/minus.png);
  //   }
  //   )");
}

MainWindow::~MainWindow()
{
  delete ui;
}


void MainWindow::CommandClicked(rp::Command* cmd) {
  ui->stackedWidget->editCommand(ui->treeView->model(), cmd);
}

