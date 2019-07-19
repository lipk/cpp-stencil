#include <SDL.h>
#include <buffer.hpp>
#include <cmath>
#include <cstdlib>
#include <iostream>

#define ASSERT_SDL(expr)                                                       \
    if (!(expr)) {                                                             \
        std::cerr << SDL_GetError() << std::endl;                              \
        std::exit(1);                                                          \
    }

using namespace stencil;

double distsq(double x1, double y1, double x2, double y2)
{
    const double xd = x1 - x2;
    const double yd = y1 - y2;
    return xd * xd + yd * yd;
}

struct simulation
{
    buffer<2, double> buf00, buf01, buf10, buf11;
    double t;

    simulation()
        : buf00({ 400, 400 }, 1)
        , buf01({ 400, 400 }, 1)
        , buf10({ 400, 400 }, 1)
        , buf11({ 400, 400 }, 1)
        , t(0)
    {
        buf00.fill(0);
        buf01.fill(0);
        buf10.fill(0);
        buf11.fill(0);
    }

    void run_one_iteration(SDL_Renderer* renderer)
    {
        buf00.copy_halo_from(buf10, { 1, 0 });
        buf00.copy_halo_from(buf01, { 0, 1 });
        buf00.copy_halo_from(buf11, { 1, 1 });

        buf01.copy_halo_from(buf10, { 1, -1 });
        buf01.copy_halo_from(buf00, { 0, -1 });
        buf01.copy_halo_from(buf11, { 1, 0 });

        buf10.copy_halo_from(buf00, { -1, 0 });
        buf10.copy_halo_from(buf01, { -1, 1 });
        buf10.copy_halo_from(buf11, { 0, 1 });

        buf11.copy_halo_from(buf10, { 0, -1 });
        buf11.copy_halo_from(buf01, { -1, 0 });
        buf11.copy_halo_from(buf00, { -1, -1 });

        double source_x = 400 + cos(t) * 300;
        double source_y = 400 + sin(t) * 300;

        u64 xoff = 0, yoff = 0;
        auto iterate_func = [&](const std::array<u64, 2>& it,
                                buffer<2, double>::accessor<1>& acc) {
            if (distsq(source_x, source_y, it[0] + xoff, it[1] + yoff) < 25) {
                acc.get({ 0, 0 }) = 1.0;
            } else {
                acc.get({ 0, 0 }) =
                    acc.get({ 0, 0 }) +
                    0.1 * ((acc.get({ 0, -1 }) - 2 * acc.get({ 0, 0 }) +
                            acc.get({ 1, 0 })) +
                           (acc.get({ 0, -1 }) - 2 * acc.get({ 0, 0 }) +
                            acc.get({ 0, 1 })));
            }
            int color = acc.get({ 0, 0 }) * 255;
            SDL_SetRenderDrawColor(
                renderer, color, color, color, SDL_ALPHA_OPAQUE);
            SDL_RenderDrawPoint(renderer, it[0] + xoff, it[1] + yoff);
        };

        buf00.iterate<1>(iterate_func);
        xoff = 400;
        yoff = 0;
        buf10.iterate<1>(iterate_func);
        xoff = 0;
        yoff = 400;
        buf01.iterate<1>(iterate_func);
        xoff = 400;
        yoff = 400;
        buf11.iterate<1>(iterate_func);

        t = t + .01;
    }
};

int main()
{
    ASSERT_SDL(!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS));

    SDL_Window* window = SDL_CreateWindow(
        "Heat Dissipation Demo", 0, 0, 800, 800, SDL_WINDOW_SHOWN);
    ASSERT_SDL(window != nullptr);

    SDL_Renderer* renderer =
        SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    ASSERT_SDL(renderer != nullptr);

    ASSERT_SDL(!SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE));
    ASSERT_SDL(!SDL_RenderClear(renderer));
    SDL_RenderPresent(renderer);

    simulation sim;

    SDL_Event event;
    bool run = true;
    while (run) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    run = false;
                    break;
                default:
                    break;
            }
        }
        sim.run_one_iteration(renderer);
        SDL_RenderPresent(renderer);
    }
}
