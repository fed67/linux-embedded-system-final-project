
from .NetworkWrapper import NetworkWrapper

class OnewireClient:
    """Provides an api for the onewire demon
    """
    def __init__(self, host, port, log = None):
        self.c = NetworkWrapper(host, port)
        self.log = log

    def close(self):
        self.c.close()
    
    def read_id(self) -> bytes:
        if self.log is not None:
            self.log("[OneWire]: sending RA")
        self.c.send("RA")

        return self.c.receive()
    
    def read_temperature(self) -> float:
        if self.log is not None:
            self.log("[OneWire]: sending CP")
        self.c.send("CP")

        if self.log is not None:
            self.log("[OneWire]: sending RS")
        self.c.send("RS")

        data = self.c.receive()
        if self.log is not None:
            self.log(f"[OneWire]: received {data}")


        return float(data)
    
