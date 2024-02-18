import re
import tkinter as tk
from tkinter import ttk
from tkinter.scrolledtext import ScrolledText

class LabelWidget(ttk.Label):
    def __init__(self, master, text):
        super().__init__(master, text=text)

    def set_row(self, row: int):
        self.grid(row=row, column=0, sticky=tk.E, padx=8)
        return self


class CustomWidget:
    @staticmethod
    def text_from_name(name: str) -> str:
        words = re.findall(r'[A-Z]?[a-z]+|[A-Z]+(?=[A-Z]|$)', name)
        return ' '.join(words).lower()

    def set_row(self, row: int):
        self.grid(row=row, column=1, sticky=tk.EW, padx=4, pady=2)
        return self

    @staticmethod
    def _addvar(name, type, callback):
        if type is str:
            var = tk.StringVar()
        elif type is int:
            var = tk.IntVar()
        elif type is float:
            var = tk.DoubleVar()
        elif type is bool:
            var = tk.BooleanVar
        else:
            raise TypeError(f'Invalid variable type {type}')
        def tk_value_changed(name1, name2, op, name=name, var=var):
            try:
                value = var.get()
                # print(f'value_changed:"{name1}", "{name2}", "{op}", "{value}"')
                callback(name, value)
            except tk.TclError: # Can happen whilst typing if conversion fails, empty strings, etc.
                pass
        var.trace_add('write', tk_value_changed)
        return var

    def set_choices(self, choices):
        pass


class FrameWidget(ttk.Frame, CustomWidget):
    pass


class EntryWidget(ttk.Entry, CustomWidget):
    def __init__(self, master, name, vartype, callback):
        self.var = self._addvar(name, vartype, callback)
        super().__init__(master, textvariable=self.var)

    def get_value(self):
        return self.var.get()

    def set_value(self, value):
        self.var.set(value)


class TextWidget(ScrolledText, CustomWidget):
    def __init__(self, master, name, callback):
        super().__init__(master, height=4)
        self.callback = callback
        def on_modified(evt, name=name):
            if self.callback:
                self.edit_modified(False)
                self.callback(name, self.get_value())
        self.bind('<<Modified>>', on_modified)

    def get_value(self):
        # TK always appends a newline, so remove it
        return self.get('1.0', 'end')[:-1]

    def set_value(self, value):
        self.replace('1.0', 'end', value)


class ComboboxWidget(ttk.Combobox, CustomWidget):
    def __init__(self, master, name, vartype, values: list, callback):
        self.var = self._addvar(name, vartype, callback)
        super().__init__(master, textvariable=self.var, values=values)

    def get_value(self):
        return self.var.get()

    def set_value(self, value):
        self.var.set(value)

    def set_choices(self, choices):
        self.configure(values=choices)


class ScaleWidget(tk.Scale, CustomWidget):
    def __init__(self, master, name, vartype, callback, from_, to, resolution):
        self.callback = callback
        # Don't get variable trace callbacks for scale widgets (don't know why) so use command
        def cmd_changed(value, name=name, vartype=vartype):
            if self.callback:
                self.callback(name, vartype(value))
        super().__init__(master, orient=tk.HORIZONTAL, from_=from_, to=to,
            resolution=resolution, command=cmd_changed)

    def get_value(self):
        return self.get()

    def set_value(self, value):
        self.set(value)


class CheckFieldsWidget(ttk.Frame, CustomWidget):
    def __init__(self, master, name, values, callback):
        super().__init__(master)
        self.callback = callback
        self.var = set()
        self.ctrls = {}
        for value in values:
            def check_invoked(name=name, value=value):
                ctrl = self.ctrls[value]
                state = ctrl.state()
                if 'selected' in state:
                    self.var.add(value)
                else:
                    self.var.discard(value)
                if self.callback:
                    self.callback(name, self.var)
                self.update_checks()
            ctrl = ttk.Checkbutton(self, text=self.text_from_name(value), command=check_invoked)
            ctrl.state(['!alternate'])
            ctrl.pack(side=tk.LEFT)
            self.ctrls[value] = ctrl

    def update_checks(self):
        for f, c in self.ctrls.items():
            c.state(['selected' if f in self.var else '!selected'])

    def get_value(self):
        return self.var

    def set_value(self, value):
        self.var = set(value)
        self.update_checks()


class GroupedCheckFieldsWidget(ttk.LabelFrame, CustomWidget):
    def __init__(self, master, name, groups: dict, callback):
        """Split set up into distinct parts"""
        super().__init__(master, text=self.text_from_name(name))
        self.grid(column=0, columnspan=2)
        self.callback = callback
        self.var = set()
        self.ctrls = {}
        row = 0 # Within our new frame
        for group, values in groups.items():
            def split(value):
                text, _, value = value.partition(':')
                return (text, value) if value else (text, text)
            group, _, kind = group.partition(':')
            if kind == 'oneof':
                value_set = {split(x)[1] for x in values}
            else:
                value_set = ()
                if kind:
                    raise ValueError(f'Bad field group kind "{kind}"')
            LabelWidget(self, self.text_from_name(group)).set_row(row)
            group_frame = FrameWidget(self).set_row(row)
            row += 1
            for value in values:
                text, value = split(value)
                def check_invoked(name=name, value=value, value_set=value_set):
                    ctrl = self.ctrls[value]
                    state = ctrl.state()
                    for v in value_set:
                        if v != value:
                            self.var.discard(v)
                    if 'selected' in state:
                        self.var.add(value)
                    else:
                        self.var.discard(value)
                    if self.callback:
                        self.callback(name, self.var)
                    self.update_checks()
                ctrl = ttk.Checkbutton(group_frame, text=text, command=check_invoked)
                ctrl.state(['!alternate'])
                ctrl.pack(side=tk.LEFT)
                self.ctrls[value] = ctrl

    def update_checks(self):
        for f, c in self.ctrls.items():
            c.state(['selected' if f in self.var else '!selected'])

    def get_value(self):
        return self.var

    def set_value(self, value):
        self.var = set(value)
        self.update_checks()


