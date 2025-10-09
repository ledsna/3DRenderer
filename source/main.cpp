#include "engine/Application.h"
#include "utils/Dimensions.h"

int main() {
    rasterizer::Application app(rasterizer::PixelWidth(1280), rasterizer::PixelHeight(720), "CPU Rasterizer");
    app.run();
    return 0;
}