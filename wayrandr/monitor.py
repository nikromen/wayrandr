import json
from dataclasses import dataclass
from enum import StrEnum
from functools import cached_property
from subprocess import run
from typing import Optional

from wayrandr.helpers import apply_scaling


class Transform(StrEnum):
    normal = "normal"
    normal_90 = "90"
    normal_180 = "180"
    normal_270 = "270"
    flipped = "flipped"
    flipped_90 = "flipped-90"
    flipped_180 = "flipped-180"
    flipped_270 = "flipped-270"

    @staticmethod
    def _map() -> dict:
        return {
            # 0-3 UI buttons
            Transform.normal: 0,
            Transform.normal_90: 1,
            Transform.normal_180: 2,
            Transform.normal_270: 3,
            Transform.flipped: 4,
            Transform.flipped_90: 5,
            Transform.flipped_180: 6,
            Transform.flipped_270: 7,
        }

    @classmethod
    def reverse_map(cls) -> dict:
        return {v: k for k, v in cls._map().items()}

    def value_index(self) -> int:
        return self._map()[self]

    def flipped_value(self) -> "Transform":
        flip_constant = 4
        flip_num = self._map()[self] + flip_constant
        if flip_num > len(Transform) - 1:
            flip_num -= len(Transform)

        return Transform.reverse_map()[flip_num]

    @property
    def is_flipped(self) -> bool:
        return self._map()[self] >= 4

    @property
    def is_rotated(self) -> bool:
        return self in [
            self.normal_90,
            self.normal_270,
            self.flipped_90,
            self.flipped_270,
        ]

    @property
    def is_upside_down(self) -> bool:
        return self in [self.normal_180, self.flipped_180]


@dataclass
class Mode:
    width: int
    height: int
    refresh: int
    preferred: bool
    current: bool

    def __eq__(self, other) -> bool:
        return (
            self.width == other.width
            and self.height == other.height
            and self.refresh == other.refresh
        )

    def __str__(self) -> str:
        return f"{self.width}x{self.height}@{self.refresh}Hz"

    def scaled_resolution(self, scale: float = 1) -> tuple[int, int]:
        return apply_scaling(self.width, scale), apply_scaling(self.height, scale)


@dataclass
class Position:
    x: int
    y: int

    def scaled_position(self, scale: float = 1) -> tuple[int, int]:
        return apply_scaling(self.x, scale), apply_scaling(self.y, scale)


@dataclass
class Monitor:
    name: str
    make: str
    model: Optional[str]
    serial: Optional[str]
    enabled: bool
    scale: float
    position: Position
    modes: list[Mode]
    transform: Transform = Transform.normal
    # the rest is not important now

    @cached_property
    def description(self) -> str:
        model = self.model
        if self.model is None:
            model = "Unknown"

        serial = self.serial
        if self.serial is None:
            serial = "Unknown"

        return f"{self.make} {model} {serial}"

    @property
    def active_mode(self) -> Mode:
        for mode in self.modes:
            if mode.current:
                return mode

        return self.modes[0]

    @active_mode.setter
    def active_mode(self, value: Mode) -> None:
        prev_active = self.active_mode
        if prev_active == value:
            return

        prev_active.current = False
        for mode in self.modes:
            if mode == value:
                mode.current = True
                return

    @property
    def width(self) -> int:
        return self.active_mode.width

    @property
    def height(self) -> int:
        return self.active_mode.height

    @property
    def refresh(self) -> int:
        return self.active_mode.refresh


def _parse_modes(modes: list[dict]) -> list[Mode]:
    result = []
    for mode in modes:
        result.append(
            Mode(
                width=mode["width"],
                height=mode["height"],
                refresh=mode["refresh"],
                preferred=mode["preferred"],
                current=mode["current"],
            ),
        )

    return result


def get_monitors() -> list[Monitor]:
    response = run(["wlr-randr", "--json"], capture_output=True, text=True).stdout
    data = json.loads(response)
    result = []
    for monitor in data:
        mode_json = monitor.get("modes", [])
        modes = _parse_modes(mode_json)

        result.append(
            Monitor(
                name=monitor["name"],
                make=monitor["make"],
                model=monitor["model"],
                serial=monitor.get("serial"),
                enabled=monitor["enabled"],
                scale=monitor["scale"],
                position=Position(
                    x=monitor["position"]["x"],
                    y=monitor["position"]["y"],
                ),
                modes=modes,
                transform=Transform(monitor["transform"]),
            ),
        )

    return result
