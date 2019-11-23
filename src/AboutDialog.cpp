#include "AboutDialog.hpp"
#include "ui_AboutDialog.h"

AboutDialog::AboutDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::AboutDialog)
{
  ui->setupUi(this);
  this->setFixedSize(this->size());
}

AboutDialog::~AboutDialog()
{
  delete ui;
}
