"""PRUDP v1 codec (RendezVous variation) - pure Python, little-endian.

Grounding: kinnay/NintendoClients PRUDP wiki + nex-go encoder. This models the
Quazal RendezVous PRUDPv1 framing we expect FC3's `prudp:/` RV client to speak.
It is intentionally tolerant on parse (keeps every option verbatim) so that whatever
exact RV option IDs the FC3 client sends are preserved for replay, while the server
can still answer the SYN->CONNECT handshake.

Packet layout (little endian):
  magic:      2 bytes = 0xEA 0xD0            (u16 LE 0xD0EA)
  version:    1 byte  = 1
  options_len:1 byte                          (length of the options block)
  payload_len:2 bytes                          (length of the payload)
  source:     1 byte
  destination:1 byte
  type_flags: 2 bytes = (flags << 4) | type
  session_id: 1 byte
  substream:  1 byte
  sequence:   2 bytes
  [signature] 2 bytes  if FLAG_HAS_SIGNATURE  (header checksum)
  options:    options_len bytes  (each: id(1), (type<<4|size)(1), value[size])
  payload:    payload_len bytes
"""
from __future__ import annotations
import struct
from dataclasses import dataclass, field

MAGIC = b"\xEA\xD0"
PRUDP_VERSION = 1

# packet types (low nibble of type_flags)
SYN, CONNECT, DATA, DISCONNECT, PING, USER, ROUTE, RAW = range(8)
TYPE_NAMES = {SYN:"SYN", CONNECT:"CONNECT", DATA:"DATA", DISCONNECT:"DISCONNECT", PING:"PING"}

# flags (the field holds (flags<<4)|type; masks below are on the *flags* value)
FLAG_ACK        = 0x0001
FLAG_RELIABLE   = 0x0002
FLAG_NEED_ACK   = 0x0004
FLAG_HAS_SIGNATURE = 0x0008   # some stacks use bit 15 (0x8000<<4); we detect both

# option value element types (high nibble of the option size byte)
OPT_RAW, OPT_BOOL, OPT_UINT8, OPT_UINT16, OPT_UINT32, OPT_UINT64, OPT_STRING = range(7)


@dataclass
class Option:
    id: int
    size: int            # byte length of value
    type: int            # element type (high nibble)
    raw: bytes           # value bytes, verbatim

    def as_int(self) -> int:
        return int.from_bytes(self.raw, "little", signed=False)


@dataclass
class PRUDPPacket:
    type: int = DATA
    flags: int = 0
    session_id: int = 0
    substream_id: int = 0
    sequence: int = 0
    source: int = 0
    destination: int = 1
    options: list = field(default_factory=list)
    payload: bytes = b""

    def type_name(self) -> str:
        return TYPE_NAMES.get(self.type, str(self.type))

    def has_flag(self, f: int) -> bool:
        return (self.flags & f) != 0

    # ---- encode ----
    def encode(self, with_signature: bool = False) -> bytes:
        opts = b""
        for o in self.options:
            opts += struct.pack("<BB", o.id, ((o.type & 0xF) << 4) | (o.size & 0xF))
            opts += o.raw
        type_flags = ((self.flags & 0xFFFF) << 4) | (self.type & 0xF)
        if with_signature:
            self.flags |= 0x0008
            type_flags = ((self.flags & 0xFFFF) << 4) | (self.type & 0xF)
        header = struct.pack(
            "<BBHBBHBBH",
            PRUDP_VERSION, len(opts), len(self.payload),
            self.source, self.destination, type_flags,
            self.session_id, self.substream_id, self.sequence,
        )
        sig = b""
        if with_signature:
            sig = struct.pack("<H", _header_checksum(header))
        return MAGIC + header + sig + opts + self.payload

    # ---- decode ----
    @staticmethod
    def decode(buf: bytes) -> "PRUDPPacket":
        if len(buf) < 14:
            raise ValueError(f"too short for PRUDP v1 ({len(buf)} bytes)")
        if buf[0:2] != MAGIC:
            raise ValueError(f"bad magic {buf[0:2].hex()}")
        version = buf[2]
        if version != PRUDP_VERSION:
            raise ValueError(f"not PRUDP v1 (version={version}); consider v0 codec")
        options_len = buf[3]
        payload_len = struct.unpack_from("<H", buf, 4)[0]
        source = buf[6]
        destination = buf[7]
        type_flags = struct.unpack_from("<H", buf, 8)[0]
        session_id = buf[10]
        substream = buf[11]
        sequence = struct.unpack_from("<H", buf, 12)[0]
        ptype = type_flags & 0xF
        flags = (type_flags >> 4) & 0xFFFF
        # fixed header after magic is 12 bytes (offsets 2..13). signature present per flag
        off = 14
        if (flags & 0x0008) or (type_flags & 0x8000):
            off += 2  # skip 2-byte header signature
        opt_bytes = buf[off:off + options_len]
        opts = PRUDPPacket._parse_options(opt_bytes)
        off += options_len
        payload = buf[off:off + payload_len]
        pk = PRUDPPacket(ptype, flags, session_id, substream, sequence,
                         source, destination, opts, payload)
        return pk

    @staticmethod
    def _parse_options(b: bytes) -> list:
        out = []
        i = 0
        while i + 2 <= len(b):
            oid = b[i]
            szb = b[i + 1]
            size = szb & 0xF
            etype = (szb >> 4) & 0xF
            val = b[i + 2:i + 2 + size]
            out.append(Option(oid, size, etype, val))
            i += 2 + size
        return out


def _header_checksum(header: bytes) -> int:
    # PRUDP v1 header checksum: simple additive fold (placeholder; refined against a live capture).
    s = 0
    for b in header:
        s = (s + b) & 0xFFFF
    return s ^ 0xFFFF


# ---- RMC (Remote Method Call) layer, carried in the PRUDP DATA payload ----
# NOTE: this is a clean, self-consistent scaffolding header (our synthetic client and
# server agree on it) so the request->response plumbing is testable OFFLINE. The exact
# Quazal-RendezVous RMC wire encoding + protocol/method IDs for FC3 MUST be confirmed from
# a live capture (S0, gated on condition A). Kept separate so swapping in the real layout
# later is contained.
RMC_REQUEST, RMC_RESPONSE, RMC_RESPONSE_ERROR = 0, 1, 2


@dataclass
class RMCMessage:
    call_type: int = RMC_REQUEST
    protocol_id: int = 0
    method_id: int = 0
    session_id: int = 0
    body: bytes = b""

    def encode(self) -> bytes:
        # header: u8 call_type, u16 protocol_id, u16 method_id, u32 session_id, then body
        return struct.pack("<BHHi", self.call_type, self.protocol_id, self.method_id, self.session_id) + self.body

    @staticmethod
    def parse(buf: bytes) -> "RMCMessage":
        if len(buf) < 9:
            raise ValueError("payload too short for RMC header")
        ct, pid, mid, sid = struct.unpack_from("<BHHi", buf, 0)
        return RMCMessage(ct, pid, mid, sid, buf[9:])


def opt(oid: int, value: int, etype: int = OPT_UINT32) -> Option:
    """Build an integer option (size from type)."""
    if etype in (OPT_BOOL, OPT_UINT8):
        raw = struct.pack("<B", value)
    elif etype == OPT_UINT16:
        raw = struct.pack("<H", value)
    elif etype == OPT_UINT64:
        raw = struct.pack("<Q", value)
    else:
        raw = struct.pack("<I", value)
    return Option(oid, len(raw), etype, raw)
