from PySide6.QtWidgets import QApplication, QWidget, QHBoxLayout, QVBoxLayout, QLabel, QPushButton, QLineEdit, QMessageBox, QGridLayout
from PySide6.QtGui import QFont, QIcon
from PySide6.QtCore import Qt

from .net.OnewireClient import OnewireClient

class ControlWidget(QWidget):
    

    def __init__(self, ):
        super().__init__()
        self.init()
        self.client_network = None

    
    def init(self):

        self.layout = QGridLayout()
        self.layout.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.setLayout(self.layout)

        self.button0 = QPushButton("Connect")
        self.button0.clicked.connect(self.button_connect_signal)


        self.label1 = QLabel("Address")
        current_font = self.label1.font()
        current_font.setBold(True)
        self.label1.setFont(current_font)

        self.line = QLineEdit()
        self.line.setText("0.0.0.0")
        self.layout.addWidget(self.button0, 0, 0)
        self.layout.addWidget(self.label1, 0, 1)
        self.layout.addWidget(self.line, 0, 2)

        self.label2 = QLabel("Port")
        current_font = self.label2.font()
        current_font.setBold(True)
        self.label2.setFont(current_font)

        self.line2 = QLineEdit()
        self.layout.addWidget(self.label2, 1, 1)
        self.layout.addWidget(self.line2, 1, 2)

        self.buttons = ActionWidget(parent=self)
        self.layout.addWidget(self.buttons, 2, 1)


    def button_connect_signal(self):
        print("button clicked")
        self.client_network = OnewireClient(self.line.text(), self.line2.text())

    def read_id(self):
        self.client_network.read_id()

    def read_temperature(self):
        self.client_network.read_temperature()

class ActionWidget(QWidget):
    """
        Action Widget
    """

    read_id = Signal(int)
    read_temperature = Signal(int)

    def __init__(self, parent=None):
        super().__init__(parent)
        self.init_ui()

    def init_ui(self):
        """
            Initialize all elements
        """

        self.net_client = None

        self.layout = QHBoxLayout()
        self.widget.setLayout(self.layout)

        self.read_id = QPushButton("Read ID")
        self.read_id.clicked.connect(self.connect_read_id)
        self.layout.addWidget(self.read_id)

        self.read_temp = QPushButton("Read Temperature")
        self.read_temp.clicked.connect(self.connect_read_temp)
        self.layout.addWidget(self.read_temp)

    def connect_read_temp(self):
        self.read_temperature.emit(0)

    def connect_read_id(self):
        self.read_id.emit(0)