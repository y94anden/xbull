#!/usr/bin/python3

from argparse import ArgumentParser
from selectors import DefaultSelector, EVENT_READ
from serial import Serial

DEFAULT_BAUDRATE=115200

if __name__ == '__main__':
    parser = ArgumentParser('Join several serialports so they seem to be attached to '
                            'a bus.')
    parser.add_argument('-l', '--loopback', help='Get sent traffic on RX',
                        action='store_true')
    parser.add_argument('-b', '--baud', default=DEFAULT_BAUDRATE, type=int,
                        help='Baudrate. Defaults to %d' % DEFAULT_BAUDRATE)
    parser.add_argument('ports', nargs='+')

    args = parser.parse_args()

    sel = DefaultSelector()

    ports = []
    for port in args.ports:
        p = Serial(port, baudrate=args.baud, timeout=0)
        ports.append(p)
        sel.register(p, EVENT_READ)

    while True:
        events = sel.select()
        for key, mask in events:
            data = key.fileobj.read(100)
            print('Received', len(data), 'bytes from', key.fileobj.port)
            for port in ports:
                if port == key.fileobj:
                    print('Not sending to self', port.port)
                else:
                    port.write(data)
