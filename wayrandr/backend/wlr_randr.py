"""
this executes wlr-randr command according to the monitor dataclass
"""

from subprocess import run

from wayrandr.backend.base import RandrBackend
from wayrandr.monitor import Monitor


class WlrRandrBackend(RandrBackend):
    def save_configuration(self, monitors: list[Monitor]) -> None:
        for monitor in monitors:
            command = ["wlr-randr"]
            command.append(f"--output {monitor.name}")

            if monitor.enabled:
                command.append("--on")
            else:
                command.append("--off")

            mode = monitor.active_mode
            command.append(f"--mode {mode.width}x{mode.height}@{mode.refresh}")
            command.append("--prefered")
            command.append(f"--pos {monitor.position.x},{monitor.position.y}")
            command.append(f"--transform {monitor.transform}")
            command.append(f"--scale {monitor.scale}")

            run(command)
