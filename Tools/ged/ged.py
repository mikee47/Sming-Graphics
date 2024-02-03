import os
import sys
import copy
from enum import Enum
from random import randrange
import pygame
from pygame import Rect


DISPLAY_SIZE = DISPLAY_WIDTH, DISPLAY_HEIGHT = 800, 480
MIN_ELEMENT_WIDTH = MIN_ELEMENT_HEIGHT = 1

class Element(Enum):
    CORNER_NW = 1
    CORNER_N = 2
    CORNER_NE = 3
    CORNER_E = 4
    CORNER_SE = 5
    CORNER_S = 6
    CORNER_SW = 7
    CORNER_W = 8
    BODY = 9


class GElement(Rect):
    def __init__(self, x=0, y=0, w=0, h=0):
        super().__init__(x, y, w, h)
        self.init()

    def init(self):
        self.line_width = 1
        self.color = 0xa0a0a0

    def test(self, pt):
        for e in Element:
            if self.element_rect(e).collidepoint(pt):
                return e
        return None

    def get_cursor(self, pt):
        elem = self.test(pt)
        if elem is None:
            return None
        if elem in [Element.CORNER_NW, Element.CORNER_SE]:
            return pygame.SYSTEM_CURSOR_SIZENWSE
        if elem in [Element.CORNER_NE, Element.CORNER_SW]:
            return pygame.SYSTEM_CURSOR_SIZENESW
        if elem in [Element.CORNER_N, Element.CORNER_S]:
            return pygame.SYSTEM_CURSOR_SIZENS
        if elem in [Element.CORNER_E, Element.CORNER_W]:
            return pygame.SYSTEM_CURSOR_SIZEWE
        return pygame.SYSTEM_CURSOR_HAND

    def draw_select(self, surface, captured):
        r = self.inflate(4, 4)
        if captured:
            pygame.draw.rect(surface, 0xffffff, r, 1)
        else:
            pygame.draw.rect(surface, 0xa0a0a0, r, 1)
            for e in Element:
                if e != Element.BODY:
                    pygame.draw.rect(surface, 0xa0a0a0, self.element_rect(e), 1)


    def element_pos(self, elem: Element):
        return {
            Element.CORNER_NW: (self.x, self.y),
            Element.CORNER_N: (self.x + self.w // 2, self.y),
            Element.CORNER_NE: (self.x + self.w, self.y),
            Element.CORNER_E: (self.x + self.w, self.y + self.h // 2),
            Element.CORNER_SE: (self.x + self.w, self.y + self.h),
            Element.CORNER_S: (self.x + self.w // 2, self.y + self.h),
            Element.CORNER_SW: (self.x, self.y + self.h),
            Element.CORNER_W: (self.x, self.y + self.h // 2),
            Element.BODY: (self.x, self.y),
        }.get(elem)


    def element_rect(self, elem: Element):
        HR = 4
        if elem == Element.BODY:
            return Rect(self.x - HR, self.y - HR, self.w + 2*HR, self.h + 2*HR)
        pt = self.element_pos(elem)
        if pt:
            return Rect(pt[0] - HR, pt[1] - HR, HR*2, HR*2)
        return Rect()


    def adjust(self, elem, orig, off):
        if elem == Element.BODY:
            self.x, self.y = off
            return
        x, y, w, h = self.x, self.y, self.w, self.h
        if elem in [Element.CORNER_N, Element.CORNER_NW, Element.CORNER_NE]:
            y, h = off[1], orig.h + orig.y - off[1]
        if elem in [Element.CORNER_E, Element.CORNER_NE, Element.CORNER_SE]:
            w = off[0] - orig.x
        if elem in [Element.CORNER_S, Element.CORNER_SE, Element.CORNER_SW]:
            h = off[1] - orig.y
        if elem in [Element.CORNER_W, Element.CORNER_NW, Element.CORNER_SW]:
            x, w = off[0], orig.w + orig.x - off[0]
        if w >= MIN_ELEMENT_WIDTH + self.line_width*2 and h >= MIN_ELEMENT_HEIGHT + self.line_width*2:
            self.x, self.y, self.w, self.h = x, y, w, h


class GRect(GElement):
    def init(self):
        super().init()
        self.radius = 0

    def draw(self, surface):
        pygame.draw.rect(surface, self.color, self, self.line_width, self.radius)


class GEllipse(GElement):
    def draw(self, surface):
        pygame.draw.ellipse(surface, self.color, self, self.line_width)


def run():
    pygame.init()
    screen = pygame.display.set_mode(DISPLAY_SIZE)
    clock = pygame.time.Clock()

    mouse_captured = False
    display_list = []
    for i in range(1, 10):
        w_max = 200
        h_max = 100
        w = randrange(w_max)
        h = randrange(h_max)
        x = randrange(DISPLAY_WIDTH - w)
        y = randrange(DISPLAY_HEIGHT - h)
        kind = randrange(5)
        if kind == 1:
            r = GEllipse(x, y, w, h)
        else:
            r = GRect(x, y, w, h)
            r.radius = randrange(0, 20)
        r.color = randrange(0xffffff)
        r.line_width = randrange(5)
        display_list.append(r)

    sel_item = None
    sel_elem = None
    cap_item = None
    mousePos = None
    clickOffset = None

    def hit_test():
        for item in reversed(display_list):
            if item.element_rect(Element.BODY).collidepoint(mousePos):
                return item
        return None

    def render_display():
        screen.fill(0)
        # Draw everything, in order, except selected item
        for item in display_list:
            item.draw(screen)

        # draw selected item on top
        if sel_item:
            sel_item.draw_select(screen, mouse_captured)
        pygame.display.flip()


    def setCursor(sys_cur):
        pygame.mouse.set_cursor(sys_cur or pygame.cursors.arrow)

    running = True
    while running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
                break

            if event.type == pygame.MOUSEBUTTONUP:
                if mouse_captured and event.button == pygame.BUTTON_LEFT:
                    mouse_captured = False
                    cap_item = None
                    setCursor(None)

            elif event.type == pygame.MOUSEBUTTONDOWN:
                if event.button == pygame.BUTTON_LEFT and not mouse_captured:
                    if not (sel_item and sel_item.test(mousePos)):
                        sel_item = hit_test()
                    if sel_item:
                        cap_item = copy.copy(sel_item)
                        sel_elem = sel_item.test(mousePos)
                        pt = sel_item.element_pos(sel_elem)
                        clickOffset = mousePos[0] - pt[0], mousePos[1] - pt[1]
                        mouse_captured = True
                        setCursor(pygame.SYSTEM_CURSOR_SIZEALL)
                    else:
                        sel_elem = cap_item = None

            elif event.type == pygame.MOUSEMOTION:
                mousePos = event.pos
                if mouse_captured:
                    sel_item.adjust(sel_elem, cap_item, (mousePos[0] - clickOffset[0], mousePos[1] - clickOffset[1]))
                elif sel_item:
                    setCursor(sel_item.get_cursor(mousePos))


        render_display()
        clock.tick(30)

    pygame.quit()
    return 0


if __name__ == "__main__":
    sys.exit(run())

