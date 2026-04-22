add_library(ParserMavlinkRest SHARED
  mavlinkrest_parser.h
  mavlinkrest_parser.cpp
)

target_link_libraries(ParserMavlinkRest PRIVATE
  Qt5::Widgets
  plotjuggler_base
  nlohmann_json::nlohmann_json
)

target_compile_definitions(ParserMavlinkRest PRIVATE QT_PLUGIN)

install(TARGETS ParserMavlinkRest DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY})
