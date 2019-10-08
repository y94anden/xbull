from pathlib import Path

DEFAULT_PORT = '/dev/ttyUSB0'

defaults = Path('./defaults')
if defaults.is_file():
    DEFAULT_PORT = defaults.read_text().strip()
