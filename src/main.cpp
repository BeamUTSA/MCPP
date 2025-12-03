#include "Game/MinecraftApp.h"
#define GLM_ENABLE_EXPERIMENTAL
int main() {
    MinecraftApp app;
    if (!app.init()) {
        return -1;
    }

    app.run();
    return 0;
}
