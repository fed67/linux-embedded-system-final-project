from PySide6.QtWidgets import QApplication, QWidget, QHBoxLayout, QVBoxLayout, QLabel, QPushButton, QLineEdit, QMessageBox, QGridLayout
from PySide6.QtGui import QFont, QIcon, QPalette
from PySide6.QtCore import Qt, Signal, Slot

from net.OnewireClient import OnewireClient
from collections.abc import Callable

class ControlWidget(QWidget):
    
    signal_new_temperature = Signal((float,))
    disconnect_color = Qt.yellow


    def __init__(self, log : Callable[[str], None] = None, parent=None):
        super().__init__(parent)
        self.init()
        self.client_network = None
        self.log = log
    
    def init(self):

        self.layout = QGridLayout()
        self.layout.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.setLayout(self.layout)

        self.button0 = QPushButton("Connect")
        self.button0.clicked.connect(self.button_connect_signal)

        self.button_close = QPushButton("Disconnect")
        self.button_close.clicked.connect(self.button_close_signal)

        self.label1 = QLabel("Address")
        current_font = self.label1.font()
        current_font.setBold(True)
        self.label1.setFont(current_font)

        self.line = QLineEdit()
        self.line.setText("0.0.0.0")
        self.layout.addWidget(self.button0, 0, 0)
        self.layout.addWidget(self.label1, 0, 1)
        self.layout.addWidget(self.line, 0, 2)
        self.layout.addWidget(self.button_close, 1, 0)

        self.label2 = QLabel("Port")
        current_font = self.label2.font()
        current_font.setBold(True)
        self.label2.setFont(current_font)

        self.line2 = QLineEdit()
        self.layout.addWidget(self.label2, 1, 1)
        self.layout.addWidget(self.line2, 1, 2)

        self.buttons = ActionWidget(parent=self)
        self.layout.addWidget(self.buttons, 2, 1)
        self.buttons.signal_read_id[int].connect(self.read_id)
        self.buttons.signal_read_temperature[int].connect(self.read_temperature)

        palette = self.palette()
        palette.setColor(self.backgroundRole(), self.disconnect_color)
        self.setAutoFillBackground(True)
        self.setPalette(palette)

    def button_close_signal(self):
        self.client_network = None

        palette = self.palette()
        palette.setColor(self.backgroundRole(), self.disconnect_color)
        self.setPalette(palette)


    def button_connect_signal(self):
        print("button clicked")
        self.client_network = OnewireClient(self.line.text(), self.line2.text(), self.log)

        palette = self.palette()
        palette.setColor(self.backgroundRole(), Qt.green)
        self.setPalette(palette)


    @Slot(int)
    def read_id(self):
        if self.client_network is not None:
            self.client_network.read_id()
        else:
            messageBox = QMessageBox()
            messageBox.critical(None, "Network Error", "Not connected to client")

    @Slot(int)
    def read_temperature(self):
        if self.client_network is not None:
            value = self.client_network.read_temperature()
            self.signal_new_temperature[float].emit(value)
        else:
            messageBox = QMessageBox()
            messageBox.critical(None, "Network Error", "Not connected to client")



class ActionWidget(QWidget):
    """
        Action Widget
    """

    signal_read_id = Signal((int,))
    signal_read_temperature = Signal((int,))

    def __init__(self, parent=None):
        super().__init__(parent)
        self.init_ui()

    def init_ui(self):
        """
            Initialize all elements
        """

        self.net_client = None

        self.layout = QHBoxLayout()
        self.setLayout(self.layout)

        self.read_id = QPushButton("Read ID")
        self.read_id.clicked.connect(self.connect_read_id)
        self.layout.addWidget(self.read_id)

        self.read_temp = QPushButton("Read Temperature")
        self.read_temp.clicked.connect(self.connect_read_temp)
        self.layout.addWidget(self.read_temp)

    def connect_read_temp(self):
        self.signal_read_temperature[int].emit(0)

    def connect_read_id(self):
        self.signal_read_id[int].emit(0)