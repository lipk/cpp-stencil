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
    std::vector<buffer_set<2, double, double>*> bufs;
    std::vector<grid_set<2, double, double>*> grids;
    double t;
    u32 itercount;

    simulation()
        : t(0)
        , itercount(0)
    {
        for (size_t i = 0; i < 4; ++i) {
            std::array<u64, 2> bufsize = { 402, 402 };
            std::array<u64, 2> gridsize = { 400, 400 };
            bufs.push_back(new buffer_set<2, double, double>(bufsize));
            grids.push_back(new grid_set<2, double, double>(
                gridsize, 1, { 1, 1 }, *bufs[i]));
            grids.back()->get<0>().fill(0.0);
        }
    }

    grid_set<2, double, double>::subset_t<double, double> get_buffers_ordered(
        size_t i)
    {
        if (itercount % 2 == 0) {
            return grids[i]->subset<0, 1>();
        } else {
            return grids[i]->subset<1, 0>();
        }
    }

    void run_one_iteration(SDL_Renderer* renderer)
    {
        std::vector<grid_set<2, double, double>::subset_t<double, double>> ord;
        for (size_t i = 0; i < 4; ++i) {
            ord.emplace_back(get_buffers_ordered(i));
        }
        for (size_t i = 0; i < 4; ++i) {
            for (size_t j = 0; j < 4; ++j) {
                if (i == j) {
                    continue;
                }
                i32 x = (j & 1) - (i & 1);
                i32 y = ((j & 2) - (i & 2)) >> 1;
                ord[i].get<0>().copy_halo_from(ord[j].get<0>(), { x, y });
            }
        }

        double source_x = 400 + cos(t) * 300;
        double source_y = 400 + sin(t) * 300;

        for (size_t i = 0; i < 4; ++i) {
            u64 xoff = 400 * (i & 1), yoff = 400 * ((i & 2) >> 1);
            ord[i].iterate<1>([&](const std::array<u64, 2>& it,
                                  accessor<1, 2, double, double>& acc) {
                if (distsq(source_x, source_y, it[0] + xoff, it[1] + yoff) <
                    25) {
                    acc.get<1>({ 0, 0 }) = 1.0;
                } else {
                    acc.get<1>({ 0, 0 }) =
                        acc.get<0>({ 0, 0 }) +
                        0.2 *
                            ((acc.get<0>({ -1, 0 }) - 2 * acc.get<0>({ 0, 0 }) +
                              acc.get<0>({ 1, 0 })) +
                             (acc.get<0>({ 0, -1 }) - 2 * acc.get<0>({ 0, 0 }) +
                              acc.get<0>({ 0, 1 })));
                }
            });
        }

        for (size_t i = 0; i < 4; ++i) {
            u64 xoff = 400 * (i & 1), yoff = 400 * ((i & 2) >> 1);
            ord[i].iterate<1>([&](const std::array<u64, 2>& it,
                                  accessor<1, 2, double, double>& acc) {
                int color = acc.get<1>({ 0, 0 }) * 255;
                SDL_SetRenderDrawColor(
                    renderer, color, color, color, SDL_ALPHA_OPAQUE);
                SDL_RenderDrawPoint(renderer, it[0] + xoff, it[1] + yoff);
            });
        }

        t = t + .02;
        itercount++;
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
