#include "Game/MinecraftApp.h"

int main() {
    MinecraftApp app;
    if (!app.init()) {
        return -1;
    }

    app.run();
    return 0;
}
