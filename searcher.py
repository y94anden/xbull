#!/usr/bin/python3

from argparse import ArgumentParser
import bull

class ResponseGarbage(Exception): pass

class Searcher:
    """
    Class for searching the net for bull devices.
    """
    def __init__(self, b, nr_slots=255):
        assert isinstance(b, bull.Bull)
        assert nr_slots > 4 and nr_slots <=255
        self.bull = b
        self.bull.serial.timeout = 0.1
        self.nr_slots = nr_slots

    def search(self):
        self.start()
        units = []
        errorslots = []
        for slot in range(self.nr_slots+1):
            unit = self.read_slot(slot)
            if unit:
                units.append(unit)
        return units

    def start(self):
        self.bull.write(0xFF, 0x08, bytes([self.nr_slots]))

    def read_slot(self, slot):
        print('Reading slot % 3d:' % slot, end=' ')
        unit = None
        response = self.bull.read(0xFf, 0x08, bytes([slot]), True)

        if response['ok']:
            print('0x%02X responded.' % response['address'], end=' ')
            if len(response['data']) == 1:
                next_slot = int(response['data'][0])
                print('Next selected slot:', next_slot)
                unit = (response['address'], next_slot)
            else:
                print('Bad slot selection:', response['data'])
        elif response['raw']:
            print('Got garbled response:', response['raw'])
        else:
            print('No response', end='\r')

        return unit


if __name__ == '__main__':
    parser = ArgumentParser()
    parser.add_argument('-p', '--port', help='Serial port to use. Defaults to '
                        + bull.DEFAULT_PORT, default=bull.DEFAULT_PORT)
    parser.add_argument('-s', '--slots', type=int, default=30, help='The '
                        'number of slots to divide the search in. Default 30')
    parser.add_argument('-r', '--rounds', type=int, default=5, help='Number of '
                        'rounds to run search. Defaults to 5')
    parser.add_argument('-a', '--autoset', action='store_true', help='Autoset '
                        'id of units with id == 0')
    parser.add_argument('ignored', nargs='*', help='Addresses of units that '
                        'are to be silenced before search')
    args = parser.parse_args()

    b = bull.Bull(args.port)

    for address in args.ignored:
        address = int(address, 0)
        # Tell unit to stop listening to traffic until it has been quiet
        # for 5 seconds.
        b.write(address, 0x03)

    s = Searcher(b, args.slots)

    for i in range(args.rounds):
        print('Starting search round %d of %d' % (i+1, args.rounds))
        units = s.search()
        print(' ' * 40)

    if args.autoset:
        used = args.ignored
        used.append(0)  # We do not want this either.
        for unit, next_slot in units:
            if unit in used:
                suggestions = [i for i in range(255) if i not in used]
                newid = suggestions[0]
                print('Addressing 0x%02X unit as 0x%02X' % (unit, newid))
                # Since multiple units might have the same address, identify
                # them by their selected next_slot
                b.write(unit, 0x01, bytes([newid, next_slot]))
                unit = newid
            used.append(unit)
