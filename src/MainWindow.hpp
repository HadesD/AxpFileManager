#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>
#include <QDirModel>
#include <QStandardItemModel>
#include <functional>

namespace Ui {
  class MainWindow;
}

class AxpArchivePort;

class MainWindow : public QMainWindow
{
    Q_OBJECT

  private:
    static MainWindow* s_instance;

  public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

  public:
    static MainWindow* getInstance();
    Ui::MainWindow* getUi() const;
    QString getSelectedDirAxpKey() const;
    void setCurrentDir(const QModelIndex& index);
    void setProgress(
        const QString& name,
        const std::size_t current, const std::size_t total
        );
    AxpArchivePort* getAxpArchive() {return m_axpArchive;}

  signals:
    void invoke(std::function<void()> cb);

  private slots:
    void openAxpArchive(const QString& fileName);
    void onAxpReadListProgress(
        const QString& fileName,
        const std::size_t current, const std::size_t total
        );
    void invokeCallback(std::function<void()> cb);

  private slots:
    void on_actionOpen_triggered();

    void on_actionAbout_triggered();

    void on_actionNew_triggered();

    void on_actionExtract_All_Data_triggered();

    void on_actionExit_triggered();

    void on_workingPathLabel_linkActivated(const QString &link);

    void on_actionAdd_File_triggered();

    void on_actionSave_As_triggered();

    void on_actionAdd_Folder_triggered();

    void on_actionNew_From_directory_triggered();

  private:
    Ui::MainWindow *ui = nullptr;
    AxpArchivePort* m_axpArchive = nullptr;
    QStandardItemModel* m_dirModel = nullptr;
    QStandardItemModel* m_fileModel = nullptr;
};

#endif // MAINWINDOW_HPP
