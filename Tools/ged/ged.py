import os
import sys
import copy
from enum import IntEnum
from random import randrange
import pygame
from pygame import Rect
import pygame._sdl2
import tkinter as tk


DISPLAY_SIZE = DISPLAY_WIDTH, DISPLAY_HEIGHT = 800, 480
MIN_ELEMENT_WIDTH = MIN_ELEMENT_HEIGHT = 1

# Top 4 bits identify hit class
HIT_CLASS_MASK = 0xf0
# Lower 4 bits for direction
HIT_DIR_MASK = 0x0f
# Hit mask for handles at corners and midpoints of bounding rect
HIT_HANDLE = 0x10
# Hit mask for edges of bounding rectangle
HIT_EDGE = 0x20
HIT_BODY = 0x30

# Directions
DIR_N = 0x01
DIR_E = 0x02
DIR_S = 0x04
DIR_W = 0x08
DIR_NE = DIR_N | DIR_E
DIR_SE = DIR_S | DIR_E
DIR_SW = DIR_S | DIR_W
DIR_NW = DIR_N | DIR_W

def hit_class(e):
    return e & HIT_CLASS_MASK

def hit_dir(e):
    return e & HIT_DIR_MASK

class Element(IntEnum):
    HANDLE_N = HIT_HANDLE | DIR_N
    HANDLE_NE = HIT_HANDLE | DIR_NE
    HANDLE_E = HIT_HANDLE | DIR_E
    HANDLE_SE = HIT_HANDLE | DIR_SE
    HANDLE_S = HIT_HANDLE | DIR_S
    HANDLE_SW = HIT_HANDLE | DIR_SW
    HANDLE_W = HIT_HANDLE | DIR_W
    HANDLE_NW = HIT_HANDLE | DIR_NW

    EDGE_N = HIT_EDGE | DIR_N
    EDGE_NE = HIT_EDGE | DIR_NE
    EDGE_E = HIT_EDGE | DIR_E
    EDGE_SE = HIT_EDGE | DIR_SE
    EDGE_S = HIT_EDGE | DIR_S
    EDGE_SW = HIT_EDGE | DIR_SW
    EDGE_W = HIT_EDGE | DIR_W
    EDGE_NW = HIT_EDGE | DIR_NW

    BODY = HIT_BODY


class GElement(Rect):
    def __init__(self, x=0, y=0, w=0, h=0):
        super().__init__(x, y, w, h)
        self.init()

    def init(self):
        self.line_width = 1
        self.color = 0xa0a0a0

    def test(self, pt):
        for e in Element:
            r = self.element_rect(e)
            if r and r.collidepoint(pt):
                return e
        return None

    def get_cursor(self, pt):
        elem = self.test(pt)
        if elem is None:
            return None
        if elem in [Element.HANDLE_NW, Element.HANDLE_SE]:
            return pygame.SYSTEM_CURSOR_SIZENWSE
        if elem in [Element.HANDLE_NE, Element.HANDLE_SW]:
            return pygame.SYSTEM_CURSOR_SIZENESW
        if elem in [Element.HANDLE_N, Element.HANDLE_S]:
            return pygame.SYSTEM_CURSOR_SIZENS
        if elem in [Element.HANDLE_E, Element.HANDLE_W]:
            return pygame.SYSTEM_CURSOR_SIZEWE
        return pygame.SYSTEM_CURSOR_HAND

    def draw_select(self, surface, captured):
        r = self.element_rect(Element.BODY)
        if captured:
            pygame.draw.rect(surface, 0xffffff, r, 1)
        else:
            pygame.draw.rect(surface, 0xa0a0a0, r, 1)
            for e in Element:
                if hit_class(e) == HIT_HANDLE:
                    pygame.draw.rect(surface, 0xa0a0a0, self.element_rect(e), 1)


    def element_rect(self, elem):
        HR = 8, 8
        ER = 2, 2
        return {
            Element.HANDLE_N: Rect(self.x + self.w // 2, self.y, 1, 1).inflate(HR),
            Element.HANDLE_NE: Rect(self.x + self.w, self.y, 1, 1).inflate(HR),
            Element.HANDLE_E: Rect(self.x + self.w, self.y + self.h // 2, 1, 1).inflate(HR),
            Element.HANDLE_SE: Rect(self.x + self.w, self.y + self.h, 1, 1).inflate(HR),
            Element.HANDLE_S: Rect(self.x + self.w // 2, self.y + self.h, 1, 1).inflate(HR),
            Element.HANDLE_SW: Rect(self.x, self.y + self.h, 1, 1).inflate(HR),
            Element.HANDLE_W: Rect(self.x, self.y + self.h // 2, 1, 1).inflate(HR),
            Element.HANDLE_NW: Rect(self.x, self.y, 1, 1).inflate(HR),
            Element.EDGE_N: Rect(self.x, self.y, self.w, self.line_width).inflate(ER),
            Element.EDGE_E: Rect(self.x + self.w - self.line_width, self.y, self.line_width, self.h).inflate(ER),
            Element.EDGE_S: Rect(self.x, self.y + self.h - self.line_width, self.w, self.line_width).inflate(ER),
            Element.EDGE_W: Rect(self.x, self.y, self.line_width, self.h).inflate(ER),
            Element.BODY: Rect(self.x, self.y, self.w, self.h).inflate(ER),
        }.get(elem)


    def adjust(self, elem, orig, off):
        if elem == Element.BODY:
            self.x, self.y = orig.x + off[0], orig.y + off[1]
            return
        x, y, w, h = self.x, self.y, self.w, self.h
        if elem & DIR_N:
            y, h = orig.y + off[1], orig.h - off[1]
        if elem & DIR_E:
            w = orig.w + off[0]
        if elem & DIR_S:
            h = orig.h + off[1]
        if elem & DIR_W:
            x, w = orig.x + off[0], orig.w - off[0]
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


def setCursor(sys_cur):
    pygame.mouse.set_cursor(sys_cur or pygame.cursors.arrow)


def get_random_shapes(display_list, count):
    for i in range(count):
        w_min, w_max = 10, 200
        h_min, h_max = 10, 100
        w = randrange(w_min, w_max)
        h = randrange(h_min, h_max)
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


def run():
    root = tk.Tk()
    def btn_click():
        print('Button clicked')
    btn = tk.Button(root, text='Hello', command=btn_click)
    btn.pack(side=tk.TOP)
    embed_pygame = tk.Frame(root, width=DISPLAY_WIDTH, height=DISPLAY_HEIGHT)
    embed_pygame.pack(side=tk.TOP)
    root.update()

    # os.environ['SDL_WINDOWID'] = str(embed_pygame.winfo_id())
    pygame.display.init()
    screen = pygame.display.set_mode(DISPLAY_SIZE, pygame.SCALED | pygame.RESIZABLE)
    pygame.display.set_caption('Graphical Display Editor')
    print(pygame.display.Info())
    clock = pygame.time.Clock()

    mouse_captured = False
    display_list = []
    get_random_shapes(display_list, 20)

    sel_item = None
    sel_elem = None
    cap_item = None
    mouse_pos = None
    sel_pos = None

    def hit_test():
        # Items lower in Z-order can be selected through upper objects by clicking edges
        body_hit = None
        for item in reversed(display_list):
            e = item.test(mouse_pos)
            if not e:
                continue
            if e != Element.BODY:
                return item
            if not body_hit:
                body_hit = item
        return body_hit


    def render_display():
        screen.fill(0)
        # Draw everything, in order, except selected item
        for item in display_list:
            item.draw(screen)

        # draw selected item on top
        if sel_item:
            sel_item.draw_select(screen, mouse_captured)
        pygame.display.flip()


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
                    if not (sel_item and sel_item.test(mouse_pos)):
                        sel_item = hit_test()
                    if sel_item:
                        cap_item = copy.copy(sel_item)
                        sel_elem = sel_item.test(mouse_pos)
                        sel_pos = mouse_pos
                        mouse_captured = True
                    else:
                        sel_elem = cap_item = None

            elif event.type == pygame.MOUSEMOTION:
                mouse_pos = event.pos
                if mouse_captured:
                    sel_item.adjust(sel_elem, cap_item, (mouse_pos[0] - sel_pos[0], mouse_pos[1] - sel_pos[1]))
                elif sel_item:
                    setCursor(sel_item.get_cursor(mouse_pos))


        render_display()
        root.update_idletasks()
        root.update()
        clock.tick(30)

    pygame.quit()
    return 0


if __name__ == "__main__":
    sys.exit(run())

