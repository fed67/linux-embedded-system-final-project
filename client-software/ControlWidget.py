from PySide6.QtWidgets import QApplication, QWidget, QHBoxLayout, QVBoxLayout, QLabel, QPushButton, QLineEdit, QMessageBox, QGridLayout
from PySide6.QtGui import QFont, QIcon
from PySide6.QtCore import Qt

class ControlWidget(QWidget):
    

    def __init__(self, callback):
        super().__init__()
        self.init()
        self.callback = callback

    
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




    def button_connect_signal(self):
        print("button clicked")

        self.callback((self.line.text(), self.line2.text()))
