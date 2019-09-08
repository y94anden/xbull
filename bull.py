#!/usr/bin/python3

from serial import Serial
from binascii import hexlify
from datetime import datetime
import time
import struct


class Flasher:
    Resp_STK_OK             =   0x10
    Resp_STK_FAILED         =   0x11
    Resp_STK_UNKNOWN        =   0x12
    Resp_STK_NODEVICE       =   0x13
    Resp_STK_INSYNC         =   0x14
    Resp_STK_NOSYNC         =   0x15

    Parm_STK_SW_MAJOR       =   0x81
    Parm_STK_SW_MINOR       =   0x82

    Cmnd_STK_LOAD_ADDRESS   =   0x55
    Cmnd_STK_PROG_PAGE      =   0x64  # Flash only. Max 256 bytes per write.
    Cmnd_STK_READ_PAGE      =   0x74  # Flash only. Max 256 bytes per read.
    Cmnd_STK_READ_SIGN      =   0x75
    Cmnd_STK_LEAVE_PROGMODE =   0x51
    Cmnd_STK_GET_PARAMETER  =   0x41

    def __init__(self, bull, address):
        self.bull = bull
        self.serial = bull.serial
        self.address = address

    def enter_programming_mode(self):
        org_verbose = self.bull.verbose
        self.bull.verbose = 0
        self.bull.write(self.address, 0x04, 1)
        self.bull.verbose = org_verbose

    def leave_programming_mode(self):
        self.serial.write(b'Q ')
        self.read()  # Clear buffer

    def read(self, length=None):
        r = self.serial.read(length or 512) # length == None -> Extremly long
        if length:
            assert len(r) == length, 'Too short reply'

        assert r[0] == self.Resp_STK_INSYNC, 'Response not in sync'
        assert r[-1] == self.Resp_STK_OK, 'Response not OK'
        return r

    def read_bootloader_version(self):
        self.enter_programming_mode();
        time.sleep(0.2)

        # Read signature
        self.serial.write(b'u ');
        r = self.read(5); # InSync, D1, D2, D3, OK
        print('Signature bytes:', hexlify(r[1:4]).decode())

        # Read SW Major version
        self.serial.write(b'\x41\x81 ')  # Request SW Major version
        r = self.read()
        major = r[1]

        # Read SW Minor version
        self.serial.write(b'\x41\x82 ')  # Request SW Minorversion
        r = self.read()
        minor = r[1]

        print('Optiboot version: %d.%d' % (major, minor))
        self.leave_programming_mode()


class Searcher:
    """
    Class for searching the net for bull devices.
    """
    def __init__(self, bull, nr_slots=255):
        assert isinstance(bull, Bull)
        assert nr_slots > 4 and nr_slots <=255
        self.bull = bull
        self.bull.serial.timeout = 0.1
        self.nr_slots = nr_slots

    def search(self):
        print('Starting search')
        self.start()
        for slot in range(self.nr_slots+1):
            self.read_slot(slot)
        print('All slots read')

    def start(self):
        self.bull.write(0xFF, 0x08, bytes([self.nr_slots]))

    def read_slot(self, slot):
        print('Reading slot % 3d:' % slot, end=' ')
        response = self.bull.read(0xFf, 0x08, bytes([slot]), True)

        if response['ok']:
            print('0x%02X responded.' % response['address'], end=' ')
            if len(response['data']) == 1:
                print('Next selected slot:', int(response['data'][0]))
            else:
                print('Bad slot selection:', response['data'])
        elif response['raw']:
            print('Got garbled response:', response['raw'])
        else:
            print('No response')


class Bull:
    def __init__(self, port):
        self.serial = Serial(port, baudrate=115200)
        self.serial.timeout = 2
        self.verbose = 0

    def __del__(self):
        self.serial.close()

    def print(self, *args, **kwargs):
        if self.verbose:
            print(*args, **kwargs)

    def checksum(self, data):
        sum = 0;
        for d in data:
            sum += d
        return sum % 0x100

    def escape(self, data):
        if not data:
            return '<empty>'
        try:
            assert False not in [(x >= 30 and x < 128) for x in data]
            return '"' + data.decode('ascii') + '"'
        except (UnicodeDecodeError, AssertionError):
            return '0x%s' % hexlify(data).decode()

    def serialRead(self, full_data=False):
        response = self.serial.read(5)
        if len(response) < 5:
            self.print('Too short response:', self.escape(response))
            if full_data:
                return {'ok': False,
                        'raw': response,
                }
            return

        address = response[0]
        command = response[1]
        parameter = response[2]
        length = response[3]
        if length > 0:
            response += self.serial.read(length)
        data = response[4:-1]
        checksum = response[-1]
        ok = True

        if self.checksum(response[:-1]) != checksum:
            self.print('Bad checksum')
            ok = False

        if length + 5 != len(response):
            self.print('Bad length of response')
            ok = False

        self.print('Unit 0x%X responded' % address)

        if command == 0xFF:
            self.print('Error response:', self.escape(data))
            ok = False
        else:
            self.print('data:', self.escape(data))

        if full_data:
            return {'address': address,
                    'command': command,
                    'parameter': parameter,
                    'length': length,
                    'data': data,
                    'checksum': checksum,
                    'ok': ok,
                    'raw': response,
            }

        return data

    def read(self, address, parameter, data=b'', full_data=False):
        msg = bytes([address, 0x01, parameter, len(data)])
        msg += data;
        msg += bytes([self.checksum(msg)]);

        self.serial.write(msg);
        return self.serialRead(full_data)

    def write(self, address, parameter, value, full_data=False):
        if not isinstance(value, bytes):
            value = bytes([value])
        msg = bytes([address, 0x81, parameter, len(value)]) + value
        msg += bytes([self.checksum(msg)])

        self.serial.write(msg)
        return self.serialRead(full_data)

    def read_time(self, address):
        org_verbose = self.verbose
        self.verbose = 0
        d = self.read(address, 0x05)
        t = struct.unpack('I', d)[0]
        dt = datetime.fromtimestamp(t)
        print('Time is %d (%s)' % (t, dt.isoformat(' ')[:19]))
        self.verbose = org_verbose
        return dt

    def write_time(self, address, newtime=None):
        if newtime is None:
            newtime = int(time.time())
        elif isinstance(time, str):
            newtime = datetime.fromisoformat(time)

        data = struct.pack('I', newtime)
        self.write(address, 0x05, data)

    def read_temp(self, address):
        d = self.read(address, 0x22)
        temp = d[0] + d[1]*0x100
        temp /= 16;
        deviceid = hexlify(d[2:][::-1]).decode()
        return temp, deviceid;

if __name__ == '__main__':
    import sys
    if len(sys.argv) < 3:
        print('Supply address, parameter on command line')
        print('If a string of hex bytes is added, the parameter is written')
        sys.exit(1)

    address = int(sys.argv[1], 0)
    param = int(sys.argv[2], 0)
    method = 'Reading from'
    data = None
    if len(sys.argv) >= 4:
        data = b''.fromhex(''.join(sys.argv[3:]))
        method = 'Writing to'

    b = Bull('/dev/ttyUSB0')
    b.verbose = 1
    if data:
        b.write(address, param, data)
    else:
        b.read(address, param)
