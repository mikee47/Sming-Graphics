from dataclasses import dataclass

@dataclass
class Font:
    name: str = ''
    family: str = ''
    size: int = 12
    # style: list[str] For now, assume all styles are available


@dataclass
class Image:
    name: str = ''
    source: str = ''

