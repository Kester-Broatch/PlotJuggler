#include "websocket_client.h"
#include "websocket_dialog.h"

#include <QMessageBox>
#include <QSettings>

#include <chrono>

// =======================
// WebsocketClient
// =======================
WebsocketClient::WebsocketClient()
{
  loadDefaultSettings();
  setupSettings();

  connect(&_socket, &QWebSocket::connected, this, &WebsocketClient::onConnected);
  connect(&_socket, &QWebSocket::textMessageReceived, this,
          &WebsocketClient::onTextMessageReceived);
  connect(&_socket, &QWebSocket::binaryMessageReceived, this,
          &WebsocketClient::onBinaryMessageReceived);
  connect(&_socket, &QWebSocket::disconnected, this, &WebsocketClient::onDisconnected);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  connect(&_socket, &QWebSocket::errorOccurred, this, &WebsocketClient::onError);
#else
  connect(&_socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error), this,
          &WebsocketClient::onError);
#endif
}

void WebsocketClient::setupSettings()
{
  _action_settings = new QAction("Pause", this);

  connect(_action_settings, &QAction::triggered, this, [this]() {
    if (!_running)
      return;

    if (_paused)
    {
      if (resume())
      {
        _paused = false;
        _action_settings->setText("Pause");
      }
    }
    else
    {
      if (pause())
      {
        _paused = true;
        _action_settings->setText("Resume");
      }
    }
  });

  _actions = { _action_settings };
}

// =======================
// PlotJuggler actions
// =======================
const std::vector<QAction*>& WebsocketClient::availableActions()
{
  return _actions;
}

// =======================
// Start client
// =======================
bool WebsocketClient::start(QStringList*)
{
  if (_running)
    return true;

  if (parserFactories() == nullptr || parserFactories()->empty())
  {
    QMessageBox::warning(nullptr, "WebSocket Client", "No available MessageParsers",
                         QMessageBox::Ok);
    return false;
  }

  WebsocketDialog dialog(_config);
  _dialog = &dialog;

  // Populate protocol combobox from available parsers
  for (const auto& it : *parserFactories())
  {
    dialog.addProtocol(it.first, it.second->optionsWidget());
  }
  dialog.setSelectedProtocol(_config.protocol);

  // Connect button: open/close socket
  connect(dialog.connectButton(), &QPushButton::toggled, this, [&](bool checked) {
    if (checked)
    {
      const QString urlStr = dialog.url();
      _url = QUrl(urlStr);
      if (!_url.isValid() || urlStr.isEmpty())
      {
        QMessageBox::warning(&dialog, "WebSocket Client", "Invalid URL", QMessageBox::Ok);
        dialog.setConnected(false);
        return;
      }
      _config.url = urlStr;
      _socket.open(_url);
    }
    else
    {
      _socket.abort();
      _socket.close();
      _running = false;
      dialog.setOkButton("Start", false);
    }
  });

  // OK button: create parser and begin streaming
  connect(dialog.buttonBox(), &QDialogButtonBox::accepted, this, [&]() {
    if (_socket.state() != QAbstractSocket::ConnectedState)
      return;

    const QString proto = dialog.selectedProtocol();
    auto it = parserFactories()->find(proto);
    if (it == parserFactories()->end())
      return;

    _parser = it->second->createParser({}, {}, {}, dataMap());
    _config.url = dialog.url();
    _config.protocol = proto;
    saveDefaultSettings();

    _running = true;
    dialog.accept();
  });

  // Cancel button
  connect(dialog.buttonBox(), &QDialogButtonBox::rejected, this, [&]() {
    shutdown();
    dialog.reject();
  });

  dialog.exec();
  _dialog = nullptr;

  if (!_running || !_parser)
  {
    shutdown();
    return false;
  }

  return true;
}

void WebsocketClient::shutdown()
{
  if (!_running && _socket.state() == QAbstractSocket::UnconnectedState)
    return;

  _running = false;
  _paused = false;
  _parser.reset();

  if (_action_settings)
    _action_settings->setText("Pause");

  if (_dialog)
  {
    _dialog->reject();
    _dialog = nullptr;
  }

#ifdef PJ_BUILD
  dataMap().clear();
  emit dataReceived();
#endif

  _closing = true;
  _socket.abort();
  _socket.close();
}

bool WebsocketClient::pause()
{
  if (!_running)
    return false;
  _paused = true;
  return true;
}

bool WebsocketClient::resume()
{
  if (!_running)
    return false;
  _paused = false;
  return true;
}

// =======================
// Socket events
// =======================
void WebsocketClient::onConnected()
{
  if (_dialog)
  {
    _dialog->setConnected(true);
    _dialog->setOkButton("Start", true);
  }
}

void WebsocketClient::onDisconnected()
{
  if (_dialog)
  {
    _dialog->setConnected(false);
    _dialog->setOkButton("Start", false);
  }

  if (_closing)
  {
    _closing = false;
    return;
  }

  if (_running)
  {
    _running = false;
    if (!_dialog)
    {
      QMessageBox::warning(nullptr, "WebSocket Client", "Server closed the connection",
                           QMessageBox::Ok);
    }
  }
}

void WebsocketClient::onError(QAbstractSocket::SocketError)
{
  QMessageBox::warning(nullptr, "WebSocket Client", _socket.errorString(), QMessageBox::Ok);
  onDisconnected();
}

// =======================
// Message parsing
// =======================
void WebsocketClient::parseMessage(const uint8_t* data, size_t size)
{
  if (!_running || _paused || !_parser)
    return;

  using namespace std::chrono;
  auto ts = high_resolution_clock::now().time_since_epoch();
  double timestamp = 1e-6 * double(duration_cast<microseconds>(ts).count());

  std::lock_guard<std::mutex> lock(mutex());

  PJ::MessageRef msg(data, size);
  try
  {
    _parser->parseMessage(msg, timestamp);
  }
  catch (std::exception& err)
  {
    QMessageBox::warning(nullptr, "WebSocket Client",
                         tr("Problem parsing the message. WebSocket Client will be "
                            "stopped.\n%1")
                             .arg(err.what()),
                         QMessageBox::Ok);
    shutdown();
    emit closed();
    return;
  }

  emit dataReceived();
}

void WebsocketClient::onTextMessageReceived(const QString& message)
{
  QByteArray bytes = message.toUtf8();
  parseMessage(reinterpret_cast<const uint8_t*>(bytes.constData()), bytes.size());
}

void WebsocketClient::onBinaryMessageReceived(const QByteArray& message)
{
  parseMessage(reinterpret_cast<const uint8_t*>(message.constData()), message.size());
}

// =======================
// PlotJuggler profiles
// =======================
void WebsocketClient::saveDefaultSettings()
{
  QSettings s;
  _config.saveToSettings(s, "WebsocketClient");
}

void WebsocketClient::loadDefaultSettings()
{
  QSettings s;
  _config.loadFromSettings(s, "WebsocketClient");
}

bool WebsocketClient::xmlSaveState(QDomDocument& doc, QDomElement& parent) const
{
  _config.xmlSaveState(doc, parent);
  return true;
}

bool WebsocketClient::xmlLoadState(const QDomElement& parent)
{
  _config.xmlLoadState(parent);
  return true;
}
