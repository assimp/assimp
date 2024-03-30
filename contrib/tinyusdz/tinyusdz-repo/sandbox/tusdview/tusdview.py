
import os, sys

# workaround.
# import PySide6 first, then import numpy, cv2, etc. otherwise 
#
# qt.qpa.wayland: Failed to initialize EGL display 3001
#
# error happens in WSL2 environment

from PySide6.QtWidgets import QApplication, QMainWindow, QWidget, QLabel, QFileDialog
from PySide6.QtGui import QAction, QImage, QPixmap

#import cv2
import numpy as np


class MainWindow(QMainWindow):

    def __init__(self, parent=None):
       super().__init__(parent)

       self.setup()
   
    def setup(self):

        self.width = 512
        self.height = 512

        # HWC
        img = np.zeros((self.height, self.width, 3)).astype(np.uint8)
    
        for x in range(self.width):
            for y in range(self.height):
                img[x,y,0] = x % 256
                img[x,y,1] = y % 256
                img[x,y,2] = 127

        stride = img.strides[0]

        qimg = QImage(img.data, self.width, self.height, stride, QImage.Format.Format_RGB888)       
        pixmap = QPixmap(QPixmap.fromImage(qimg))
        imgLabel = QLabel(self)
        imgLabel.setPixmap(pixmap)
        
        #self.resize(imgLabel.pixmap().size())

        open_file = QAction("Open(&O)", self)
        open_file.setShortcut("Ctrl+O")
        open_file.triggered.connect(self.openFile)

        quit_app = QAction("Quit(&Q)", self)
        quit_app.setShortcut("Ctrl+Q")
        quit_app.triggered.connect(self.quitApp)
         
        menu = self.menuBar()
        file_menu = menu.addMenu("File")
        file_menu.addAction(open_file)
        file_menu.addAction(quit_app)

             
        self.setWindowTitle("TinyUSDZ viewer")

    def openFile(self):
        filters = "USD files(*.usd *.usdc *.usda *.usdz);;Any files (*)"
        filename = QFileDialog.getOpenFileName(filter=filters)
        print(filename)
        
    def quitApp(self):
        import sys
        sys.exit()
        
if __name__ == '__main__':
    app = QApplication([])
    window = MainWindow()
    window.show()
    app.exec()
    


