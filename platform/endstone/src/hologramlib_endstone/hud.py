"""HUD（界面元素）域 —— LeviLamina 版 HudManager 的 endstone 移植。"""

from __future__ import annotations

import threading

HUD_PAPER_DOLL = 0
HUD_ARMOR = 1
HUD_TOOL_TIPS = 2
HUD_TOUCH_CONTROLS = 3
HUD_CROSSHAIR = 4
HUD_HOTBAR = 5
HUD_HEALTH = 6
HUD_PROGRESS_BAR = 7
HUD_HUNGER = 8
HUD_AIR_BUBBLES = 9
HUD_HORSE_HEALTH = 10
HUD_EFFECTS = 11
HUD_EXPERIENCE = 12

HUD_ELEMENTS = 13  # 0..12

# SetHudPacket::HudVisibility
HUD_HIDE = 0
HUD_RESET = 1

ALL_ELEMENTS = list(range(HUD_ELEMENTS))


class HudApi:
    """HUD 对外 API，方法名对齐 LSE 的 hud*。"""

    def __init__(self, plugin) -> None:
        self._plugin = plugin
        self._lock = threading.RLock()

    # ── 内部 ──────────────────────────────────────────────────────────────
    def _player(self, name: str):
        try:
            return self._plugin.server.get_player(name)
        except Exception:
            return None

    def _send(self, player, elements: list[int], visibility: int) -> bool:
        try:
            body = self._plugin.core.encode_set_hud(elements, visibility)
            player.send_packet(self._plugin.core.set_hud_packet_id, body)
            return True
        except Exception as exc:
            self._plugin.logger.debug(f"hud 发包失败 ({player.name}): {exc}")
            return False

    # ── 对外 ──────────────────────────────────────────────────────────────
    def set_element(self, player_name: str, element: int, visible: bool) -> bool:
        player = self._player(player_name)
        if player is None:
            return False
        element = int(element)
        if not (0 <= element < HUD_ELEMENTS):
            return False
        if visible:
            return self._send(player, list(ALL_ELEMENTS), HUD_RESET)
        return self._send(player, [e for e in ALL_ELEMENTS if e != element], HUD_HIDE)

    def set_elements(self, player_name: str, mask: int, visible: bool) -> int:
        """按位掩码设置，语义与 LL 版一致：

        - visible=True：发「全部 13 项 + Reset」，返回 13；
        - visible=False：mask 命中的元素保留在列表里（Hide 模式下列表 = 仍可见的），
          返回被隐藏的数量（13 - 命中数）。
        """
        player = self._player(player_name)
        if player is None:
            return 0
        if visible:
            return len(ALL_ELEMENTS) if self._send(player, list(ALL_ELEMENTS), HUD_RESET) else 0
        keep = [e for e in ALL_ELEMENTS if (int(mask) & (1 << e)) != 0]
        if not self._send(player, keep, HUD_HIDE):
            return 0
        return HUD_ELEMENTS - len(keep)

    def set_raw(self, player_name: str, mask: int, visibility: int) -> bool:
        """原始写法：mask 命中什么就发什么，visibility 原样下发（排障用）。"""
        player = self._player(player_name)
        if player is None:
            return False
        elements = [e for e in ALL_ELEMENTS if (int(mask) & (1 << e)) != 0]
        return self._send(player, elements, int(visibility))

    def reset_all(self, player_name: str) -> bool:
        player = self._player(player_name)
        if player is None:
            return False
        return self._send(player, list(ALL_ELEMENTS), HUD_RESET)

    def probe(self) -> str:
        """把「Hide + 保留 12 项」的 body 打出来，便于和抓包对照。"""
        keep = [e for e in ALL_ELEMENTS if e != 0]
        body = self._plugin.core.encode_set_hud(keep, HUD_HIDE)
        return (
            f"bytes={len(body)} body_hex={body.hex()} "
            f"packet_id={self._plugin.core.set_hud_packet_id} (Hide + 保留 12 项，zigzag varint)"
        )
