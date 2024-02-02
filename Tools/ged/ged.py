import os
import sys
from enum import Enum
from random import randrange

import pygame
from pygame import Rect


DISPLAY_SIZE = DISPLAY_WIDTH, DISPLAY_HEIGHT = 800, 480

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


class GElement(Rect):
    def test(self, pt):
        for e in Element:
            if self.element_rect(e).collidepoint(pt):
                return e
        return None

    def element_pos(self, elem: Element):
        if elem == Element.CORNER_NW or elem == Element.BODY:
            return self.x, self.y
        if elem == Element.CORNER_NE:
            return self.x + self.w, self.y
        if elem == Element.CORNER_SE:
            return self.x + self.w, self.y + self.h
        if elem == Element.CORNER_SW:
            return self.x, self.y + self.h
        return None


    def element_rect(self, elem: Element):
        HR = 4
        if elem == Element.BODY:
            return Rect(self.x - HR, self.y - HR, self.w + 2*HR, self.h + 2*HR)
        pt = self.element_pos(elem)
        if pt:
            return Rect(pt[0] - HR, pt[1] - HR, HR*2, HR*2)
        return Rect()


class GRect(GElement):
    def draw(self, surface):
        pygame.draw.rect(surface, self.color, self, 1)


class GEllipse(GElement):
    def draw(self, surface):
        pygame.draw.ellipse(surface, self.color, self, 1)


def run():
    pygame.init()
    screen = pygame.display.set_mode(DISPLAY_SIZE)
    clock = pygame.time.Clock()

    capture_state = CaptureState.IDLE
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
        r.color = randrange(0xffffff)
        display_list.append(r)
    # rect1 = Rect(288, 208, 100, 100)
    # rect2 = Rect(50, 50, 100, 80)
    # display_list = [rect1, rect2]
    sel_item = None
    sel_elem = None
    mousePos = None
    clickOffset = None

    def hit_test():
        for item in display_list:
            if item.element_rect(Element.BODY).collidepoint(mousePos):
                return item
        return None

    def render_display():
        screen.fill(0)
        for item in display_list:
            item.draw(screen)
            if sel_item and item == sel_item:
                r = item.inflate(4, 4)
                if capture_state == CaptureState.IDLE:
                    pygame.draw.rect(screen, 0xa0a0a0, r, 1)
                    pygame.draw.rect(screen, 0xa0a0a0, item.element_rect(Element.CORNER_NW), 1)
                    pygame.draw.rect(screen, 0xa0a0a0, item.element_rect(Element.CORNER_NE), 1)
                    pygame.draw.rect(screen, 0xa0a0a0, item.element_rect(Element.CORNER_SE), 1)
                    pygame.draw.rect(screen, 0xa0a0a0, item.element_rect(Element.CORNER_SW), 1)
                else:
                    pygame.draw.rect(screen, 0xffffff, r, 1)
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
                if capture_state != CaptureState.IDLE and event.button == pygame.BUTTON_LEFT:
                    capture_state = CaptureState.IDLE
                    setCursor(None)

            elif event.type == pygame.MOUSEBUTTONDOWN:
                if event.button == pygame.BUTTON_LEFT and capture_state == CaptureState.IDLE:
                    sel_item = hit_test()
                    if sel_item:
                        clickOffset = mousePos[0] - sel_item.x, mousePos[1] - sel_item.y
                        sel_elem = sel_item.test(mousePos)
                        capture_state = CaptureState.DRAGGING
                        setCursor(pygame.SYSTEM_CURSOR_SIZEALL)
                    else:
                        sel_elem = None

            elif event.type == pygame.MOUSEMOTION:
                mousePos = event.pos
                if capture_state == CaptureState.DRAGGING:
                    if sel_elem == Element.BODY:
                        sel_item.x = mousePos[0] - clickOffset[0]
                        sel_item.y = mousePos[1] - clickOffset[1]
                elif capture_state == CaptureState.IDLE:
                    cur = None
                    item = hit_test()
                    if sel_item and item and sel_item == item:
                        elem = item.test(mousePos)
                        if elem in [Element.CORNER_NW, Element.CORNER_SE]:
                            cur = pygame.SYSTEM_CURSOR_SIZENWSE
                        elif elem in [Element.CORNER_NE, Element.CORNER_SW]:
                            cur = pygame.SYSTEM_CURSOR_SIZENESW
                        else:
                            cur = pygame.SYSTEM_CURSOR_HAND
                    setCursor(cur)


        render_display()
        clock.tick(30)

    pygame.quit()
    return 0


if __name__ == "__main__":
    sys.exit(run())

