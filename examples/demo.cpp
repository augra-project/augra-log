// This file is part of the augra-log Project.
// License: GPL-3.0-or-later. Contact: augra-project@trinity2k.net
//

#include <augra/log.h>

#include <cstdio>
#include <memory>

namespace {

void print_banner()
{
    auto& logger = augra::Logger::instance();
    logger.info("augra_log", "");
    logger.info("augra_log", "       ****");
    logger.info("augra_log", "     *@@@@@@*            A U G R A   L O G");
    logger.info("augra_log", "   *@@#**#@@* .          v0.1.0");
    logger.info("augra_log", " . *@@#*  #@@* .");
    logger.info("augra_log", "   *@@#**#@@*            https://gitlab.com/the-augra-project/");
    logger.info("augra_log", "     *@@@@@@*");
    logger.info("augra_log", "       ****              greetings to all who preserve. keep the old games alive.");
    logger.info("augra_log", "");
}

} // anonymous namespace

int main()
{
    auto& logger = augra::Logger::instance();

    print_banner();

    augra::log_info("demo", "the logger starts at Info level by default");
    augra::log_warn("demo", "warnings pass through");
    augra::log_debug("demo", "this debug message is filtered out");

    logger.set_component_level("demo", augra::LogLevel::Debug);
    augra::log_debug("demo", "now debug is visible for the demo component");
    augra::log_debug("other", "but not for other components");
    logger.clear_component_level("demo");

    auto file = std::make_shared<augra::FileHandler>("demo.log");
    if (file->is_open()) {
        logger.add_handler(file);
        augra::log_info("demo", "messages now go to stderr and demo.log");
    }

    file->set_level(augra::LogLevel::Warn);
    augra::log_info("demo", "this info goes to stderr only");
    augra::log_warn("demo", "this warning goes to both");

    file->set_format("{level}:{component}:{message}");
    augra::log_error("demo", "compact format in the log file");

    logger.remove_handler(file);
    augra::log_info("demo", "done");

    return 0;
}
