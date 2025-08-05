
from .NetworkWrapper import NetworkWrapper

class OnewireClient:
    """Provides an api for the onewire demon
    """
    def __init__(self, host, port):
        self.c = NetworkWrapper(host, port)

    
    def read_temp(self) -> bytes:
        self.c.send("RA")

        return self.c.receive()
    
    def read_temperature(self):
        self.c.send("CP")
        self.c.send("RS")
        return self.c.receive()
    
