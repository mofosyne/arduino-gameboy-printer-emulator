import argparse
import os
import re
import serial
import serial.tools.list_ports
from datetime import datetime
import time

from gbp import gbpimage, gbpparser


GBP_EMULATOR_BAUD_RATE = 115200
DEFAULT_OUTPUT_DIR = 'output'
OUTPUTFILE_PREFIX = 'GBP_'
verbose_debug = False


# Debug and testing dummy serial connection
class MockSerial():
    ts = 1
    datafilename = os.path.join(
        'testdata', '2020-08-10_Pokemon_trading_card_compressiontest.txt')
    f = None

    def __init__(self, *args, **kwargs):
        print("**** MockSerial ****")
        if 'timeout' in kwargs:
            self.ts = kwargs['timeout']
        self.f = open(self.datafilename, 'rb')

    def readline(self):
        time.sleep(0.05)
        l = self.f.readline()
        if l:
            return l
        else:
            # Sleep a while and start over
            time.sleep(self.ts)
            self.f = open(self.datafilename, 'rb')
            return None


class EmulatorConnection:
    log = None

    def __init__(self, verbose: bool = False):
        self.conn = None
        self.verbose = verbose

    def open_port(self, port, timeoutms):
        self.conn = serial.Serial(
            port, baudrate=GBP_EMULATOR_BAUD_RATE, timeout=timeoutms/1000)
        # self.conn = MockSerial()

    def debug_print(self, farg, *fargs):
        if self.verbose:
            print(farg, *fargs)

    def closelog(self):
        if self.log:
            self.log.close()
        self.log = None

    def openlog(self, path):
        self.closelog()
        print(f'Opening log {path}')
        self.log = open(path, 'wb')

    def readln(self) -> str:
        data = self.conn.readline()  # NOTE readline uses sole \n as a line separator
        if data:
            self.debug_print('< ', data)
            if self.log:
                self.log.write(data)
            str = data.decode().strip('\r\n ')
            return str
        return None


# Write out png files
def savePNG(pixels, w, h, outfilebase):

    def chunker(seq, size):
        return (seq[pos:pos + size] for pos in range(0, len(seq), size))

    from PIL import Image, ImageDraw
    import numpy as np
    pixels = list(chunker(pixels, w))
    raw = np.array(pixels, dtype=np.uint8)
    out_img = Image.fromarray(raw)
    tmpfile = outfilebase + '.tmp'
    outfile = outfilebase + '.png'
    out_img.save(tmpfile, format='png')
    os.replace(tmpfile, outfile)
    print("Wrote " + outfile)

    # out_img = out_img.resize((w*2, h*2), Image.Resampling.LANCZOS)
    # outfile = outfilebase + "-2x.png"
    # out_img.save(tmpfile, format='png')
    # os.replace(tmpfile, outfile)
    # print("Wrote " + outfile)


def processPackets(packets, outputbase):
    # Decode packet data
    bpp = gbpimage.decodePackets(packets, verbose_debug)
    (tiles, palette) = gbpimage.decode2BPPtoTiles(bpp)

    print(f'{len(tiles)} Tiles. Palette {palette}')
    # Color palette is from GB palette index to grayscale. Usually 0 -> white, 1 and 3 -> black.
    palette = [255, 85, 170, 0]
    (pixels, (w, h)) = gbpimage.decodeTilesToPixels(tiles, palette=palette)

    print(f'{w} x {h} (w x h) {len(pixels)} pixels')

    if len(pixels) == w*h and len(pixels) > 0:
        savePNG(pixels, w, h, outputbase)
        return True
    else:
        print("No image data!")
    return False


def stripComments(hexdata):
    # Removes comments like //.. and /* ... */
    p = re.compile(r'^\/\*.*\*\/|^\/\/.*$', re.MULTILINE)
    return re.sub(p, '', hexdata)


def main():
    description = """
GameBoy Printer Emulator Reader reads image data over serial port and stores decoded images.
Data can be additionally logged to text files.
"""
    parser = argparse.ArgumentParser(
        description=description)
    parser.add_argument('--verbose', action='store_true', help='verbose mode')
    parser.add_argument('-d', '--dest', metavar='DIR',
                        help='Image output directory', default=DEFAULT_OUTPUT_DIR)
    parser.add_argument('-l', '--log', action='store_true',
                        help='Log received data')
    parser.add_argument('-p', '--port', metavar='PORT', help='Serial port')
    # parser.add_argument('-c', '--cmd', nargs='+', metavar='CMD', required=True, help='Command list: LEFT, RIGHT or RESET')
    args = parser.parse_args()

    global verbose_debug
    verbose_debug = args.verbose

    port = None
    if args.port:
        port = args.port
    else:
        # Attempt to find serial port
        if verbose_debug:
            print("Serial ports:")
        ports = list(serial.tools.list_ports.comports())
        for p in ports:
            if verbose_debug:
                print("\t", p)
            # Try to locate Arduino or a clone
            if "Arduino" in p.description or "CH340" in p.description or p.vid == 0x2341:
                port = p.device

    if port:
        print("Device port: ", port)
    else:
        print("ERROR: No Device port found.")
        exit(1)

    outdir = args.dest
    print(f'Output directory: {outdir}')

    if not os.path.exists(outdir):
        print(f"ERROR: Output directory {outdir} not found")
        exit(1)

    dongle = EmulatorConnection(verbose_debug)
    dongle.open_port(port, timeoutms=2000)

    def getoutbasefilename():
        datestr = datetime.now().strftime('%Y-%m-%d %H%M%S')
        return os.path.join(outdir, OUTPUTFILE_PREFIX + datestr)

    def openlog(basefilename: str):
        path = basefilename + ".txt"
        print(f'Opening log {path}')
        return open(path, 'wb')

    print('Waiting for data...')

    while True:
        packets = []
        parser = gbpparser.ParserState([])
        outputbase = getoutbasefilename()
        if args.log:
            dongle.openlog(outputbase + ".txt")

        while True:  # Collect data in loop until timeout
            line = ''
            try:
                line = dongle.readln()
            except KeyboardInterrupt:
                print("\nExiting.. (Ctrl-C)")
                exit(0)
            if line != None:
                line = stripComments(line)
                bytes = gbpparser.to_bytes(line)
                packet = gbpparser.parse_packet_with_state(parser, bytes)
                if packet:
                    if not verbose_debug:
                        print('.', end='', flush=True)
                    packets.append(packet)
                    if not packet.checksumOK:
                        print(
                            f'WARNING: Command {packet.command}. Checksum {hex(packet.checksum)} does not match data.')
                else:
                    if not verbose_debug:
                        print('#', end='', flush=True)

            elif len(packets) > 0:  # timeout, try to process received packets
                dongle.closelog()
                print('')
                try:
                    print(f'Processing {len(packets)} packets')
                    processPackets(packets, outputbase)
                except Exception as ex:
                    print('Failed to process packets.')
                    print(str(ex))
                break


if __name__ == '__main__':
    main()
