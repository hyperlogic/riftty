#ifndef APPCONFIG_H
#define APPCONFIG_H

#include <string>

struct AppConfig
{
    AppConfig(bool fullscreenIn, bool msaaIn, int widthIn, int heightIn) :
        fullscreen(fullscreenIn), msaa(msaaIn), msaaSamples(4),
        width(widthIn), height(heightIn) {};

    bool fullscreen;
    bool msaa;
    int msaaSamples;
    int width;
    int height;
    std::string title;
};

extern AppConfig* s_config;

#endif // CONFIG_H
