from abc import ABC, abstractmethod


class Base(ABC):
    @abstractmethod
    def save_configuration(self) -> None:
        pass

    @abstractmethod
    def get_configuration(self) -> dict:
        pass
