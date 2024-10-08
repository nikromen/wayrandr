from abc import ABC, abstractmethod

from wayrandr.monitor import Monitor


class RandrBackend(ABC):
    @abstractmethod
    def save_configuration(self, monitors: list[Monitor]) -> None:
        pass
