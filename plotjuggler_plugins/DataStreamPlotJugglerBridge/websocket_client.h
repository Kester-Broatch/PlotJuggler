#pragma once

#include <QWebSocket>
#include <QTimer>
#include <QElapsedTimer>

#include "websocket_client_config.h"
#include "websocket_dialog.h"

#include "PlotJuggler/datastreamer_base.h"
#include "PlotJuggler/messageparser_base.h"

class WebsocketClient : public PJ::DataStreamer
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "facontidavide.PlotJuggler3.DataStreamer")
  Q_INTERFACES(PJ::DataStreamer)

public:
  WebsocketClient();

  const std::vector<QAction*>& availableActions() override;

  virtual bool start(QStringList*) override;

  virtual void shutdown() override;

  virtual bool isRunning() const override
  {
    return _running;
  }

  ~WebsocketClient() override
  {
    shutdown();
  }

  virtual const char* name() const override
  {
    return "WebSocket Client";
  }

  virtual bool isDebugPlugin() override
  {
    return false;
  }

  bool xmlSaveState(QDomDocument& doc, QDomElement& parent_element) const override;
  bool xmlLoadState(const QDomElement& parent_element) override;

  bool pause();
  bool resume();

private:
  QAction* _action_settings = nullptr;
  std::vector<QAction*> _actions;

  WebsocketClientConfig _config;

  QWebSocket _socket;
  QUrl _url;
  bool _running = false;
  bool _closing = false;
  bool _paused = false;

  PJ::MessageParserPtr _parser;

  QPointer<WebsocketDialog> _dialog;

  void setupSettings();
  void saveDefaultSettings();
  void loadDefaultSettings();

  void parseMessage(const uint8_t* data, size_t size);

private slots:
  void onConnected();
  void onTextMessageReceived(const QString& message);
  void onBinaryMessageReceived(const QByteArray& message);
  void onDisconnected();
  void onError(QAbstractSocket::SocketError);
};
