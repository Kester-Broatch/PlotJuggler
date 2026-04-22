#include "websocket_dialog.h"

#include <QPushButton>
#include <QSettings>

#include "ui_websocket_client.h"

WebsocketDialog::WebsocketDialog(const WebsocketClientConfig& config)
  : QDialog(nullptr), ui(new Ui::WebSocketDialog)
{
  ui->setupUi(this);
  setWindowTitle("WebSocket Client");

  ui->lineEditUrl->setText(config.url);

  setOkButton("Start", false);

  connect(ui->comboBoxProtocol, &QComboBox::currentTextChanged, this,
          &WebsocketDialog::onProtocolChanged);

  QSettings s;
  restoreGeometry(s.value("WebsocketClient/dialogGeometry").toByteArray());
}

WebsocketDialog::~WebsocketDialog()
{
  // Options widgets are owned by the parser factories, not by this dialog.
  // Remove them from the layout and reparent to nullptr so Qt does not
  // destroy them when the UI is deleted.
  while (ui->layoutOptions->count() > 0)
  {
    auto* item = ui->layoutOptions->takeAt(0);
    if (item->widget())
      item->widget()->setParent(nullptr);
    delete item;
  }

  QSettings s;
  s.setValue("WebsocketClient/dialogGeometry", saveGeometry());
  delete ui;
}

// --- URL ---

QString WebsocketDialog::url() const
{
  return ui->lineEditUrl->text().trimmed();
}

// --- Protocol ---

QString WebsocketDialog::selectedProtocol() const
{
  return ui->comboBoxProtocol->currentText();
}

void WebsocketDialog::setSelectedProtocol(const QString& name)
{
  ui->comboBoxProtocol->setCurrentText(name);
}

void WebsocketDialog::addProtocol(const QString& name, QWidget* options_widget)
{
  ui->comboBoxProtocol->addItem(name);
  if (options_widget)
  {
    options_widget->setVisible(false);
    ui->layoutOptions->addWidget(options_widget);
  }
}

void WebsocketDialog::onProtocolChanged(const QString&)
{
  if (_current_options_widget)
  {
    _current_options_widget->setVisible(false);
    _current_options_widget = nullptr;
  }

  // Show the options widget for the newly selected protocol (if any).
  // The widgets were added to layoutOptions during addProtocol(); we iterate
  // to find the one that belongs to the current selection.  We rely on the
  // caller having added widgets in the same order as the combobox items.
  const int idx = ui->comboBoxProtocol->currentIndex();
  if (idx >= 0 && idx < ui->layoutOptions->count())
  {
    auto* item = ui->layoutOptions->itemAt(idx);
    if (item && item->widget())
    {
      _current_options_widget = item->widget();
      _current_options_widget->setVisible(true);
    }
  }

  adjustSize();
}

// --- Connection state ---

void WebsocketDialog::setConnected(bool connected)
{
  ui->buttonConnect->blockSignals(true);
  ui->buttonConnect->setChecked(connected);
  ui->buttonConnect->setText(connected ? "Connected" : "Connect");
  ui->buttonConnect->blockSignals(false);

  ui->lineEditUrl->setEnabled(!connected);
  ui->comboBoxProtocol->setEnabled(!connected);
}

// --- OK button ---

void WebsocketDialog::setOkButton(const QString& text, bool enabled)
{
  auto* b = ui->buttonBox->button(QDialogButtonBox::Ok);
  if (b)
  {
    b->setText(text);
    b->setEnabled(enabled);
  }
}

// --- Signal access ---

QDialogButtonBox* WebsocketDialog::buttonBox() const
{
  return ui->buttonBox;
}

QPushButton* WebsocketDialog::connectButton() const
{
  return ui->buttonConnect;
}
