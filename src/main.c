#include "SDL3/SDL_gpu.h"
#include <SDL3/SDL.h>

int main(void)
{
    SDL_Window *window = SDL_CreateWindow("Test", 800, 600, SDL_WINDOW_MAXIMIZED);
    SDL_GPUDevice *device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, 0, 0);
    SDL_ClaimWindowForGPUDevice(device, window);

    for(;;)
    {
        SDL_Event e;
        while(SDL_PollEvent(&e))
        {
            switch(e.type)
            {
                case SDL_EVENT_QUIT:
                {
                    exit(0);
                } break;
            }
        }
    }
}
