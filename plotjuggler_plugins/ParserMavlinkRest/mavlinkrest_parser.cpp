#include "mavlinkrest_parser.h"

#include "nlohmann/json.hpp"

#include <chrono>
#include <functional>
#include <string>

using namespace PJ;

class MsgParserImpl : public MessageParser
{
public:
  MsgParserImpl(const std::string& topic_name, PlotDataMapRef& data)
    : MessageParser(topic_name, data)
  {
  }

  bool parseMessage(const MessageRef msg, double& timestamp) override
  {
    nlohmann::json doc;
    try
    {
      doc = nlohmann::json::parse(msg.data(), msg.data() + msg.size());
    }
    catch (...)
    {
      return false;
    }

    if (!doc.is_object())
      return false;

    // Use message.type as the series root prefix (e.g. "ATTITUDE", "RC_CHANNELS")
    std::string prefix;
    auto msg_it = doc.find("message");
    if (msg_it != doc.end() && msg_it->is_object())
    {
      auto type_it = msg_it->find("type");
      if (type_it != msg_it->end() && type_it->is_string())
        prefix = type_it->get<std::string>();

      // Use time_boot_ms (ms) as timestamp when available
      auto time_it = msg_it->find("time_boot_ms");
      if (time_it != msg_it->end() && time_it->is_number())
        timestamp = time_it->get<double>() * 1e-3;
    }

    std::function<void(const std::string&, const nlohmann::json&)> flatten;
    flatten = [&](const std::string& key, const nlohmann::json& val) {
      if (val.is_object())
      {
        for (const auto& [k, v] : val.items())
          flatten(key + "/" + k, v);
      }
      else if (val.is_array())
      {
        for (size_t i = 0; i < val.size(); ++i)
          flatten(key + "[" + std::to_string(i) + "]", val[i]);
      }
      else if (val.is_number())
      {
        getSeries(key).pushBack({ timestamp, val.get<double>() });
      }
      else if (val.is_boolean())
      {
        getSeries(key).pushBack({ timestamp, val.get<bool>() ? 1.0 : 0.0 });
      }
      // strings are silently skipped
    };

    flatten(prefix, doc);
    return true;
  }
};

MessageParserPtr ParserMavlinkRest::createParser(const std::string& topic_name,
                                                  const std::string&, const std::string&,
                                                  PlotDataMapRef& data)
{
  return std::make_shared<MsgParserImpl>(topic_name, data);
}
