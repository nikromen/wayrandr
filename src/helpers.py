from src.constants import GRID_SCALING


def apply_scaling(value: float, scale: float = 1) -> int:
    return int(value // GRID_SCALING * scale)


def deapply_scaling(value: int, scale: float = 1) -> float:
    return value * GRID_SCALING / scale
