from dataclasses import dataclass
import re

# Python implementation of https://www.npmjs.com/package/gbp-decode
# Copyright (C) 2023 Teemu Ikonen

COMMAND_INIT = 0x1
COMMAND_PRINT = 0x2
COMMAND_DATA = 0x4
COMMAND_STATUS = 0xf

STATE_AWAIT_MAGIC_BYTES = 0
STATE_AWAIT_COMMAND = 1
STATE_AWAIT_COMPRESSION_INFO = 2
STATE_AWAIT_PACKET_DATA_LENGTH = 3
STATE_AWAIT_DATA = 4
STATE_AWAIT_CHECKSUM = 5
STATE_AWAIT_KEEPALIVE = 6
STATE_AWAIT_STATUS_QUERY = 7

MODE_DETECT_LENGTH = 0
MODE_COMPRESSED = 1
MODE_UNCOMPRESSED = 2


@dataclass
class Packet:
    command: int = 0
    buffer = []
    data = []
    hasCompression = None
    dataLength: int = 0
    checksum: int = 0
    checksumOK = False


def command_to_str(t: int):
    if t == COMMAND_INIT:
        return "COMMAND_INIT"
    if t == COMMAND_PRINT:
        return "COMMAND_PRINT"
    if t == COMMAND_DATA:
        return "COMMAND_DATA"
    if t == COMMAND_STATUS:
        return "COMMAND_STATUS"
    return "COMMAND_UNKNOWN"


# Convert hexadecimal array to bytes ignoring comment lines.
def to_bytes(data: str):
    p = re.compile(r',| ')
    return [int(cc, 16) for line in data.split('\n') if not (line.startswith('//') or line.startswith('#')) for cc in p.split(line.strip()) if cc]


@dataclass
class ParserState:
    buffer: list
    state = STATE_AWAIT_MAGIC_BYTES


def parse_packet_with_state(parser: ParserState, bytes: list):
    parser.buffer.extend(bytes)
    packet = Packet()
    checksum = 0

    # [ 00 ][ 01 ][ 02 ][ 03 ][ 04 ][ 05 ][ 5+X ][5+X+1][5+X+2][5+X+3][5+X+4]
    # [SYNC][SYNC][COMM][COMP][LEN0][LEN1][DATAX][CSUM0][CSUM1][STATUS][STATUS]

    for idx in range(0, len(parser.buffer)):
        byte = parser.buffer[idx]
        if parser.state == STATE_AWAIT_MAGIC_BYTES:
            if len(packet.buffer) == 0 and byte == 0x88:
                packet.buffer.append(byte)
            elif len(packet.buffer) == 1 and byte == 0x33:
                packet.buffer = []
                parser.state = STATE_AWAIT_COMMAND
            else:
                packet = Packet()
        elif parser.state == STATE_AWAIT_COMMAND:
            packet.command = byte
            parser.state = STATE_AWAIT_COMPRESSION_INFO
        elif parser.state == STATE_AWAIT_COMPRESSION_INFO:
            packet.hasCompression = byte
            parser.state = STATE_AWAIT_PACKET_DATA_LENGTH
        elif parser.state == STATE_AWAIT_PACKET_DATA_LENGTH:
            if len(packet.buffer) == 0:
                packet.buffer.append(byte)
            else:
                packet.dataLength = packet.buffer[0] + (byte << 8)
                packet.buffer = []
                if packet.dataLength == 0:
                    parser.state = STATE_AWAIT_CHECKSUM
                else:
                    parser.state = STATE_AWAIT_DATA
        elif parser.state == STATE_AWAIT_DATA:
            if len(packet.buffer) < packet.dataLength:
                packet.buffer.append(byte)
                checksum += byte
            else:
                packet.data = packet.buffer
                packet.buffer = [byte]
                parser.state = STATE_AWAIT_CHECKSUM
        elif parser.state == STATE_AWAIT_CHECKSUM:
            if len(packet.buffer) == 0:
                packet.buffer.append(byte)
            else:
                packet.checksum = packet.buffer[0] + (byte << 8)

                checksum += packet.command
                checksum += packet.hasCompression
                checksum += (packet.dataLength >> 8) & 0xff
                checksum += (packet.dataLength) & 0xff
                checksum = checksum & 0xffff
                packet.checksumOK = (checksum == packet.checksum)

                packet.buffer = []
                parser.state = STATE_AWAIT_KEEPALIVE
        elif parser.state == STATE_AWAIT_KEEPALIVE:
            packet.buffer.append(byte)
            parser.state = STATE_AWAIT_STATUS_QUERY
        elif parser.state == STATE_AWAIT_STATUS_QUERY:
            if len(packet.buffer) == 0:
                packet.buffer.append(byte)
            else:
                packet.buffer = []
                parser.state = STATE_AWAIT_MAGIC_BYTES
                # discard processed packet bytes
                parser.buffer = parser.buffer[idx+1:]
                return packet
    # No packet found, reset to start state
    parser.state = STATE_AWAIT_MAGIC_BYTES
    return None


def parse_packets(bytes):
    packets = []
    parser = ParserState(bytes)
    while True:
        packet = parse_packet_with_state(parser, [])
        if packet:
            packets.append(packet)
        else:
            break
    return packets


def get_imagedata_stream(packets):
    def isstream(packet):
        return packet.command == COMMAND_DATA or packet.command == COMMAND_PRINT
    return list(filter(isstream, packets))


def unpack(data):
    dataOut = []

    mode = MODE_DETECT_LENGTH
    length = 0

    for byte in data:
        if mode == MODE_DETECT_LENGTH:
            if byte & 0x80:
                mode = MODE_COMPRESSED
                length = (byte & 0x7f) + 2
            else:
                mode = MODE_UNCOMPRESSED
                length = byte + 1
        elif mode == MODE_UNCOMPRESSED:
            dataOut.append(byte)
            length -= 1
            if length == 0:
                mode = MODE_DETECT_LENGTH

        elif mode == MODE_COMPRESSED:  # RLE encoded
            dataOut.extend([byte] * length)
            mode = MODE_DETECT_LENGTH
            length = 0

    return dataOut


def decompress_data_stream(packets):
    def decomp(packet):
        if packet.hasCompression:
            packet.data = unpack(packet.data)
        packet.hasCompression = 0

        return packet
    return [decomp(p) if p.command == COMMAND_DATA else p for p in packets]


def parse_palette_byte(paletteRaw):
    return [
        (paletteRaw >> 6) & 0x3,
        (paletteRaw >> 4) & 0x3,
        (paletteRaw >> 2) & 0x3,
        (paletteRaw >> 0) & 0x3,
    ]


def decode_print_commands(packets):
    def decode(packet):
        packet.data = {
            'margins': packet.data[1],
            'marginUpper': packet.data[1] >> 4,
            'marginLower': packet.data[1] & 0xf,
            'palette': packet.data[2],
            'paletteData': parse_palette_byte(packet.data[2]),
        }
        return packet
    return [decode(p) if p.command == COMMAND_PRINT else p for p in packets]


def decode_2BPP(bh, bl):
    return [(((bh >> 7 - i) & 0x1) << 1) | ((bl >> 7 - i) & 0x1)
            for i in range(0, 8)]


def harmonize_palette(bl, bh, paletteDefinition=[3, 2, 1, 0]):
    row = decode_2BPP(bh, bl)
    row = [paletteDefinition[3 - val] for val in row]

    a = 0
    b = 0
    for idx, val in enumerate(row):
        a += (val >> 1) << 7 - idx
        b += (val & 1) << 7 - idx

    return (a & 0xff, b & 0xff)


def harmonize_palettes(packets):
    unharmonized_packets = []
    for packet in packets:
        if packet.command == COMMAND_DATA:
            unharmonized_packets.append(packet)
        elif packet.command == COMMAND_PRINT:
            while unharmonized_packets:
                unharmonized_packet = unharmonized_packets.pop(0)
                data = []
                for i in range(0, len(unharmonized_packet.data), 2):
                    data.extend(harmonize_palette(
                        unharmonized_packet.data[i], unharmonized_packet.data[i + 1], packet.data['paletteData']))
                unharmonized_packet.data = data
    return packets


def transform_to_classic(packets):
    image = {
        'transformed': [],
        'palette': None
    }
    currentLine = []
    images = []
    for packet in packets:
        if packet.command == COMMAND_DATA:
            for i in range(len(packet.data)):
                currentLine.append(hex(packet.data[i])[2:].zfill(2))
                if i % 16 == 15:
                    image['transformed'].append(' '.join(currentLine))
                    currentLine = []
        elif packet.command == COMMAND_PRINT:
            image['palette'] = packet.data.get('paletteData', image['palette'])
            if packet.data.get('marginLower', 0) != 0:
                print(packet.data.get('marginLower'))  # debug
                images.append(image['transformed'])
                image = {
                    'transformed': [],
                    'palette': None
                }
                currentLine = []
    if len(image['transformed']) > 0:
        images.append(image['transformed'])
    return images
