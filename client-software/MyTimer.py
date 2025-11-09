from PySide6.QtCore import QObject, QTimer


class MyTimer(QObject):
    """Object ot create a timer for the GUI.
       Calls a callback when reaching 0."""

    def __init__(self, interval_seconds, callback, parent=None):
        super().__init__(parent)
        self._callback = callback
        self._timer = QTimer(self)
        self._timer.setInterval(int(interval_seconds * 1000))  # milliseconds
        self._timer.timeout.connect(self._callback)

    def start(self):
        """Enable the timer"""
        if not self._timer.isActive():
            self._timer.start()

    def stop(self):
        """Disable the timer"""
        if self._timer.isActive():
            self._timer.stop()

    def is_running(self):
        """Returns the status of the timer """
        return self._timer.isActive()
