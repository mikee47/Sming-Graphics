"""Simple example for using sdl2 directly."""
import os
import sys
import ctypes
from sdl2 import *
from enum import Enum

class CaptureState(Enum):
    IDLE = 0
    DRAGGING = 1
    SIZING = 2


class Rect(SDL_Rect):
    pass


def run():
    SDL_Init(SDL_INIT_VIDEO)
    window = SDL_CreateWindow(b"SDL2 Drag and Drop",
                                   SDL_WINDOWPOS_CENTERED,
                                   SDL_WINDOWPOS_CENTERED,
                                   800, 480, SDL_WINDOW_SHOWN)
    renderer = SDL_CreateRenderer(window, -1, 0)

    capture_state = CaptureState.IDLE
    rect1 = Rect(288, 208, 100, 100)
    rect2 = Rect(50, 50, 100, 80)
    display_list = [rect1, rect2]
    selected = None
    mousePos = SDL_Point()
    clickOffset = SDL_Point()

    def hit_test():
        for elem in display_list:
            if SDL_PointInRect(mousePos, elem):
                return elem
        return None

    def render_display():
            SDL_SetRenderDrawColor(renderer, 242, 242, 242, 255)
            SDL_RenderClear(renderer)
            for elem in display_list:
                if selected and elem == selected:
                    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255)
                else:
                    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255)
                SDL_RenderFillRect(renderer, elem)
            SDL_RenderPresent(renderer);

    running = True
    event = SDL_Event()
    while running:
        while SDL_PollEvent(ctypes.byref(event)) != 0:
            if event.type == SDL_QUIT:
                running = False
                break

            if event.type == SDL_MOUSEBUTTONUP:
                if capture_state != CaptureState.IDLE and event.button.button == SDL_BUTTON_LEFT:
                    capture_state = CaptureState.IDLE
                    selected = None

            elif event.type == SDL_MOUSEBUTTONDOWN:
                if event.button.button == SDL_BUTTON_LEFT and capture_state == CaptureState.IDLE:
                    elem = hit_test()
                    if elem:
                        clickOffset.x = mousePos.x - elem.x
                        clickOffset.y = mousePos.y - elem.y
                        capture_state = CaptureState.DRAGGING
                        selected = elem

            elif event.type == SDL_MOUSEMOTION:
                mousePos = SDL_Point(event.motion.x, event.motion.y)
                if capture_state == CaptureState.DRAGGING:
                    selected.x = mousePos.x - clickOffset.x
                    selected.y = mousePos.y - clickOffset.y

        render_display()

        SDL_Delay(10)

    SDL_DestroyWindow(window)
    SDL_Quit()
    return 0


if __name__ == "__main__":
    sys.exit(run())

