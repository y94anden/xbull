from serial import Serial
from binascii import hexlify

class Bull:
    def __init__(self, port):
        self.serial = Serial(port, baudrate=19200)
        self.serial.timeout = 0.2

    def checksum(self, data):
        sum = 0;
        for d in data:
            sum += d
        return sum % 0x100

    def hex(self, data):
        if not data:
            return '<empty>'

        return '0x%s' % hexlify(data).decode()

    def serialRead(self):
        response = self.serial.read(260)
        if len(response) < 5:
            print('Too short response:', self.hex(response))
            return

        address = response[0]
        command = response[1]
        parameter = response[2]
        length = response[3]
        data = response[4:-1]
        checksum = response[-1]

        if self.checksum(response[:-1]) != checksum:
            print('Bad checksum')

        if length + 5 != len(response):
            print('Bad length of response')

        if command == 0xFF:
            print('Error response:', data)
        else:
            print('data:', self.hex(data))

    def read(self, address, parameter):
        msg = bytes([address, 0x01, parameter, 0x00])
        msg += bytes([self.checksum(msg)]);

        self.serial.write(msg);
        self.serialRead()

    def write(self, address, parameter, value):
        if not isinstance(value, bytes):
            value = bytes([value])
        msg = bytes([address, 0x81, parameter, len(value)]) + value
        msg += bytes([self.checksum(msg)])

        self.serial.write(msg)
        self.serialRead()
