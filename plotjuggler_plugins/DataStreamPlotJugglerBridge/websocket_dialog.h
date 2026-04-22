#pragma once

#include <QDialog>
#include <QDialogButtonBox>
#include <QPushButton>

#include "websocket_client_config.h"

namespace Ui
{
class WebSocketDialog;
}

class WebsocketDialog : public QDialog
{
public:
  explicit WebsocketDialog(const WebsocketClientConfig& config);
  ~WebsocketDialog();

  // URL
  QString url() const;

  // Protocol selection
  QString selectedProtocol() const;
  void setSelectedProtocol(const QString& name);
  void addProtocol(const QString& name, QWidget* options_widget);

  // Connection state
  void setConnected(bool connected);

  // OK button
  void setOkButton(const QString& text, bool enabled);

  // Signal access for external connections
  QDialogButtonBox* buttonBox() const;
  QPushButton* connectButton() const;

private slots:
  void onProtocolChanged(const QString& protocol);

private:
  Ui::WebSocketDialog* ui;
  QWidget* _current_options_widget = nullptr;
};
