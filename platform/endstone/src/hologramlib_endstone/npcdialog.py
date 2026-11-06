"""NPC 对话（NpcDialog）域 —— LeviLamina 版 NpcDialogueManager 的 endstone 移植（下发子集）。"""

from __future__ import annotations

import json
import threading

from .holograms import _dim_id

ACTION_OPEN = 0
ACTION_CLOSE = 1

DEFAULT_CARRIER = "minecraft:villager_v2"


def build_action_json(buttons_spec: str) -> str:
    buttons = []
    for raw in str(buttons_spec).split(";"):
        entry = raw.strip()
        if not entry:
            continue
        fields = [f.strip() for f in entry.split("|")]
        label = fields[0] if fields else "按钮"
        mode = 0
        commands: list[str] = []
        if len(fields) > 2 and fields[2]:
            try:
                mode = int(fields[2])
            except ValueError:
                mode = 0
        if len(fields) > 3 and fields[3]:
            commands = [c.strip() for c in fields[3].split(",") if c.strip()]
        buttons.append(
            {
                "button_name": label or "按钮",
                "data": [{"cmd_line": c, "cmd_ver": 50} for c in commands],
                "mode": mode,
                "text": "\n".join(commands) + ("\n" if commands else ""),
                "type": 1,
            }
        )
    return json.dumps(buttons, ensure_ascii=False, indent=3)


class _Dialog:
    __slots__ = ("id", "player", "npc_name", "scene_name", "dialogue", "entity_id", "npc_uid", "buttons")

    def __init__(self, dialog_id: int, player: str, npc_name: str, scene_name: str, dialogue: str, entity_id: int, npc_uid: int) -> None:
        self.id = dialog_id
        self.player = player
        self.npc_name = npc_name
        self.scene_name = scene_name
        self.dialogue = dialogue
        self.entity_id = entity_id
        self.npc_uid = npc_uid
        self.buttons = ""


class NpcDialogApi:
    def __init__(self, plugin) -> None:
        self._plugin = plugin
        self._lock = threading.RLock()
        self._dialogs: dict[int, _Dialog] = {}
        self._next_id = 1

    # ── 内部 ──────────────────────────────────────────────────────────────
    def _player(self, name: str):
        try:
            return self._plugin.server.get_player(name)
        except Exception:
            return None

    def _send(self, player, body: bytes) -> bool:
        if not body:
            return False
        try:
            player.send_packet(self._plugin.core.npc_dialogue_packet_id, body)
            return True
        except Exception as exc:
            self._plugin.logger.debug(f"npcdialog 发包失败 ({player.name}): {exc}")
            return False

    def _send_dialog(self, dialog: _Dialog, action: int) -> bool:
        player = self._player(dialog.player)
        if player is None:
            return False
        body = self._plugin.core.encode_npc_dialogue(
            dialog.npc_uid, action, dialog.dialogue, dialog.scene_name, dialog.npc_name, dialog.buttons
        )
        return self._send(player, body)

    # ── 对外 ──────────────────────────────────────────────────────────────
    def open(self, player_name: str, npc_name: str, scene_name: str, dialogue: str, buttons_spec: str = "", carrier_identifier: str = "") -> int:
        player = self._player(player_name)
        if player is None:
            return -1
        loc = player.location
        entity_id = self._plugin.entities.create_advanced(
            carrier_identifier or DEFAULT_CARRIER, loc.x, loc.y - 1.0, loc.z, _dim_id(player),
            0.0, 0.0, 1.0, "", viewers={player_name},
        )
        if entity_id < 0:
            return -1
        self._plugin.entities.set_invisible(entity_id, True)
        self._plugin.entities.set_visible_player(entity_id, player_name)
        npc_uid = self._plugin.entities.unique_id(entity_id)

        with self._lock:
            dialog_id = self._next_id
            self._next_id += 1
            dialog = _Dialog(dialog_id, player_name, str(npc_name), str(scene_name), str(dialogue), entity_id, npc_uid)
            dialog.buttons = build_action_json(buttons_spec)
            self._dialogs[dialog_id] = dialog
        if not self._send_dialog(dialog, ACTION_OPEN):
            self._plugin.entities.destroy(entity_id)
            with self._lock:
                self._dialogs.pop(dialog_id, None)
            return -1
        return dialog_id

    def update(self, dialog_id: int, dialogue: str, buttons_spec: str = "") -> bool:
        with self._lock:
            dialog = self._dialogs.get(dialog_id)
            if dialog is None:
                return False
            dialog.dialogue = str(dialogue)
            dialog.buttons = build_action_json(buttons_spec)
        return self._send_dialog(dialog, ACTION_OPEN)

    def close(self, dialog_id: int) -> bool:
        with self._lock:
            dialog = self._dialogs.pop(dialog_id, None)
        if dialog is None:
            return False
        self._send_dialog(dialog, ACTION_CLOSE)
        self._plugin.entities.destroy(dialog.entity_id)
        return True

    def close_all(self) -> int:
        with self._lock:
            ids = list(self._dialogs.keys())
        for dialog_id in ids:
            self.close(dialog_id)
        return len(ids)

    def get_ids(self) -> list[int]:
        with self._lock:
            return sorted(self._dialogs.keys())

    def is_open(self, dialog_id: int) -> bool:
        with self._lock:
            return dialog_id in self._dialogs

    def set_avatar(self, dialog_id: int, skin_variant: int, avatar_view_spec: str = "") -> bool:
        _ = (skin_variant, avatar_view_spec)
        return False  # 头像需要皮肤数据，未实现

    # 需要接住客户端 NPC 交互回传（未实现）
    def poll_clicks(self) -> list[str]:
        return []

    def clear_clicks(self) -> None:
        return None

    def dropped_clicks(self) -> int:
        return 0
