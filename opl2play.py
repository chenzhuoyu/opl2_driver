#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import io
import time
import serial
import signal
import struct

from typing import Tuple
from typing import BinaryIO
from typing import Iterable
from typing import Optional

ERR_OK     = 0xe0
ERR_INVAL  = 0xe1
ERR_OVERFL = 0xe2

CMD_RESET  = 0xc0
CMD_WRITE  = 0xc1
CMD_QUERY  = 0xc2

OPL_SLEEP  = 0xd0
OPL_WRITE  = 0xd1
OPL_CLOSE  = 0xd2

VGM_MAGIC                 = b'Vgm '
VGM_MAGIC_GD3             = b'Gd3 '
VGM_SAMPLES_PR_SECOND     = 44100

VGM_OFFSET_GD3            = 0x14
VGM_OFFSET_NUM_SAMPLE     = 0x18
VGM_OFFSET_LOOP_OFFSET    = 0x1c
VGM_OFFSET_DATA_OFFSET    = 0x34

VGM_CODE_CHIP_YM3812      = 0x5a
VGM_CODE_WAIT_N_SAMPLES   = 0x61
VGM_CODE_WAIT_735_SAMPLES = 0x62
VGM_CODE_WAIT_882_SAMPLES = 0x63
VGM_CODE_END_OF_DATA      = 0x66
VGM_CODE_WAIT_1_SAMPLE    = 0x70
VGM_CODE_WAIT_16_SAMPLES  = 0x7f

class VGMetaData:
    minutes     : int
    seconds     : int
    centisecs   : int
    samples     : int
    title       : str
    titlejp     : str
    name        : str
    namejp      : str
    system      : str
    systemjp    : str
    composer    : str
    composerjp  : str
    release     : str
    vgm_by      : str
    notes       : str

    def __init__(self):
        self.minutes    = 0
        self.seconds    = 0
        self.centisecs  = 0
        self.samples    = 0
        self.title      = ''
        self.titlejp    = ''
        self.name       = ''
        self.namejp     = ''
        self.system     = ''
        self.systemjp   = ''
        self.composer   = ''
        self.composerjp = ''
        self.release    = ''
        self.vgm_by     = ''
        self.notes      = ''

    def __str__(self) -> str:
        return '\n'.join([
            'Duration    : %d:%02d.%02d (%d samples)' % (self.minutes, self.seconds, self.centisecs, self.samples),
            'Track Title : %s%s' % (self.title   , ' (%s)' % self.titlejp    if self.titlejp    else ''),
            'Game Name   : %s%s' % (self.name    , ' (%s)' % self.namejp     if self.namejp     else ''),
            'System      : %s%s' % (self.system  , ' (%s)' % self.systemjp   if self.systemjp   else ''),
            'Composer    : %s%s' % (self.composer, ' (%s)' % self.composerjp if self.composerjp else ''),
            'Release     : ' + self.release,
            'VGM By      : ' + self.vgm_by,
            'Notes       : ' + self.notes,
        ])

class VGMCommand:
    pos: int

class VGMWaitSamples(VGMCommand):
    pos: int
    num: int

    def __init__(self, pos: int, num: int):
        self.pos = pos
        self.num = num

    def __repr__(self) -> str:
        return '{WAIT %d}' % self.num

class VGMWriteRegister(VGMCommand):
    pos: int
    reg: int
    val: int

    def __init__(self, pos: int, reg: int, val: int):
        self.pos = pos
        self.reg = reg
        self.val = val

    def __repr__(self) -> str:
        return '{WRITE %#02x=%#02x}' % (self.reg, self.val)

class VGM:
    _lp: int
    _fp: BinaryIO
    _md: VGMetaData

    def __init__(self, fp: BinaryIO):
        self._fp = fp
        self._md = self._read_meta()
        self._lp = self._read_loop()

    @property
    def meta(self) -> VGMetaData:
        return self._md

    @property
    def data(self) -> Iterable[VGMCommand]:
        pos = -1
        loop = -1
        self._fp.seek(VGM_OFFSET_DATA_OFFSET)
        self._fp.seek(VGM_OFFSET_DATA_OFFSET + struct.unpack('=I', self._fp.read(4))[0])

        # read every instructions
        while True:
            pos += 1
            cmd, = self._fp.read(1)

            # set the loop position
            if self._fp.tell() == VGM_OFFSET_LOOP_OFFSET + self._lp + 1:
                loop = pos - 1

            # command dispatch
            if cmd == VGM_CODE_CHIP_YM3812:
                buf = self._fp.read(2)
                yield VGMWriteRegister(pos, buf[0], buf[1])

            # handle end of data
            elif cmd == VGM_CODE_END_OF_DATA:
                if not self._lp:
                    break
                else:
                    pos = loop
                    self._fp.seek(VGM_OFFSET_LOOP_OFFSET + self._lp)
                    continue

            # it must be WaitSample commands
            elif cmd == VGM_CODE_WAIT_735_SAMPLES:
                yield VGMWaitSamples(pos, 735)
            elif cmd == VGM_CODE_WAIT_882_SAMPLES:
                yield VGMWaitSamples(pos, 882)
            elif cmd == VGM_CODE_WAIT_N_SAMPLES:
                yield VGMWaitSamples(pos, struct.unpack('=H', self._fp.read(2))[0])
            elif cmd >= VGM_CODE_WAIT_1_SAMPLE and cmd <= VGM_CODE_WAIT_16_SAMPLES:
                yield VGMWaitSamples(pos, cmd - VGM_CODE_WAIT_1_SAMPLE + 1)
            else:
                raise IOError('invalid VGM command: ' + hex(cmd))

    @staticmethod
    def _decode_ucs2(v: bytes) -> Iterable[bytes]:
        buf = bytearray()
        for a, b in zip(*[iter(v)] * 2):
            if a == b == 0:
                yield bytes(buf)
                buf.clear()
            else:
                buf.append(a)
                buf.append(b)
        if buf:
            yield buf

    def _read_loop(self) -> int:
        self._fp.seek(VGM_OFFSET_LOOP_OFFSET)
        return struct.unpack('=I', self._fp.read(4))[0]

    def _read_meta(self) -> VGMetaData:
        self._fp.seek(0)
        val = self._fp.read(4)

        # check for magic
        if val != VGM_MAGIC:
            raise IOError('invalid VGM magic')

        # read sample count
        self._fp.seek(VGM_OFFSET_NUM_SAMPLE)
        nsmp = struct.unpack('=I', self._fp.read(4))[0]
        rems = nsmp % (VGM_SAMPLES_PR_SECOND * 60)

        # build the metadata
        md           = VGMetaData()
        md.samples   = nsmp
        md.minutes   = md.samples // (VGM_SAMPLES_PR_SECOND * 60)
        md.seconds   = rems // VGM_SAMPLES_PR_SECOND
        md.centisecs = (rems % VGM_SAMPLES_PR_SECOND) // (VGM_SAMPLES_PR_SECOND // 100)

        # read GD3 offset
        self._fp.seek(VGM_OFFSET_GD3)
        gd3v, = struct.unpack('=I', self._fp.read(4))

        # check for GD3 metadata
        if gd3v == 0:
            return md

        # read actual GD3 data
        self._fp.seek(VGM_OFFSET_GD3 + gd3v)
        gd3m, nbuf = struct.unpack('=4s4xI', self._fp.read(12))

        # check for GD3 magic
        if gd3m != VGM_MAGIC_GD3:
            raise IOError('invalid GD3 magic')

        # decode the GD3 buffer
        vals = self._decode_ucs2(self._fp.read(nbuf))
        vals = [val.decode('utf-16le') for val in vals]

        # should have 11 elements
        if len(vals) != 11:
            raise IOError('invalid GD3 tag: ' + repr(vals))

        # unpack the values
        md.title      = vals[0]
        md.titlejp    = vals[1]
        md.name       = vals[2]
        md.namejp     = vals[3]
        md.system     = vals[4]
        md.systemjp   = vals[5]
        md.composer   = vals[6]
        md.composerjp = vals[7]
        md.release    = vals[8]
        md.vgm_by     = vals[9]
        md.notes      = vals[10]
        return md

def opl2_cmd(com: serial.Serial, cmd: int, args: bytes = b''):
    com.write(struct.pack('=BB', cmd, len(args)))
    com.write(args)

def opl2_ack(com: serial.Serial, read_reply: bool = False) -> Optional[Tuple[int, int]]:
    buf = com.read(1)
    ret = struct.unpack('=B', buf)[0]

    # check for errors
    if ret == ERR_OVERFL:
        raise BufferError()
    elif ret != ERR_OK:
        raise IOError('OPL2 error: %#02x' % ret)
    elif read_reply:
        return struct.unpack('=HH', com.read(4))
    else:
        return None

def make_wait(cmd: VGMWaitSamples) -> Iterable[bytes]:
    ns = cmd.num
    ms = int(ns / VGM_SAMPLES_PR_SECOND * 1000)

    # wait in 65535ms chunks
    while ms >= 65535:
        ms -= 65535
        yield struct.pack('=BH', OPL_SLEEP, 65535)

    # the last few milliseconds
    if ms != 0:
        yield struct.pack('=BH', OPL_SLEEP, ms)

def make_write(cmd: VGMWriteRegister) -> bytes:
    return struct.pack('=BBB', OPL_WRITE, cmd.reg, cmd.val)

def main():
    sdev  = '/dev/tty.usbserial-2330'
    sbaud = 1000000
    fname = '/Users/chenzhuoyu/Sources/tests/test_vgm/chinese.vgm'

    # read the VGM file
    with open(fname, 'rb') as fp:
        fp = io.BytesIO(fp.read())

    # start the console
    vgm = VGM(fp)
    com = serial.Serial(sdev, sbaud)

    # print metadata
    print('File Name   : ' + fname)
    print(vgm.meta)

    # reset the OPL2 chip
    opl2_cmd(com, CMD_RESET)
    opl2_ack(com)

    def stop(*_):
        nonlocal run
        run = False

    # reset the OPL2 on exit
    signal.signal(signal.SIGHUP, stop)
    signal.signal(signal.SIGINT, stop)
    signal.signal(signal.SIGTERM, stop)

    # mark the start time
    buf = []
    run = True
    eof = False
    src = iter(vgm.data)

    # execute every command
    while run:
        opl2_cmd(com, CMD_QUERY)
        cap, nb = opl2_ack(com, read_reply = True)
        wlim, out = cap - nb, b''

        # accumulate some data
        while run and sum(map(len, buf)) < wlim:
            try:
                cmd = next(src)
            except StopIteration:
                eof = True
                break

            # write command
            if isinstance(cmd, VGMWaitSamples):
                buf.extend(make_wait(cmd))
            elif isinstance(cmd, VGMWriteRegister):
                buf.append(make_write(cmd))
            else:
                raise RuntimeError('invalid command: ' + repr(cmd))

        # check for EOF
        if eof or not run:
            break

        # write at most 16 bytes every time
        if wlim > 16:
            wlim = 16

        # fill as much data as possible
        while run and buf and len(out) + len(buf[0]) <= wlim:
            out += buf[0]
            buf  = buf[1:]

        # write the data, if possible
        if run and out:
            opl2_cmd(com, CMD_WRITE, out)
            opl2_ack(com)

    # close the OPL2 chip on exit
    if not run:
        opl2_cmd(com, CMD_RESET)
        opl2_ack(com)
    else:
        opl2_cmd(com, CMD_WRITE, struct.pack('=B', OPL_CLOSE))
        opl2_ack(com)
        opl2_ack(com)

if __name__ == '__main__':
    main()
