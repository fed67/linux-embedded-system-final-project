from PySide6.QtWidgets import QWidget
from PySide6.QtCore import QDateTime, Qt
from PySide6.QtGui import QPainter
from PySide6.QtWidgets import QVBoxLayout, QSizePolicy, QLabel
from PySide6.QtCharts import QChart, QChartView, QLineSeries, QValueAxis


class Graph2D(QWidget):
    """Graph Widget"""
    def __init__(self, data):
        super().__init__()
        self.init(data)

    def init(self, data):

        # Creating QChart and QChartView
        self.chart = QChart()
        self.chart.setAnimationOptions(QChart.AllAnimations)

        self.chart_view = QChartView(self.chart)
        self.chart_view.setRenderHint(QPainter.Antialiasing)

        # Layout
        self.main_layout = QVBoxLayout()
        size = QSizePolicy()
        size.setHorizontalPolicy(QSizePolicy.Policy.Maximum)
        

        label = QLabel("Graph2d")
        self.main_layout.addWidget(label)


        # Right Layout
        self.main_layout.addWidget(self.chart_view)

        self.setLayout(self.main_layout)

        self.setData(data)

    def appendData(self, data, qdata : QLineSeries):
        paired_list = zip(data[0], data[1])
        for el in paired_list:
            x, y = el
            self.qdata.append(x, y)

        self.chart.addSeries(self.qdata)


    def setData(self, data : list):

        self.qdata = QLineSeries()
        self.qdata.setName("Default data")

        paired_list = zip(data[0], data[1])
        for el in paired_list:
            x, y = el
            self.qdata.append(x, y)

        #before the axis is initalized
        self.chart.addSeries(self.qdata)

        #x axis
        self.axis_x = QValueAxis()
        self.axis_x.setTitleText("X Data")
        self.axis_x.setLabelFormat("%.2f")
        self.chart.addAxis(self.axis_x, Qt.AlignBottom)
        self.qdata.attachAxis(self.axis_x)
        # y axis
        self.axis_y = QValueAxis()
        self.axis_y.setLabelFormat("%.2f")
        self.axis_y.setTitleText("Y Data")
        self.chart.addAxis(self.axis_y, Qt.AlignLeft)
        self.qdata.attachAxis(self.axis_y)


    def data_update(data):
        self.qdata.clear()

        paired_list = zip(data[0], data[1])
        for el in paired_list:
            x, y = el
            self.qdata.append(x, y)



