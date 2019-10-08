#!/usr/bin/python3

from argparse import ArgumentParser
from serial import Serial
from binascii import hexlify
from datetime import datetime

import time
import struct

from port import DEFAULT_PORT

class Bull:
    def __init__(self, port):
        self.serial = Serial(port, baudrate=19200)
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

        self.print('Unit 0x%X: ' % address, end='')

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

        if command == 0xFF:
            self.print('Error response:', self.escape(data))
            ok = False
        else:
            self.print(self.escape(data))

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

    def read_chip_info(self, address):
        data = self.read(address, 0x0A)
        d = dict()
        d['fuse_l'] = '0x%02X' % data[0]
        d['fuse_h'] = '0x%02X' % data[1]
        d['fuse_e'] = '0x%02X' % data[2]
        d['lock'] = '0x%02X' % data[3]
        d['signature'] = ', '.join(['0x%02X' % s for s in data[4:7]])
        d['calibration'] = '0x%02X' % data[7]
        return d

if __name__ == '__main__':
    parser = ArgumentParser()
    parser.add_argument('-p', '--port', help='Serial port to use. Defaults to '
                        + DEFAULT_PORT, default=DEFAULT_PORT)
    group = parser.add_argument_group()
    group.add_argument('-w', '--write', action='store_true', help='Write '
                       'eventhough payload is not supplied')
    group.add_argument('-r', '--read', action='store_true', help='Read '
                       'eventhough payload is supplied')
    parser.add_argument('-s', '--string', action='store_true', help='Payload '
                        'supplied as string. Joined by space.')
    parser.add_argument('-o', '--poll', action='store_true')
    parser.add_argument('-l', '--sleep', type=float, default=0, help='Sleep '
                        'between polls [ms]')
    parser.add_argument('addresses', help='Address(es) of device(s)')
    parser.add_argument('parameter', help='Bull parameter to access')
    parser.add_argument('payload', nargs='*', help='Payload as hex string. When '
                        'supplied, a write will be performed unless -r is supplied')
    args = parser.parse_args()
    

    addresses = [int(address, 0) for address in args.addresses.split()]
    param = int(args.parameter, 0)
    if args.string:
        data = (' '.join(args.payload)).encode()
    else:
        data = b''.fromhex(''.join(args.payload))
    writing = (data and not args.read) or args.write
    b = Bull(args.port)
    b.verbose = 1
    while True:
        for address in addresses:
            if writing:
                b.write(address, param, data)
            else:
                b.read(address, param, data)
        if not args.poll:
            break
        if args.sleep:
            time.sleep(args.sleep/1000)
