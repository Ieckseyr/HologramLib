"""皮肤注册表（PlayerNpc 的数据侧）—— endstone 没有暴露 `Image`，所以皮肤只能自己解码。"""

from __future__ import annotations

import base64
import json
import struct
import threading
import zlib
from pathlib import Path


class PngError(ValueError):
    pass


def decode_png(data: bytes) -> tuple[int, int, bytes]:
    """最小 PNG 解码：返回 (宽, 高, RGBA 字节)。只支持 8 位 RGB / RGBA、非隔行。"""
    if len(data) < 8 or data[:8] != b"\x89PNG\r\n\x1a\n":
        raise PngError("不是 PNG")
    pos = 8
    width = height = 0
    color_type = -1
    bit_depth = 0
    idat = bytearray()
    while pos + 8 <= len(data):
        (length,) = struct.unpack(">I", data[pos : pos + 4])
        ctype = data[pos + 4 : pos + 8]
        body = data[pos + 8 : pos + 8 + length]
        pos += 12 + length
        if ctype == b"IHDR":
            width, height, bit_depth, color_type, _comp, _filt, interlace = struct.unpack(">IIBBBBB", body)
            if interlace != 0:
                raise PngError("不支持隔行 PNG")
            if bit_depth != 8 or color_type not in (2, 6):
                raise PngError(f"只支持 8 位 RGB/RGBA，当前 depth={bit_depth} type={color_type}")
        elif ctype == b"IDAT":
            idat += body
        elif ctype == b"IEND":
            break
    if width <= 0 or height <= 0 or not idat:
        raise PngError("PNG 缺 IHDR 或 IDAT")

    channels = 4 if color_type == 6 else 3
    stride = width * channels
    raw = zlib.decompress(bytes(idat))
    out = bytearray(width * height * 4)
    prev = bytearray(stride)
    offset = 0
    for y in range(height):
        if offset >= len(raw):
            raise PngError("PNG 数据不完整")
        filt = raw[offset]
        offset += 1
        line = bytearray(raw[offset : offset + stride])
        offset += stride
        if len(line) < stride:
            raise PngError("PNG 行数据不完整")
        if filt == 1:
            for i in range(channels, stride):
                line[i] = (line[i] + line[i - channels]) & 0xFF
        elif filt == 2:
            for i in range(stride):
                line[i] = (line[i] + prev[i]) & 0xFF
        elif filt == 3:
            for i in range(stride):
                left = line[i - channels] if i >= channels else 0
                line[i] = (line[i] + ((left + prev[i]) >> 1)) & 0xFF
        elif filt == 4:
            for i in range(stride):
                a = line[i - channels] if i >= channels else 0
                b = prev[i]
                c = prev[i - channels] if i >= channels else 0
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                pred = a if (pa <= pb and pa <= pc) else (b if pb <= pc else c)
                line[i] = (line[i] + pred) & 0xFF
        elif filt != 0:
            raise PngError(f"未知 PNG 过滤类型 {filt}")

        if channels == 4:
            out[y * width * 4 : (y + 1) * width * 4] = line
        else:
            dest = y * width * 4
            for x in range(width):
                r, g, b = line[x * 3], line[x * 3 + 1], line[x * 3 + 2]
                out[dest + x * 4] = r
                out[dest + x * 4 + 1] = g
                out[dest + x * 4 + 2] = b
                out[dest + x * 4 + 3] = 0xFF
        prev = line
    return width, height, bytes(out)


class _Skin:
    __slots__ = ("id", "width", "height", "rgba", "geometry", "arm_size", "source", "image_size")

    def __init__(self, skin_id: str, width: int, height: int, rgba: bytes, geometry: str = "", arm_size: int = 0, source: str = "") -> None:
        self.id = skin_id
        self.width = width
        self.height = height
        self.rgba = rgba
        self.geometry = geometry
        self.arm_size = arm_size
        self.source = source
        self.image_size = (0.0, 0.0, False)


class SkinRegistry:
    def __init__(self, plugin) -> None:
        self._plugin = plugin
        self._lock = threading.RLock()
        self._skins: dict[str, _Skin] = {}

    # ── 注册 ──────────────────────────────────────────────────────────────
    def register_png(self, png_path: str, skin_id: str, geometry: str = "", arm_size: int = 0) -> bool:
        path = Path(str(png_path))
        if not skin_id or not path.is_file():
            return False
        try:
            width, height, rgba = decode_png(path.read_bytes())
        except Exception as exc:
            self._plugin.logger.debug(f"registerSkin 解码失败 ({path}): {exc}")
            return False
        return self._store(_Skin(str(skin_id), width, height, rgba, str(geometry), int(arm_size), str(path)))

    def register_raw(self, skin_id: str, width: int, height: int, rgba: bytes, geometry: str = "", arm_size: int = 0) -> bool:
        if not skin_id or width <= 0 or height <= 0 or len(rgba) != width * height * 4:
            return False
        return self._store(_Skin(str(skin_id), int(width), int(height), bytes(rgba), str(geometry), int(arm_size), "raw"))

    def _store(self, skin: _Skin) -> bool:
        with self._lock:
            self._skins[skin.id] = skin
        return True

    def set_image_size(self, skin_id: str, width_blocks: float, height_blocks: float, transparent: bool = False) -> bool:
        skin = self.get(skin_id)
        if skin is None:
            return False
        skin.image_size = (float(width_blocks), float(height_blocks), bool(transparent))
        return True

    def image_size(self, skin_id: str) -> tuple[float, float, bool]:
        skin = self.get(skin_id)
        return skin.image_size if skin is not None else (0.0, 0.0, False)

    # ── 查询 ──────────────────────────────────────────────────────────────
    def has(self, skin_id: str) -> bool:
        with self._lock:
            return str(skin_id) in self._skins

    def get(self, skin_id: str) -> _Skin | None:
        with self._lock:
            return self._skins.get(str(skin_id))

    def get_ids(self) -> list[str]:
        with self._lock:
            return sorted(self._skins.keys())

    def unregister(self, skin_id: str) -> bool:
        with self._lock:
            return self._skins.pop(str(skin_id), None) is not None

    # ── blob 导入导出（自定义 JSON+base64 格式，便于跨进程搬皮肤）──────────
    def export_blob(self, skin_id: str) -> str:
        skin = self.get(skin_id)
        if skin is None:
            return ""
        payload = {
            "id": skin.id,
            "width": skin.width,
            "height": skin.height,
            "geometry": skin.geometry,
            "arm_size": skin.arm_size,
            "rgba": base64.b64encode(skin.rgba).decode("ascii"),
        }
        return base64.b64encode(json.dumps(payload).encode("utf-8")).decode("ascii")

    def import_blob(self, blob: str) -> str:
        try:
            raw = json.loads(base64.b64decode(blob.encode("ascii")).decode("utf-8"))
            return str(raw["id"]) if self.register_raw(
                raw["id"], int(raw["width"]), int(raw["height"]), base64.b64decode(raw["rgba"]),
                raw.get("geometry", ""), int(raw.get("arm_size", 0)),
            ) else ""
        except Exception as exc:
            self._plugin.logger.debug(f"registerSkinFromBlob 失败: {exc}")
            return ""

    def import_dir(self, dir_path: str) -> int:
        path = Path(str(dir_path))
        if not path.is_dir():
            return 0
        count = 0
        for png in sorted(path.glob("*.png")):
            if self.register_png(str(png), png.stem):
                count += 1
        return count
