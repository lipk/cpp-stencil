#include <SDL.h>
#include <buffer.hpp>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

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
    std::vector<buffer<2, double>*> bufs;
    double t;

    simulation()
        : t(0)
    {
        for (size_t i = 0; i < 4; ++i) {
            std::array<u64, 2> size = { 400, 400 };
            bufs.push_back(new buffer<2, double>(size, 1));
        }
    }

    void run_one_iteration(SDL_Renderer* renderer)
    {
        bufs[0]->copy_halo_from(*bufs[1], { 1, 0 });
        bufs[0]->copy_halo_from(*bufs[2], { 0, 1 });
        bufs[0]->copy_halo_from(*bufs[3], { 1, 1 });

        bufs[2]->copy_halo_from(*bufs[1], { 1, -1 });
        bufs[2]->copy_halo_from(*bufs[0], { 0, -1 });
        bufs[2]->copy_halo_from(*bufs[3], { 1, 0 });

        bufs[1]->copy_halo_from(*bufs[0], { -1, 0 });
        bufs[1]->copy_halo_from(*bufs[2], { -1, 1 });
        bufs[1]->copy_halo_from(*bufs[3], { 0, 1 });

        bufs[3]->copy_halo_from(*bufs[1], { 0, -1 });
        bufs[3]->copy_halo_from(*bufs[2], { -1, 0 });
        bufs[3]->copy_halo_from(*bufs[0], { -1, -1 });

        double source_x = 400 + cos(t) * 300;
        double source_y = 400 + sin(t) * 300;

#pragma omp parallel for
        for (size_t i = 0; i < 4; ++i) {
            u64 xoff = 400 * (i & 1), yoff = 400 * ((i & 2) >> 1);
            bufs[i]->iterate<1>([&](const std::array<u64, 2>& it,
                                    buffer<2, double>::accessor<1>& acc) {
                if (distsq(source_x, source_y, it[0] + xoff, it[1] + yoff) <
                    25) {
                    acc.get({ 0, 0 }) = 1.0;
                } else {
                    acc.get({ 0, 0 }) =
                        acc.get({ 0, 0 }) +
                        0.1 * ((acc.get({ -1, 0 }) - 2 * acc.get({ 0, 0 }) +
                                acc.get({ 1, 0 })) +
                               (acc.get({ 0, -1 }) - 2 * acc.get({ 0, 0 }) +
                                acc.get({ 0, 1 })));
                }
            });
        }

        for (size_t i = 0; i < 4; ++i) {
            u64 xoff = 400 * (i & 1), yoff = 400 * ((i & 2) >> 1);
            bufs[i]->iterate<1>([&](const std::array<u64, 2>& it,
                                    buffer<2, double>::accessor<1>& acc) {
                int color = acc.get({ 0, 0 }) * 255;
                SDL_SetRenderDrawColor(
                    renderer, color, color, color, SDL_ALPHA_OPAQUE);
                SDL_RenderDrawPoint(renderer, it[0] + xoff, it[1] + yoff);
            });
        }

        t = t + .02;
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
