#include "CustomLabel.hpp"

CustomLabel::CustomLabel(QWidget* parent) : QLabel(parent)
{

}

void CustomLabel::closeEvent(QCloseEvent* event)
{
  Q_UNUSED(event);
  this->deleteLater();
}
