#ifndef CONFIG_H
#define CONFIG_H

#include <string>

struct Config
{
    Config(bool fullscreenIn, bool msaaIn, int widthIn, int heightIn) :
        fullscreen(fullscreenIn), msaa(msaaIn), msaaSamples(4),
        width(widthIn), height(heightIn) {};

    bool fullscreen;
    bool msaa;
    int msaaSamples;
    int width;
    int height;
    std::string title;
};

extern Config* s_config;

#endif // CONFIG_H
