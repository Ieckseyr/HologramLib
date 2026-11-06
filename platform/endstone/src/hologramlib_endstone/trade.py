"""交易菜单（Trade）域 —— LeviLamina 版 TradeMenuManager 的 endstone 移植（下发子集）。"""

from __future__ import annotations

import threading

from .holograms import _dim_id
from .native import TradeItem, TradeOffer, meta_int

CONTAINER_TYPE_TRADE = 15
TIER_EXP_REQUIREMENTS = [0, 10, 70, 150, 250]
NET_ID_BASE = 3676

ID_TRADE_TIER = 101
ID_MAX_TRADE_TIER = 102
ID_TRADE_EXPERIENCE = 103

DEFAULT_CARRIER = "minecraft:villager_v2"


class _Offer:
    __slots__ = ("buy_a", "buy_b", "sell", "tier", "locked", "max_uses", "uses", "net_id")

    def __init__(self, buy_a: tuple[str, int], buy_b: tuple[str, int], sell: tuple[str, int], tier: int, locked: bool, net_id: int) -> None:
        self.buy_a = buy_a
        self.buy_b = buy_b
        self.sell = sell
        self.tier = int(tier)
        self.locked = bool(locked)
        self.max_uses = 16
        self.uses = 0
        self.net_id = net_id


class _Menu:
    __slots__ = ("id", "player", "trade_type", "tier", "experience", "entity_id", "carrier_uid", "offers", "settle", "container_id")

    def __init__(self, menu_id: int, player: str, trade_type: str, tier: int, experience: int, entity_id: int, carrier_uid: int, container_id: int) -> None:
        self.id = menu_id
        self.player = player
        self.trade_type = trade_type
        self.tier = int(tier)
        self.experience = int(experience)
        self.entity_id = entity_id
        self.carrier_uid = carrier_uid
        self.offers: list[_Offer] = []
        self.settle = False
        self.container_id = container_id


def _clamp_tier(tier: int) -> int:
    return max(1, min(5, int(tier)))


def _wire_tier(tier: int) -> int:
    return _clamp_tier(tier) - 1


class TradeApi:
    FIRST_CONTAINER_ID = 40

    def __init__(self, plugin) -> None:
        self._plugin = plugin
        self._lock = threading.RLock()
        self._menus: dict[int, _Menu] = {}
        self._next_id = 1
        self._next_container = self.FIRST_CONTAINER_ID

    # ── 内部 ──────────────────────────────────────────────────────────────
    def _player(self, name: str):
        try:
            return self._plugin.server.get_player(name)
        except Exception:
            return None

    def _entry(self, spec: tuple[str, int, int, str]) -> TradeItem:
        kind, count, damage, name = spec
        item = TradeItem()
        item.type = kind.encode("utf-8") if kind else None
        item.count = int(count)
        item.damage = int(damage)
        item.name = name.encode("utf-8") if name else None
        item.lore_csv = None
        item.ench_csv = None
        item.is_block = 0
        item.block_version = 18168865
        return item

    def _offers_nbt(self, menu: _Menu, wire_tier: int) -> bytes:
        wire = []
        for index, offer in enumerate(menu.offers):
            entry = TradeOffer()
            entry.buy_a = self._entry(offer.buy_a)
            entry.buy_b = self._entry(offer.buy_b)
            entry.sell = self._entry(offer.sell)
            entry.tier = wire_tier + 1 if offer.locked else _wire_tier(offer.tier)
            entry.max_uses = offer.max_uses
            entry.uses = offer.uses
            entry.net_id = offer.net_id if offer.net_id else NET_ID_BASE + index
            entry.trader_exp = 0
            entry.demand = 0
            entry.reward_exp = 1
            entry.price_multiplier_a = 0.05
            entry.price_multiplier_b = 0.0
            wire.append(entry)
        return self._plugin.core.build_offers(wire, TIER_EXP_REQUIREMENTS, NET_ID_BASE)

    def _send_menu(self, menu: _Menu) -> bool:
        player = self._player(menu.player)
        if player is None:
            return False
        wire_tier = _wire_tier(menu.tier)
        body = self._plugin.core.encode_update_trade(
            menu.container_id, CONTAINER_TYPE_TRADE, 0, wire_tier, menu.carrier_uid, -1,
            menu.trade_type, True, True, self._offers_nbt(menu, wire_tier),
        )
        try:
            player.send_packet(self._plugin.core.update_trade_packet_id, body)
        except Exception as exc:
            self._plugin.logger.debug(f"trade UpdateTrade 失败 ({player.name}): {exc}")
            return False
        entity = self._plugin.entities.get(menu.entity_id)
        if entity is not None:
            exp = self._plugin.core.encode_set_actor_data(
                entity.runtime_id, self._plugin.tick_count,
                [meta_int(ID_TRADE_TIER, wire_tier), meta_int(ID_MAX_TRADE_TIER, 4), meta_int(ID_TRADE_EXPERIENCE, menu.experience)],
            )
            try:
                player.send_packet(self._plugin.core.set_actor_data_packet_id, exp)
            except Exception:
                pass
        return True

    # ── 对外 ──────────────────────────────────────────────────────────────
    def open(self, player_name: str, trade_type: str = "Trade", tier: int = 1, experience: int = 0, real_trades: int = 0, carrier_identifier: str = "") -> int:
        player = self._player(player_name)
        if player is None:
            return -1
        _ = real_trades
        loc = player.location
        identifier = carrier_identifier or DEFAULT_CARRIER
        entity_id = self._plugin.entities.create_advanced(identifier, loc.x, loc.y, loc.z, _dim_id(player), 0.0, 0.0, 1.0, "")
        if entity_id < 0:
            return -1
        self._plugin.entities.set_invisible(entity_id, True)
        carrier_uid = self._plugin.entities.unique_id(entity_id)
        with self._lock:
            menu_id = self._next_id
            self._next_id += 1
            container_id = self._next_container
            self._next_container += 1
            menu = _Menu(menu_id, player_name, str(trade_type), tier, experience, entity_id, carrier_uid, container_id)
            self._menus[menu_id] = menu
        if not self._send_menu(menu):
            self._plugin.entities.destroy(entity_id)
            with self._lock:
                self._menus.pop(menu_id, None)
            return -1
        return menu_id

    def add_offer(self, menu_id: int, buy_a_type: str, buy_a_count: int, buy_b_type: str, buy_b_count: int, sell_type: str, sell_count: int, sell_name: str = "", tier: int = 1, locked: bool = False) -> bool:
        with self._lock:
            menu = self._menus.get(menu_id)
            if menu is None:
                return False
            menu.offers.append(
                _Offer(
                    (str(buy_a_type), max(1, int(buy_a_count)), 0, ""),
                    (str(buy_b_type), max(0, int(buy_b_count)), 0, "") if buy_b_type else ("", 0, 0, ""),
                    (str(sell_type), max(1, int(sell_count)), 0, str(sell_name)),
                    int(tier),
                    bool(locked),
                    NET_ID_BASE + len(menu.offers),
                )
            )
        return self._send_menu(menu)

    def set_tier(self, menu_id: int, tier: int, experience: int = 0) -> bool:
        with self._lock:
            menu = self._menus.get(menu_id)
            if menu is None:
                return False
            menu.tier = int(tier)
            menu.experience = int(experience)
        return self._send_menu(menu)

    def update(self, menu_id: int, trade_type: str = "", tier: int = 0, experience: int = 0) -> bool:
        with self._lock:
            menu = self._menus.get(menu_id)
            if menu is None:
                return False
            if trade_type:
                menu.trade_type = str(trade_type)
            menu.tier = int(tier) if int(tier) > 0 else menu.tier
            menu.experience = int(experience)
        return self._send_menu(menu)

    def close(self, menu_id: int) -> bool:
        with self._lock:
            menu = self._menus.pop(menu_id, None)
        if menu is None:
            return False
        player = self._player(menu.player)
        if player is not None:
            try:
                player.send_packet(
                    self._plugin.core.container_close_packet_id,
                    self._plugin.core.encode_container_close(menu.container_id, CONTAINER_TYPE_TRADE, True),
                )
            except Exception:
                pass
        self._plugin.entities.destroy(menu.entity_id)
        return True

    def close_all(self) -> int:
        with self._lock:
            ids = list(self._menus.keys())
        for menu_id in ids:
            self.close(menu_id)
        return len(ids)

    def get_ids(self) -> list[int]:
        with self._lock:
            return sorted(self._menus.keys())

    def is_open(self, menu_id: int) -> bool:
        with self._lock:
            return menu_id in self._menus

    def set_settle(self, menu_id: int, on: bool) -> bool:
        with self._lock:
            menu = self._menus.get(menu_id)
            if menu is None:
                return False
            menu.settle = bool(on)
            return True

    def is_settle(self, menu_id: int) -> bool:
        with self._lock:
            menu = self._menus.get(menu_id)
            return bool(menu.settle) if menu is not None else False

    # 结算链路需要拦入站交易点击（未实现）
    def poll_settlements(self) -> list[str]:
        return []

    def dropped_settlements(self) -> int:
        return 0
