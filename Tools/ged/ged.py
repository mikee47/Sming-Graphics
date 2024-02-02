"""Simple example for using sdl2 directly."""
import os
import sys
import ctypes
from sdl2 import *
import sdl2.ext
from sdl2.ext import Color
from enum import Enum
from random import randrange

DISPLAY_WIDTH = 800
DISPLAY_HEIGHT = 480

class CaptureState(Enum):
    IDLE = 0
    DRAGGING = 1
    SIZING = 2

class Element(Enum):
    CORNER_NW = 1
    CORNER_NE = 2
    CORNER_SE = 3
    CORNER_SW = 4
    BODY = 5


class Rect(SDL_Rect):
    def test(self, pt):
        for e in Element:
            if SDL_PointInRect(pt, self.element_rect(e)):
                return e
        return None

    def element_pos(self, elem: Element):
        if elem == Element.CORNER_NW or elem == Element.BODY:
            return SDL_Rect(self.x, self.y)
        if elem == Element.CORNER_NE:
            return SDL_Rect(self.x + self.w, self.y)
        if elem == Element.CORNER_SE:
            return SDL_Rect(self.x + self.w, self.y + self.h)
        if elem == Element.CORNER_SW:
            return SDL_Rect(self.x, self.y + self.h)
        return None


    def element_rect(self, elem: Element):
        HR = 4
        if elem == Element.BODY:
            return SDL_Rect(self.x - HR, self.y - HR, self.w + 2*HR, self.h + 2*HR)
        pt = self.element_pos(elem)
        return SDL_Rect(pt.x - HR, pt.y - HR, HR*2, HR*2) if pt else None

        # if elem == Element.CORNER_NW:
        #     return SDL_Rect(self.x - HR, self.y - HR, 2*HR, 2*HR)
        # if elem == Element.CORNER_NE:
        #     return SDL_Rect(self.x + self.w - HR, self.y - HR, 2*HR, 2*HR)
        # if elem == Element.CORNER_SE:
        #     return SDL_Rect(self.x + self.w - HR, self.y + self.h - HR, 2*HR, 2*HR)
        # if elem == Element.CORNER_SW:
        #     return SDL_Rect(self.x - HR, self.y + self.h - HR, 2*HR, 2*HR)
        # if elem == Element.BODY:
        #     return SDL_Rect(self.x - HR, self.y - HR, self.w + 2*HR, self.h + 2*HR)
        # return None


def inflate(rect, dx, dy = None):
    if dy is None:
        dy = dx
    return Rect(rect.x - dx, rect.y - dy, rect.w + 2 * dx, rect.h + 2 * dy)

def run():
    sdl2.ext.init(SDL_INIT_VIDEO)
    window = sdl2.ext.Window(b"SDL2 Drag and Drop", size=(DISPLAY_WIDTH, DISPLAY_HEIGHT))
    window.show()

    renderer = sdl2.ext.Renderer(window, flags=SDL_RENDERER_ACCELERATED)

    capture_state = CaptureState.IDLE
    display_list = []
    for i in range(1, 10):
        w_max = 200
        h_max = 100
        w = randrange(w_max)
        h = randrange(h_max)
        x = randrange(DISPLAY_WIDTH - w)
        y = randrange(DISPLAY_HEIGHT - h)
        r = Rect(x, y, w, h)
        r.color = randrange(0xffffff)
        display_list.append(r)
    # rect1 = Rect(288, 208, 100, 100)
    # rect2 = Rect(50, 50, 100, 80)
    # display_list = [rect1, rect2]
    sel_item = None
    sel_elem = None
    mousePos = SDL_Point()
    clickOffset = SDL_Point()

    def hit_test():
        for item in display_list:
            if SDL_PointInRect(mousePos, item.element_rect(Element.BODY)):
                return item
        return None

    def render_display():
        renderer.clear(0)
        for item in display_list:
            renderer.draw_rect(item, item.color)
            if sel_item and item == sel_item:
                r = inflate(item, 2)
                if capture_state == CaptureState.IDLE:
                    renderer.draw_rect(r, 0xa0a0a0)
                    renderer.draw_rect(item.element_rect(Element.CORNER_NW), 0xa0a0a0)
                    renderer.draw_rect(item.element_rect(Element.CORNER_NE), 0xa0a0a0)
                    renderer.draw_rect(item.element_rect(Element.CORNER_SE), 0xa0a0a0)
                    renderer.draw_rect(item.element_rect(Element.CORNER_SW), 0xa0a0a0)
                else:
                    renderer.draw_rect(r, 0xffffff)
        renderer.present()


    def setCursor(sys_cur):
        if sys_cur is None:
            SDL_SetCursor(SDL_GetDefaultCursor())
        else:
            SDL_SetCursor(SDL_CreateSystemCursor(sys_cur))


    running = True
    event = SDL_Event()
    while running:
        events = sdl2.ext.get_events()
        for event in events:
            if event.type == SDL_QUIT:
                running = False
                break

            if event.type == SDL_MOUSEBUTTONUP:
                if capture_state != CaptureState.IDLE and event.button.button == SDL_BUTTON_LEFT:
                    capture_state = CaptureState.IDLE
                    setCursor(None)

            elif event.type == SDL_MOUSEBUTTONDOWN:
                if event.button.button == SDL_BUTTON_LEFT and capture_state == CaptureState.IDLE:
                    sel_item = hit_test()
                    if sel_item:
                        clickOffset.x = mousePos.x - sel_item.x
                        clickOffset.y = mousePos.y - sel_item.y
                        sel_elem = sel_item.test(mousePos)
                        capture_state = CaptureState.DRAGGING
                        setCursor(SDL_SYSTEM_CURSOR_SIZEALL)
                    else:
                        sel_elem = None

            elif event.type == SDL_MOUSEMOTION:
                mousePos = SDL_Point(event.motion.x, event.motion.y)
                if capture_state == CaptureState.DRAGGING:
                    if sel_elem == Element.BODY:
                        sel_item.x = mousePos.x - clickOffset.x
                        sel_item.y = mousePos.y - clickOffset.y
                elif capture_state == CaptureState.IDLE:
                    cur = None
                    item = hit_test()
                    if sel_item and item and sel_item == item:
                        elem = item.test(mousePos)
                        if elem in [Element.CORNER_NW, Element.CORNER_SE]:
                            cur = SDL_SYSTEM_CURSOR_SIZENWSE
                        elif elem in [Element.CORNER_NE, Element.CORNER_SW]:
                            cur = SDL_SYSTEM_CURSOR_SIZENESW
                        else:
                            cur = SDL_SYSTEM_CURSOR_HAND
                    setCursor(cur)


        render_display()
        SDL_Delay(10)

    sdl2.ext.quit()
    return 0


if __name__ == "__main__":
    sys.exit(run())

