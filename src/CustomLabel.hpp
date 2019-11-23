#ifndef CUSTOMLABEL_HPP
#define CUSTOMLABEL_HPP

#include <QLabel>

class CustomLabel : public QLabel
{
  public:
    CustomLabel(QWidget* parent = nullptr);

  private:
    void closeEvent(QCloseEvent* event) override;
};

#endif // CUSTOMLABEL_HPP
