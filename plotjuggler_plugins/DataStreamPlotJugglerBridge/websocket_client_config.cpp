#include "websocket_client_config.h"

WebsocketClientConfig::WebsocketClientConfig() = default;

// =========================
// XML (PlotJuggler layout)
// =========================
void WebsocketClientConfig::xmlSaveState(QDomDocument& doc, QDomElement& plugin_elem) const
{
  QDomElement cfg = doc.createElement("websocket_client");
  plugin_elem.appendChild(cfg);

  cfg.setAttribute("url", url);
  cfg.setAttribute("protocol", protocol);
}

void WebsocketClientConfig::xmlLoadState(const QDomElement& parent_element)
{
  QDomElement cfg = parent_element.firstChildElement("websocket_client");
  if (cfg.isNull())
  {
    return;
  }

  // Support legacy XML that stored address + port separately
  if (cfg.hasAttribute("url"))
  {
    url = cfg.attribute("url", "ws://127.0.0.1:9090");
  }
  else
  {
    const QString addr = cfg.attribute("address", "127.0.0.1");
    const int p = cfg.attribute("port", "9090").toInt();
    url = QString("ws://%1:%2").arg(addr).arg(p);
  }

  protocol = cfg.attribute("protocol", "JSON");
}

// =========================
// QSettings (global defaults)
// =========================
void WebsocketClientConfig::saveToSettings(QSettings& settings, const QString& group) const
{
  settings.setValue(group + "/url", url);
  settings.setValue(group + "/protocol", protocol);
}

void WebsocketClientConfig::loadFromSettings(const QSettings& settings, const QString& group)
{
  // Support legacy QSettings that stored address + port separately
  if (settings.contains(group + "/url"))
  {
    url = settings.value(group + "/url", "ws://127.0.0.1:9090").toString();
  }
  else
  {
    const QString addr = settings.value(group + "/address", "127.0.0.1").toString();
    const int p = settings.value(group + "/port", 9090).toInt();
    url = QString("ws://%1:%2").arg(addr).arg(p);
  }

  protocol = settings.value(group + "/protocol", "JSON").toString();
}
