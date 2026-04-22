#pragma once

#include "PlotJuggler/messageparser_base.h"

class ParserMavlinkRest : public PJ::ParserFactoryPlugin
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "facontidavide.PlotJuggler3.ParserFactoryPlugin")
  Q_INTERFACES(PJ::ParserFactoryPlugin)

public:
  const char* name() const override
  {
    return "ParserMavlinkRest";
  }

  const char* encoding() const override
  {
    return "mavlink_rest";
  }

  PJ::MessageParserPtr createParser(const std::string& topic_name, const std::string& type_name,
                                    const std::string& schema,
                                    PJ::PlotDataMapRef& data) override;
};
