#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QApplication>
#include <QDebug>
#include <QWidget>
#include <QMainWindow>
#include <QPaintEvent>
#include <QPainter>
#include <QRect>
#include <QScreen>
#include <QSystemTrayIcon>
#include <QApplication>
#include <QAction>
#include <QMenu>
#include <QIcon>
#include <QMessageBox>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>

#include "settingsdata.h"
#include "winkeyboardhook.h"
#include "settingsdialog.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    void paintEvent(QPaintEvent *) Q_DECL_OVERRIDE;

private slots:
    void loadSettings();
    void saveSettings();
    void acceptSettingsFromDialog();
    void handlePressedArrowKey(DWORD pressedKey);
    void handlePressedKey(DWORD pressedKey);
    void toggleWindowVisibility();
    void adjustDot(bool checked);
    void showSettingsDialog();
    void resetDot();
    void showAboutDlg();
    void quitProgram();

private:
    static const QString SETTINGS_FILE_NAME;

    QString settingsFileName;

    QIcon *applicationIcon;
    QSystemTrayIcon *systrayIcon;

    QMenu *systrayMenu;
    QAction *showHideAction;
    QAction *adjustDotAction;
    QAction *resetDotAction;
    QAction *settingsAction;
    QAction *aboutAction;
    QAction *closeAction;

    SettingsData currentSettings;

    SettingsDialog *settingsDialog = nullptr;

    void createActions();
    void createSystrayIcon();

    // Resizes and repositions the window so it matches currentSettings.
    void applySettings();

    // Moves the crosshair back to the middle of the primary screen.
    void centerOnPrimaryScreen();

    // Shifts the crosshair by the given amount of pixels.
    void moveCrosshairBy(int deltaX, int deltaY);

    inline QPoint getCrosshairPosition()
    {
        return QPoint(
            x() + width() / 2,
            y() + height() / 2
        );
    }

    inline void setCrosshairPosition(const QPoint &centerPoint)
    {
        move(
            centerPoint.x() - width() / 2,
            centerPoint.y() - height() / 2
        );
    }

};

#endif // MAINWINDOW_H
