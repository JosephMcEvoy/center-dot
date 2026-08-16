#include <QColorDialog>
#include <QDebug>

#include "settingsdialog.h"
#include "crosshairrenderer.h"

namespace {

// edge length of the preview area, large enough to hold the biggest crosshair
const QSize PREVIEW_SIZE(170, 170);

// a dark backdrop, a white crosshair on the grey of a dialog is hard to judge
const char *PREVIEW_STYLE_SHEET = "background-color: #1e1e1e; border: 1px solid #3c3c3c;";

QString pixelText(int value)
{
    return QString::number(value) + " px";
}

} // namespace

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent)
{
    ui.setupUi(this);
    this->setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // offer every style the renderer knows, each entry carries its own value
    updatingControls = true;
    for (const CrosshairRenderer::StyleInfo &info : CrosshairRenderer::styles()) {
        ui.styleCombo->addItem(info.displayName, static_cast<int>(info.style));
    }
    updatingControls = false;

    // take the slider bounds from the renderer so both cannot drift apart
    ui.crosshairSizeSlider->setRange(CrosshairRenderer::MIN_SIZE, CrosshairRenderer::MAX_SIZE);
    ui.lineThicknessSlider->setRange(CrosshairRenderer::MIN_LINE_THICKNESS, CrosshairRenderer::MAX_LINE_THICKNESS);
    ui.gapSizeSlider->setRange(CrosshairRenderer::MIN_GAP_SIZE, CrosshairRenderer::MAX_GAP_SIZE);
    ui.armLengthSlider->setRange(CrosshairRenderer::MIN_ARM_LENGTH, CrosshairRenderer::MAX_ARM_LENGTH);

    ui.previewLbl->setFixedSize(PREVIEW_SIZE);
    ui.previewLbl->setStyleSheet(PREVIEW_STYLE_SHEET);
}

void SettingsDialog::setSettingsData(const SettingsData &settingsData)
{
    this->settingsData = settingsData;

    updatingControls = true;

    // update position view
    ui.xEdit->setText(QString::number(settingsData.xPosition));
    ui.yEdit->setText(QString::number(settingsData.yPosition));

    // update crosshair style view
    const int styleIndex = ui.styleCombo->findData(static_cast<int>(settingsData.style));
    ui.styleCombo->setCurrentIndex(styleIndex < 0 ? 0 : styleIndex);

    // update the measurement views
    ui.crosshairSizeSlider->setValue(settingsData.crosshairSize);
    ui.lineThicknessSlider->setValue(settingsData.lineThickness);
    ui.gapSizeSlider->setValue(settingsData.gapSize);
    ui.armLengthSlider->setValue(settingsData.armLength);

    // update color views
    showColorInBox(ui.contourColorBox, settingsData.contourColor);
    showColorInBox(ui.fillColorBox, settingsData.fillColor);

    updatingControls = false;

    refreshControls();
}

SettingsData SettingsDialog::getSettingsData()
{
    return this->settingsData;
}

void SettingsDialog::refreshControls()
{
    const CrosshairRenderer::StyleInfo &info = CrosshairRenderer::styleInfo(settingsData.style);

    // a style that ignores one of the measurements greys the control out
    ui.crosshairSizeLbl->setEnabled(info.usesSize);
    ui.crosshairSizeSlider->setEnabled(info.usesSize);
    ui.crosshairSizeValueLbl->setEnabled(info.usesSize);

    ui.lineThicknessLbl->setEnabled(info.usesThickness);
    ui.lineThicknessSlider->setEnabled(info.usesThickness);
    ui.lineThicknessValueLbl->setEnabled(info.usesThickness);

    ui.gapSizeLbl->setEnabled(info.usesGap);
    ui.gapSizeSlider->setEnabled(info.usesGap);
    ui.gapSizeValueLbl->setEnabled(info.usesGap);

    ui.armLengthLbl->setEnabled(info.usesArmLength);
    ui.armLengthSlider->setEnabled(info.usesArmLength);
    ui.armLengthValueLbl->setEnabled(info.usesArmLength);

    ui.crosshairSizeValueLbl->setText(pixelText(settingsData.crosshairSize));
    ui.lineThicknessValueLbl->setText(pixelText(settingsData.lineThickness));
    ui.gapSizeValueLbl->setText(pixelText(settingsData.gapSize));
    ui.armLengthValueLbl->setText(pixelText(settingsData.armLength));

    updatePreview();
}

void SettingsDialog::updatePreview()
{
    ui.previewLbl->setPixmap(
        CrosshairRenderer::renderPixmap(settingsData, PREVIEW_SIZE, devicePixelRatioF())
    );
}

void SettingsDialog::showColorInBox(QFrame *colorBox, const QColor &color)
{
    if (!color.isValid()) {
        return;
    }

    QPalette palette = colorBox->palette();
    palette.setColor(colorBox->backgroundRole(), color);
    colorBox->setPalette(palette);
}

void SettingsDialog::pickColor(QFrame *colorBox, QColor &target)
{
    const QColor color = QColorDialog::getColor(target, this, QString(), QColorDialog::ShowAlphaChannel);

    if (color.isValid()) {
        target = color;
        showColorInBox(colorBox, color);
        updatePreview();
    }
}

void SettingsDialog::on_styleCombo_currentIndexChanged(int index)
{
    if (updatingControls || index < 0) {
        return;
    }

    settingsData.style = static_cast<CrosshairStyle>(ui.styleCombo->itemData(index).toInt());
    qDebug() << "Crosshair style changed to" << CrosshairRenderer::styleKey(settingsData.style);

    refreshControls();
}

void SettingsDialog::on_xEdit_textEdited(const QString &arg1)
{
    bool validIntEntered = false;
    int newXvalue = arg1.toInt(&validIntEntered);

    if (validIntEntered) {
        settingsData.xPosition = newXvalue;
    } else {
        ui.xEdit->setText(QString::number(settingsData.xPosition));
    }
}


void SettingsDialog::on_yEdit_textEdited(const QString &arg1)
{
    bool validIntEntered = false;
    int newYvalue = arg1.toInt(&validIntEntered);

    if (validIntEntered) {
        settingsData.yPosition = newYvalue;
    } else {
        ui.yEdit->setText(QString::number(settingsData.yPosition));
    }
}

void SettingsDialog::on_rightBtn_clicked()
{
    settingsData.xPosition++;
    ui.xEdit->setText(QString::number(settingsData.xPosition));
}

void SettingsDialog::on_leftBtn_clicked()
{
    settingsData.xPosition--;
    ui.xEdit->setText(QString::number(settingsData.xPosition));
}

void SettingsDialog::on_downBtn_clicked()
{
    settingsData.yPosition++;
    ui.yEdit->setText(QString::number(settingsData.yPosition));
}

void SettingsDialog::on_upBtn_clicked()
{
    settingsData.yPosition--;
    ui.yEdit->setText(QString::number(settingsData.yPosition));
}

void SettingsDialog::on_crosshairSizeSlider_valueChanged(int value)
{
    if (updatingControls) {
        return;
    }

    settingsData.crosshairSize = value;
    refreshControls();
}

void SettingsDialog::on_lineThicknessSlider_valueChanged(int value)
{
    if (updatingControls) {
        return;
    }

    settingsData.lineThickness = value;
    refreshControls();
}

void SettingsDialog::on_gapSizeSlider_valueChanged(int value)
{
    if (updatingControls) {
        return;
    }

    settingsData.gapSize = value;
    refreshControls();
}

void SettingsDialog::on_armLengthSlider_valueChanged(int value)
{
    if (updatingControls) {
        return;
    }

    settingsData.armLength = value;
    refreshControls();
}

void SettingsDialog::on_selectContourColorBtn_clicked()
{
    pickColor(ui.contourColorBox, settingsData.contourColor);
}

void SettingsDialog::on_selectFillColorBtn_clicked()
{
    pickColor(ui.fillColorBox, settingsData.fillColor);
}

void SettingsDialog::on_buttonBox_accepted()
{
    qDebug() << "Settings submitted";
}
