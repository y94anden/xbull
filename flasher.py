#!/usr/bin/python3

from argparse import ArgumentParser
from binascii import hexlify
import time

import bull

class FlashException(Exception): pass

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
    Cmnd_STK_GET_SYNC       =   0x30
    Sync_CRC_EOP            =   0x20

    def __init__(self, bull, address, force=False):
        self.bull = bull
        self.serial = bull.serial
        self.address = address
        self.force = force

        if not force:
            # First make sure we have a connection by reading address
            data = self.bull.read(self.address, 1, full_data=True)
            if not data['ok']:
                raise FlashException('No contact with device at address 0x%02X' %
                                     self.address)

        # Next, enter programming mode
        self.enter_programming_mode()

    def enter_programming_mode(self):
        # Make all units deaf until we quiet down. By supplying self.addres,
        # only "our" unit will be listening to next command
        self.bull.timeout = 0.2
        self.bull.write(0xFF, 0x03, self.address)

        # Go inte programming mode. We will not get a response, so timeout
        # immediately.
        self.serial.timeout = 0
        self.bull.write(self.address, 0x04, 1)

        time.sleep(0.5)  # It takes a while for the bootloader to start apparently.                

        # Get in sync
        if self.force:
            # Try finding bootloader for 30 seconds.
            print('Press reset until it gets in sync')
            in_sync = bytes([self.Resp_STK_INSYNC, self.Resp_STK_OK])
            self.serial.timeout = 1
            for i in range(30):
                self.serial.write(b'0 ')  # Cmnd_STK_GET_SYNC
                response = self.serial.read(100)
                if response[-2:] == in_sync:
                    print('Found sync!')
                    break
        else:
            # Try to get in sync the way avrdude does
            self.serial.timeout = 0.01
            self.serial.write(b'0 ')  # Cmnd_STK_GET_SYNC
            self.serial.read(100)
            self.serial.write(b'0 ')  # Cmnd_STK_GET_SYNC
            self.serial.read(100)

        self.serial.write(b'0 ')  # Cmnd_STK_GET_SYNC
        self.read(2)  # Verify the response

    def leave_programming_mode(self):
        self.serial.write(b'Q ')
        self.read()  # Clear buffer

    def read(self, length=None):
        to_read = length or 512  # length == None -> Extremly long
        self.serial.timeout = 0.25
        r = self.serial.read(to_read)
        if length:
            assert len(r) == length, 'Too short reply'

        assert r[0] == self.Resp_STK_INSYNC, 'Response not in sync'
        assert r[-1] == self.Resp_STK_OK, 'Response not OK'
        return r

    def read_bootloader_version(self):
        time.sleep(0.2)

        # Read signature
        self.serial.write(b'u ');
        r = self.read(5); # InSync, D1, D2, D3, OK
        print('Signature bytes:', hexlify(r[1:4]).decode())

        # Read SW Major version
        self.serial.write(b'A\x81 ')  # Request SW Major version
        r = self.read()
        major = r[1]

        # Read SW Minor version
        self.serial.write(b'A\x82 ')  # Request SW Minorversion
        r = self.read()
        minor = r[1]

        print('Optiboot version: %d.%d' % (major, minor))
        self.leave_programming_mode()

    def program_page(self, address, data):
        # Load address
        self.serial.write(b'U')
        address >>= 1  # We should supply the address as word.
        self.serial.write(bytes([address & 0xFF, (address >> 8) & 0xFF]))
        self.serial.write(b' ')
        self.read(2)

        # Program page
        block_size = len(data)
        self.serial.write(b'd')                               # program page
        self.serial.write(bytes([(block_size >> 8) & 0xFF]))  # high bytes
        self.serial.write(bytes([block_size & 0xFF]))         # low bytes
        self.serial.write(b'F')                               # memtype = flash
        self.serial.write(data)
        self.serial.write(b' ')
        self.read(2)

    def read_page(self, address, block_size):
        # Load address
        self.serial.write(b'U')
        address >>= 1  # We should supply the address as word.
        self.serial.write(bytes([address & 0xFF, (address >> 8) & 0xFF]))
        self.serial.write(b' ')
        self.read(2)

        # Read page
        self.serial.write(b't')                               # read page
        self.serial.write(bytes([(block_size >> 8) & 0xFF]))  # high bytes
        self.serial.write(bytes([block_size & 0xFF]))         # low bytes
        self.serial.write(b'F')                               # memtype = flash
        self.serial.write(b' ')
        r = self.read(block_size + 2)
        return r[1:-1]

    def read_hex(self, filename):
        data = b''
        eof_found = False
        start_address = None
        with open(filename) as f:
            for i, line in enumerate(f):
                assert not eof_found
                assert line[0] == ':', 'Not starting with colon. Hex format?'
                line = b''.fromhex(line[1:].strip())  # Convert to binary
                checksum  = (((sum(line[:-1]) & 0xFF) ^ 0xFF) + 1) & 0xFF
                assert checksum == line[-1], 'Bad checksum line %d' % i
                length = line[0]
                offset = (line[1] << 8) + line[2]
                rec_type = line[3]
                if rec_type == 0x01:
                    eof_found = True
                elif rec_type == 0x00:
                    if start_address is None:
                        start_address = offset;
                    while offset > start_address + len(data):
                        # There was a hole in the supplied data. Fill with FF.
                        data += b'\xFF'
                    assert offset == start_address + len(data), 'Hex file not contigous'
                    d = line[4:-1]
                    assert len(d) == length, 'Wrong length on line %d' % i
                    data += d
                elif rec_type == 0x03:
                    # Start segment address. Ignore it.
                    pass
                else:
                    assert False, 'Unhandled record type: 0x%02X ' \
                        'on line %d' % (rec_type, i)
        return data

    def validate(self, filename):
        ref = self.read_hex(filename)

        address = 0
        data = b''
        time.sleep(0.2)
        while address < len(ref):
            block_size = min(128, len(ref) - address)
            print('\rReading address 0x%X' % address, end='  ', flush=True)
            data += self.read_page(address, block_size)
            address += block_size
        print()

        if ref == data:
            print('Validation successful')
        else:
            print('Read data does not match hex file')
        self.leave_programming_mode()

        return data, ref

    def write_hex(self, filename):
        data = self.read_hex(filename)

        time.sleep(0.2)
        address = 0
        block_size = 128
        print('About to program device:')
        while data:
            self.program_page(address, data[:block_size])
            data = data[block_size:]
            address += block_size
            print('\rWrote', address, 'of', address + len(data), end='   ')

        print('\nDone!')
        self.leave_programming_mode()


if __name__ == '__main__':
    parser = ArgumentParser()
    parser.add_argument('-p', '--port', help='Serial port to use. Defaults to '
                        + bull.DEFAULT_PORT, default=bull.DEFAULT_PORT)
    parser.add_argument('-v', '--validate', help='Validate flash with hex file',
                        action='store_true')
    parser.add_argument('-f', '--force', action='store_true',
                        help='Program device even though it does not respond. '
                        'Can be used if device is stuck in bootloader. Note '
                        'that after power on, bootloader is not active.')
    parser.add_argument('address', help='Address of device')
    parser.add_argument('hexfile', help='File to write or verify against. If '
                        'not specified, the signature bytes are read.',
                        nargs='?')
    args = parser.parse_args()

    b = bull.Bull(args.port)

    address = int(args.address, 0)
    f = Flasher(b, address, force=args.force)

    if not args.hexfile:
        f.read_bootloader_version()
    elif args.validate:
        f.validate(args.hexfile)
    else:
        f.write_hex(args.hexfile)

