from PySide6.QtWidgets import QApplication, QMainWindow, QWidget, QVBoxLayout, QLabel, QPushButton, QLineEdit, QMessageBox
from PySide6.QtCore import Qt, QFile, QTextStream


from ControlWidget import ControlWidget
from Graph2D import Graph2D
from net.OnewireClient import OnewireClient
from DebugWindow import DebugWindow

class Window(QMainWindow):
    """
        Main Window
    """
    def __init__(self):
        super().__init__()
        self.init_ui()

        self.client = None


    def init_ui(self):
        """
            Initialize all elements
        """
        self.setWindowTitle('PySide66')
        self.setGeometry(100, 100, 1000, 700)

        self.widget = QWidget()
        self.setCentralWidget(self.widget)


        layout = QVBoxLayout()
        self.widget.setLayout(layout)

        element0 =  ControlWidget()
        layout.addWidget(element0)

        graph = Graph2D(data=[[1,2,3,4,5], [-1, -2, -4.4, 2, -6]])
        layout.addWidget(graph)

        self.debugWindow = DebugWindow()
        self.debugWindow.show()
        self.debugWindow.setVisible(False)


        self.debug_button = QPushButton("Debug")
        self.debug_button.clicked.connect(self.connect_debug_window)
        layout.addWidget(self.debug_button)

        
    def connect_server(self,tp):
        host, port = tp
        print(f"{host=} {port=}")
        #self.client = OnewireClient(host, port)
        #client.read_temp()

    def connect_debug_window(self):
        self.debugWindow.setVisible( not self.debugWindow.isVisible())



