#include "mainwindow.h"

#include "crosshairrenderer.h"

const QString MainWindow::SETTINGS_FILE_NAME = "centerdot.ini";

namespace {

// Reads an integer setting and falls back to the default whenever the stored
// value is not a number or outside of the range the user interface allows.
int readBoundedInt(QSettings &settings, const QString &key, int defaultValue, int minimum, int maximum)
{
    bool isInt = false;
    const int value = settings.value(key, defaultValue).toInt(&isInt);

    if (!isInt || value < minimum || value > maximum) {
        return defaultValue;
    }

    return value;
}

QColor readColor(QSettings &settings, const QString &key, const QColor &defaultValue)
{
    const QString colorName = settings.value(key, defaultValue.name(QColor::HexArgb)).toString();

    if (!QColor::isValidColor(colorName)) {
        return defaultValue;
    }

    return QColor(colorName);
}

// A position from the settings file is only usable as long as the monitor it
// points at is still attached.
bool isOnAnyScreen(const QPoint &point)
{
    for (const QScreen *screen : QGuiApplication::screens()) {
        if (screen->geometry().contains(point)) {
            return true;
        }
    }

    return false;
}

} // namespace

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    // set window properties
    setWindowTitle(tr("Center Dot"));

    // load icon
    applicationIcon = new QIcon(":/icons/centerdot.ico");
    setWindowIcon(*applicationIcon);

    // set window flags
    // - FramelessWindowHint for window without border and title bar
    // - Tool for window without taskbar item
    // - WindowStaysOnTopHint for the window to stay on top
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);

    // never take the focus when the window is shown, a crosshair popping up
    // must not pull the player out of a running game
    setAttribute(Qt::WA_ShowWithoutActivating);

    // draw no background, keep window transparent, background will be drawn
    setAutoFillBackground(false);
    setAttribute(Qt::WA_TranslucentBackground);

    // set settings file name and try to load settings file if any
    settingsFileName = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/" + SETTINGS_FILE_NAME;
    loadSettings();

    // save settings on close
    connect(QApplication::instance(), SIGNAL(aboutToQuit()), this, SLOT(saveSettings()));

    // prepare keyboard hook
    WinKeyboardHook *winKeyboardHook = WinKeyboardHook::getInstance();
    if (!winKeyboardHook->connect()) {
        QMessageBox::warning(this, tr("Warning"), tr("Failed to establish hook to keyboard."));
    }
    connect(winKeyboardHook, SIGNAL(keyPressed(DWORD)), this, SLOT(handlePressedKey(DWORD)));

    // setup actions
    createActions();

    // setup systray
    createSystrayIcon();

    // show message on program start informing the user
    systrayIcon->showMessage(tr("Center Dot"), tr("Center dot was started and is showing the additional crosshair."));
}

MainWindow::~MainWindow()
{
}

void MainWindow::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    // the crosshair sits on the middle pixel of the window, the window itself
    // is sized by applySettings() so the whole shape fits
    const QPointF center(width() / 2 + 0.5, height() / 2 + 0.5);

    CrosshairRenderer::paint(painter, currentSettings, center);
}

void MainWindow::loadSettings()
{
    qDebug() << "Trying to load program settings from" << settingsFileName;

    QSettings settings(settingsFileName, QSettings::IniFormat, this);
    const SettingsData defaults;

    // read crosshair style
    currentSettings.style = CrosshairRenderer::styleFromKey(
        settings.value("style", CrosshairRenderer::styleKey(defaults.style)).toString(),
        defaults.style
    );

    // read the measurements of the crosshair
    currentSettings.crosshairSize = readBoundedInt(settings, "size", defaults.crosshairSize,
                                                   CrosshairRenderer::MIN_SIZE,
                                                   CrosshairRenderer::MAX_SIZE);
    currentSettings.lineThickness = readBoundedInt(settings, "lineThickness", defaults.lineThickness,
                                                   CrosshairRenderer::MIN_LINE_THICKNESS,
                                                   CrosshairRenderer::MAX_LINE_THICKNESS);
    currentSettings.gapSize       = readBoundedInt(settings, "gapSize", defaults.gapSize,
                                                   CrosshairRenderer::MIN_GAP_SIZE,
                                                   CrosshairRenderer::MAX_GAP_SIZE);
    currentSettings.armLength     = readBoundedInt(settings, "armLength", defaults.armLength,
                                                   CrosshairRenderer::MIN_ARM_LENGTH,
                                                   CrosshairRenderer::MAX_ARM_LENGTH);

    // read colors
    currentSettings.contourColor = readColor(settings, "contourColor", defaults.contourColor);
    currentSettings.fillColor    = readColor(settings, "fillColor", defaults.fillColor);

    // read the position of the crosshair
    bool positionIsKnown = false;
    QPoint center;

    if (settings.contains("centerX") && settings.contains("centerY")) {
        bool xIsInt = false;
        bool yIsInt = false;

        center.setX(settings.value("centerX").toInt(&xIsInt));
        center.setY(settings.value("centerY").toInt(&yIsInt));
        positionIsKnown = xIsInt && yIsInt;
    } else if (settings.contains("x") && settings.contains("y")) {
        // settings files written before crosshair styles existed stored the top
        // left corner of a window that was exactly as wide as the dot
        bool xIsInt = false;
        bool yIsInt = false;

        const int left = settings.value("x").toInt(&xIsInt);
        const int top  = settings.value("y").toInt(&yIsInt);

        center = QPoint(left, top) + QPoint(currentSettings.crosshairSize / 2,
                                            currentSettings.crosshairSize / 2);
        positionIsKnown = xIsInt && yIsInt;
    }

    // a position on a monitor that is no longer attached would put the
    // crosshair out of sight, only the position is dropped in that case
    if (!positionIsKnown || !isOnAnyScreen(center)) {
        qDebug() << "No usable crosshair position found, centering on the primary screen";
        centerOnPrimaryScreen();
    } else {
        qDebug() << "Crosshair position found: (x y) =" << center.x() << center.y();
        currentSettings.xPosition = center.x();
        currentSettings.yPosition = center.y();
    }

    applySettings();
}

void MainWindow::saveSettings()
{
    QSettings settings(settingsFileName, QSettings::IniFormat, this);

    // crosshair position, stored as the point the crosshair is centered on
    settings.setValue("centerX", currentSettings.xPosition);
    settings.setValue("centerY", currentSettings.yPosition);

    // the top left corner written by older versions, superseded by centerX/centerY
    settings.remove("x");
    settings.remove("y");

    // crosshair shape
    settings.setValue("style", CrosshairRenderer::styleKey(currentSettings.style));
    settings.setValue("size", currentSettings.crosshairSize);
    settings.setValue("lineThickness", currentSettings.lineThickness);
    settings.setValue("gapSize", currentSettings.gapSize);
    settings.setValue("armLength", currentSettings.armLength);

    // colors
    settings.setValue("contourColor", currentSettings.contourColor.name(QColor::HexArgb));
    settings.setValue("fillColor", currentSettings.fillColor.name(QColor::HexArgb));

    settings.sync();

    // log error or success
    if (settings.status() != QSettings::NoError) {
        qWarning() << "Error saving the settings, error code:" << settings.status();
    } else {
        qDebug() << "Settings saved";
    }
}

void MainWindow::acceptSettingsFromDialog()
{
    currentSettings = settingsDialog->getSettingsData();
    applySettings();

    qDebug() << "Updated settings";
}

void MainWindow::applySettings()
{
    // every style needs a window of its own size, a cross with long arms takes
    // up a lot more room than a plain dot
    const int side = CrosshairRenderer::boundingSide(currentSettings);
    setFixedSize(side, side);

    setCrosshairPosition(QPoint(currentSettings.xPosition, currentSettings.yPosition));

    // A translucent window keeps remains of the former crosshair on screen
    // after a resize, only going through hide() clears them. Thanks to the
    // WA_ShowWithoutActivating attribute this does not pull the focus away
    // from the running game, unlike the showMinimized()/showNormal() of
    // earlier versions.
    if (isVisible()) {
        hide();
        show();
    }

    update();
}

void MainWindow::handlePressedArrowKey(DWORD pressedKey)
{
    switch (pressedKey) {
        case VK_UP:
            moveCrosshairBy(0, -1);
            break;

        case VK_DOWN:
            moveCrosshairBy(0, 1);
            break;

        case VK_LEFT:
            moveCrosshairBy(-1, 0);
            break;

        case VK_RIGHT:
            moveCrosshairBy(1, 0);
            break;

        case VK_RETURN:
            // stop adjusting crosshair when enter key is pressed
            adjustDotAction->setChecked(false);
            break;
    }
}

void MainWindow::handlePressedKey(DWORD pressedKey)
{
    switch (pressedKey) {
        // Ctrl+H pressed?
        case 0x48: // H key
            // TODO Move to own function CtrlPressed in WinKeyboardHook
            if (GetAsyncKeyState(VK_CONTROL) & 0x8000) {
                toggleWindowVisibility();
            }
            break;
    }
}

void MainWindow::toggleWindowVisibility()
{
    if (isHidden()) {
        showHideAction->setText(tr("&Hide"));
        show();
        qDebug() << "Window shown";
    } else {
        showHideAction->setText(tr("&Show"));
        hide();
        qDebug() << "Window hidden";
    }
}

void MainWindow::adjustDot(bool checked)
{
    WinKeyboardHook* keyboardHookInstance = WinKeyboardHook::getInstance();

    if (checked) {
        qDebug() << "Enabled adjustment mode";

        if (!keyboardHookInstance->connect()) {
            QMessageBox::warning(this, tr("Warning"), tr("Failed to establish hook to keyboard."));
            return;
        }

        systrayIcon->showMessage(tr("Center Dot"), tr("Use the arrow keys on your keyboard to adjust the crosshair position. If you're done you can press Return or uncheck 'Adjust crosshair' again in the menu."));

        connect(keyboardHookInstance, SIGNAL(keyPressed(DWORD)), this, SLOT(handlePressedArrowKey(DWORD)));
    } else {
        qDebug() << "Disabled adjustment mode";

        // TODO disconnect now works for all slots, only delete the arrow key one
        disconnect(keyboardHookInstance, SIGNAL(keyPressed(DWORD)), 0, 0);
        keyboardHookInstance->disconnect();
    }
}

void MainWindow::showSettingsDialog()
{
    if (settingsDialog == nullptr) {
        settingsDialog = new SettingsDialog(this);
        connect(settingsDialog, SIGNAL(accepted()), this, SLOT(acceptSettingsFromDialog()));
    }
    settingsDialog->setSettingsData(currentSettings);
    settingsDialog->show();

    // the crosshair window stays on top, make sure the dialog does not end up
    // behind it or behind the game
    settingsDialog->raise();
    settingsDialog->activateWindow();
}

void MainWindow::resetDot()
{
    if (settingsDialog != nullptr && settingsDialog->isVisible()) {
        settingsDialog->close();
    }

    // back to the built in defaults, position included
    currentSettings = SettingsData();
    centerOnPrimaryScreen();

    applySettings();
}

void MainWindow::centerOnPrimaryScreen()
{
    const QPoint screenCenterPoint = QGuiApplication::primaryScreen()->availableGeometry().center();

    currentSettings.xPosition = screenCenterPoint.x();
    currentSettings.yPosition = screenCenterPoint.y();
}

void MainWindow::showAboutDlg()
{
    QString text = QString(
        QCoreApplication::applicationName() + "\n" +
        tr("Version") + " " + QCoreApplication::applicationVersion() + "\n" +
        tr("Shows a centered crosshair as a layer above your game.") + "\n"
        "(c) 2016 Atomkraftzwerg"
    );

    QMessageBox::about(this, tr("About Center Dot"), text);
}

void MainWindow::quitProgram()
{
    QApplication::quit();
}

void MainWindow::createActions()
{
    showHideAction = new QAction(tr("&Hide"), this);
    showHideAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_H));
    connect(showHideAction, SIGNAL(triggered()), this, SLOT(toggleWindowVisibility()));

    adjustDotAction = new QAction(tr("&Adjust dot"), this);
    adjustDotAction->setCheckable(true);
    adjustDotAction->setChecked(false);
    connect(adjustDotAction, SIGNAL(toggled(bool)), this, SLOT(adjustDot(bool)));

    resetDotAction = new QAction(tr("&Reset dot"), this);
    connect(resetDotAction, SIGNAL(triggered()), this, SLOT(resetDot()));

    settingsAction = new QAction(tr("&Settings..."), this);
    connect(settingsAction, SIGNAL(triggered()), this, SLOT(showSettingsDialog()));

    aboutAction = new QAction(tr("A&bout..."), this);
    connect(aboutAction, SIGNAL(triggered()), this, SLOT(showAboutDlg()));

    closeAction = new QAction(tr("&Exit"), this);
    connect(closeAction, SIGNAL(triggered()), this, SLOT(quitProgram()));
}

void MainWindow::createSystrayIcon()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        QMessageBox::warning(this, tr("System tray is unavailable"), tr("System tray unavailable"));
        return;
    }

    systrayIcon = new QSystemTrayIcon(*applicationIcon, this);
    systrayIcon->setToolTip(tr("Center Dot"));

    systrayMenu = new QMenu(this);
    systrayMenu->addAction(showHideAction);
    systrayMenu->addSeparator();
    systrayMenu->addAction(adjustDotAction);
    systrayMenu->addAction(resetDotAction);
    systrayMenu->addAction(settingsAction);
    systrayMenu->addSeparator();
    systrayMenu->addAction(aboutAction);
    systrayMenu->addAction(closeAction);
    systrayIcon->setContextMenu(systrayMenu);

    systrayIcon->show();
}

void MainWindow::moveCrosshairBy(int deltaX, int deltaY)
{
    currentSettings.xPosition += deltaX;
    currentSettings.yPosition += deltaY;

    setCrosshairPosition(QPoint(currentSettings.xPosition, currentSettings.yPosition));

    qDebug() << "Moved crosshair, new center:" << currentSettings.xPosition << currentSettings.yPosition;
}
