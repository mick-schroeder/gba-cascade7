#include "bn_core.h"

#include "cascade7/game.h"
#include "cascade7/renderer.h"

int main()
{
    bn::core::init();

    cascade7::game game;
    cascade7::renderer renderer;

    while(true)
    {
        game.update();
        renderer.draw(game);
        bn::core::update();
    }
}
