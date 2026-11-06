"""HologramLib 的 endstone 侧入口。"""

from typing import Iterable

from endstone import Player
from endstone.command import Command, CommandSender
from endstone.event import PacketSendEvent, PlayerJoinEvent, PlayerQuitEvent, event_handler
from endstone.plugin import Plugin

from .actoreffect import ActorEffectApi
from .anchor import AnchorApi
from .bossbar import BossBarApi
from .container import ContainerApi
from .fakeblock import FakeBlockApi
from .entities import EntityApi
from .fakeinv import FakeInventoryApi
from .forms import FormApi
from .gradients import GradientApi
from .itemdetail import ItemDetailApi
from .itemactor import ItemActorApi
from .itemdisplay import ItemDisplayApi
from .playerrespawn import PlayerRespawnApi
from .hud import HudApi
from .holograms import HologramApi
from .particles import ParticleApi
from .playereffect import PlayerEffectApi
from .npcdialog import NpcDialogApi
from .sidebar import SidebarApi
from .playernpc import PlayerNpcApi
from .skins import SkinRegistry
from .sulfur import SulfurApi
from .ghost import GhostApi
from .sensing import SensingApi
from .trade import TradeApi
from .view import ViewApi
from .visibility import VisibilityApi
from .wire import WireApi
from .shape_api import ShapeApi
from .native import NativeCore, ShapeDesc
from .shapes import ShapeScene


class _PacketFilterListener:
    """出站包过滤（actorBlockEvent*）：只在有人登记过滤时才注册，避免每个出站包都进 Python。"""

    def __init__(self, plugin: "HologramLibPlugin") -> None:
        self._plugin = plugin

    @event_handler
    def on_packet_send(self, event: PacketSendEvent) -> None:
        try:
            effects = self._plugin.actor_effects
        except AssertionError:
            return
        if event.packet_id != self._plugin.core.actor_event_packet_id:
            return
        player = event.player
        name = player.name if player is not None else ""
        if effects.blocks.should_drop(bytes(event.payload), name):
            event.cancel()


class _WireTapListener:
    """出站包抓取（wireTrace*）：只在开始 trace 时才注册。"""

    def __init__(self, plugin: "HologramLibPlugin") -> None:
        self._plugin = plugin

    @event_handler
    def on_packet_send(self, event: PacketSendEvent) -> None:
        wire = self._plugin.wire
        player = event.player
        wire.on_packet(int(event.packet_id), len(event.payload), player.name if player is not None else "")


class _ViewFilterListener:
    """视角覆盖：按 runtimeId 取消命中实体的出站包（viewSelfHide / viewEntity hidden）。"""

    def __init__(self, plugin: "HologramLibPlugin") -> None:
        self._plugin = plugin

    @event_handler
    def on_packet_send(self, event: PacketSendEvent) -> None:
        try:
            view = self._plugin.view
        except AssertionError:
            return
        player = event.player
        if player is None:
            return
        if view.should_cancel(int(event.packet_id), bytes(event.payload), player.name):
            event.cancel()


class _PlayerListener:
    """进服补发形状；退服清理该玩家相关状态（当前无 per-player 状态，留钩子）。"""

    def __init__(self, plugin: "HologramLibPlugin") -> None:
        self._plugin = plugin

    @event_handler
    def on_player_join(self, event: PlayerJoinEvent) -> None:
        self._plugin.api.sync_to(event.player)
        self._plugin.holograms.tick()

    @event_handler
    def on_player_quit(self, event: PlayerQuitEvent) -> None:
        self._plugin.api.forget(event.player)
        self._plugin.holograms.forget(event.player.name)
        self._plugin.bossbars.forget(event.player.name)
        self._plugin.sidebar.forget(event.player.name)


HOLO_API_VERSION = "1.35.0"  # 库自身 API 版本，与 LL 版 HOLOGLIB_API_VERSION 对齐


class HologramLibPlugin(Plugin):
    api_version = "0.11"
    name = "HologramLib"
    version = "26.60.2"
    description = "协议层显示库（形状/协议域）—— endstone 版"
    authors = ["伊希娅 EPSILON killcerr"]
    load = "POSTWORLD"

    commands = {
        "holo": {
            "description": "HologramLib 调试命令",
            "usages": ["/holo demo", "/holo clear", "/holo stats"],
            "permissions": ["hologramlib.command.holo"],
        },
    }

    permissions = {
        "hologramlib.command.holo": {
            "description": "允许使用 /holo 调试命令",
            "default": "op",
        },
    }

    def __init__(self) -> None:
        super().__init__()
        self._core: NativeCore | None = None
        self._scene: ShapeScene | None = None
        self._api: ShapeApi | None = None
        self._holograms: HologramApi | None = None
        self._gradients: GradientApi | None = None
        self._particles: ParticleApi | None = None
        self._bossbars: BossBarApi | None = None
        self._sidebar: SidebarApi | None = None
        self._hud: HudApi | None = None
        self._effects: PlayerEffectApi | None = None
        self._fakeblock: FakeBlockApi | None = None
        self._forms: FormApi | None = None
        self._itemdetail: ItemDetailApi | None = None
        self._effects_actor: ActorEffectApi | None = None
        self._locatorbar: LocatorBarApi | None = None
        self._respawn: PlayerRespawnApi | None = None
        self._itemactor: ItemActorApi | None = None
        self._itemdisplay: ItemDisplayApi | None = None
        self._container: ContainerApi | None = None
        self._fakeinv: FakeInventoryApi | None = None
        self._entities: EntityApi | None = None
        self._trade: TradeApi | None = None
        self._npcdialog: NpcDialogApi | None = None
        self._sulfur: SulfurApi | None = None
        self._wire: WireApi | None = None
        self._anchor: AnchorApi | None = None
        self._skins: SkinRegistry | None = None
        self._playernpc: PlayerNpcApi | None = None
        self._view: ViewApi | None = None
        self._sensing: SensingApi | None = None
        self._ghost: GhostApi | None = None
        self._visibility: VisibilityApi | None = None
        self._view_filter_registered = False
        self._wire_tap_registered = False
        self._packet_filter_registered = False
        self._tick_count = 0
        self._task = None
        self._holo_task = None
        self._particle_task = None
        self._particle_task = None

    @property
    def core(self) -> NativeCore:
        assert self._core is not None, "HologramLib 尚未启用"
        return self._core

    @property
    def scene(self) -> ShapeScene:
        assert self._scene is not None, "HologramLib 尚未启用"
        return self._scene

    @property
    def api(self) -> ShapeApi:
        assert self._api is not None, "HologramLib 尚未启用"
        return self._api

    @property
    def holograms(self) -> HologramApi:
        assert self._holograms is not None, "HologramLib 尚未启用"
        return self._holograms

    @property
    def gradients(self) -> GradientApi:
        assert self._gradients is not None, "HologramLib 尚未启用"
        return self._gradients

    @property
    def particles(self) -> ParticleApi:
        assert self._particles is not None, "HologramLib 尚未启用"
        return self._particles

    @property
    def bossbars(self) -> BossBarApi:
        assert self._bossbars is not None, "HologramLib 尚未启用"
        return self._bossbars

    @property
    def sidebar(self) -> SidebarApi:
        assert self._sidebar is not None, "HologramLib 尚未启用"
        return self._sidebar

    @property
    def hud(self) -> HudApi:
        assert self._hud is not None, "HologramLib 尚未启用"
        return self._hud

    @property
    def effects(self) -> PlayerEffectApi:
        assert self._effects is not None, "HologramLib 尚未启用"
        return self._effects

    @property
    def fakeblock(self) -> FakeBlockApi:
        assert self._fakeblock is not None, "HologramLib 尚未启用"
        return self._fakeblock

    @property
    def actor_effects(self) -> ActorEffectApi:
        assert self._effects_actor is not None, "HologramLib 尚未启用"
        return self._effects_actor

    def ensure_packet_filter(self) -> None:
        """懒注册 PacketSendEvent 监听（actorBlockEvent* 第一次调用时）。"""
        if self._packet_filter_registered:
            return
        self.register_events(_PacketFilterListener(self))
        self._packet_filter_registered = True
        if self._effects_actor is not None:
            self._effects_actor.blocks.mark_registered()

    @property
    def visibility(self) -> VisibilityApi:
        assert self._visibility is not None, "HologramLib 尚未启用"
        return self._visibility

    @property
    def ghost(self) -> GhostApi:
        assert self._ghost is not None, "HologramLib 尚未启用"
        return self._ghost

    @property
    def holo_api_version(self) -> str:
        """库 API 版本（对应 LSE 的 holoApiVersion）。"""
        return HOLO_API_VERSION

    @property
    def view(self) -> ViewApi:
        assert self._view is not None, "HologramLib 尚未启用"
        return self._view

    @property
    def sensing(self) -> SensingApi:
        assert self._sensing is not None, "HologramLib 尚未启用"
        return self._sensing

    def ensure_view_filter(self) -> None:
        """懒注册视角覆盖的出站监听。"""
        if self._view_filter_registered:
            return
        self.register_events(_ViewFilterListener(self))
        self._view_filter_registered = True
        if self._view is not None:
            self._view.mark_registered()

    @property
    def playernpc(self) -> PlayerNpcApi:
        assert self._playernpc is not None, "HologramLib 尚未启用"
        return self._playernpc

    @property
    def skins(self) -> SkinRegistry:
        assert self._skins is not None, "HologramLib 尚未启用"
        return self._skins

    @property
    def anchor(self) -> AnchorApi:
        assert self._anchor is not None, "HologramLib 尚未启用"
        return self._anchor

    @property
    def wire(self) -> WireApi:
        assert self._wire is not None, "HologramLib 尚未启用"
        return self._wire

    def ensure_wire_tap(self) -> None:
        """懒注册出站抓包（wireTraceStart 第一次调用时）。"""
        if self._wire_tap_registered:
            return
        self.register_events(_WireTapListener(self))
        self._wire_tap_registered = True
        if self._wire is not None:
            self._wire.mark_registered()

    @property
    def npcdialog(self) -> NpcDialogApi:
        assert self._npcdialog is not None, "HologramLib 尚未启用"
        return self._npcdialog

    @property
    def sulfur(self) -> SulfurApi:
        assert self._sulfur is not None, "HologramLib 尚未启用"
        return self._sulfur

    @property
    def trade(self) -> TradeApi:
        assert self._trade is not None, "HologramLib 尚未启用"
        return self._trade

    @property
    def entities(self) -> EntityApi:
        assert self._entities is not None, "HologramLib 尚未启用"
        return self._entities

    @property
    def fakeinv(self) -> FakeInventoryApi:
        assert self._fakeinv is not None, "HologramLib 尚未启用"
        return self._fakeinv

    @property
    def container(self) -> ContainerApi:
        assert self._container is not None, "HologramLib 尚未启用"
        return self._container

    # 文档与测试里用的短名，跟实际存储名对齐
    @property
    def bossbar(self) -> BossBarApi:
        return self._bossbars

    @property
    def playereffect(self) -> PlayerEffectApi:
        return self._effects

    @property
    def actoreffect(self) -> ActorEffectApi:
        return self._effects_actor

    @property
    def playerrespawn(self) -> PlayerRespawnApi:
        return self._respawn

    @property
    def itemdisplay(self) -> ItemDisplayApi:
        assert self._itemdisplay is not None, "HologramLib 尚未启用"
        return self._itemdisplay

    @property
    def itemactor(self) -> ItemActorApi:
        assert self._itemactor is not None, "HologramLib 尚未启用"
        return self._itemactor

    @property
    def respawn(self) -> PlayerRespawnApi:
        assert self._respawn is not None, "HologramLib 尚未启用"
        return self._respawn

    @property
    def locatorbar(self) -> LocatorBarApi:
        assert self._locatorbar is not None, "HologramLib 尚未启用"
        return self._locatorbar

    @property
    def forms(self) -> FormApi:
        assert self._forms is not None, "HologramLib 尚未启用"
        return self._forms

    @property
    def itemdetail(self) -> ItemDetailApi:
        assert self._itemdetail is not None, "HologramLib 尚未启用"
        return self._itemdetail

    @property
    def tick_count(self) -> int:
        """服务器 tick 计数（endstone 没暴露 current_tick，这里自己数）。"""
        return self._tick_count

    # ── 生命周期 ──────────────────────────────────────────────────────────
    def on_enable(self) -> None:
        self._core = NativeCore()
        self._scene = ShapeScene(self._core)
        self._api = ShapeApi(self)
        self._holograms = HologramApi(self)
        self._gradients = GradientApi(self)
        self._particles = ParticleApi(self)
        self._bossbars = BossBarApi(self)
        self._sidebar = SidebarApi(self)
        self._hud = HudApi(self)
        self._effects = PlayerEffectApi(self)
        self._fakeblock = FakeBlockApi(self)
        self._forms = FormApi(self)
        self._itemdetail = ItemDetailApi(self)
        self._effects_actor = ActorEffectApi(self)
        self._locatorbar = LocatorBarApi(self)
        self._respawn = PlayerRespawnApi(self)
        self._itemactor = ItemActorApi(self)
        self._itemdisplay = ItemDisplayApi(self)
        self._container = ContainerApi(self)
        self._fakeinv = FakeInventoryApi(self)
        self._entities = EntityApi(self)
        self._trade = TradeApi(self)
        self._npcdialog = NpcDialogApi(self)
        self._sulfur = SulfurApi(self)
        self._wire = WireApi(self)
        self._anchor = AnchorApi(self)
        self._skins = SkinRegistry(self)
        self._playernpc = PlayerNpcApi(self)
        self._view = ViewApi(self)
        self._sensing = SensingApi(self)
        self._ghost = GhostApi(self)
        self._visibility = VisibilityApi(self)
        self.register_events(_PlayerListener(self))
        # endstone 的 period 以 tick 计（20/s）
        self._task = self.server.scheduler.run_task(self, self._tick, delay=0, period=20)
        # 悬浮字刷新走 4Hz（与 LL 版 0.5s 节拍一致）
        self._holo_task = self.server.scheduler.run_task(self, self._holo_tick, delay=0, period=5)
        # 粒子按 tick 驱动（形状自己用 interval 节流）
        self._particle_task = self.server.scheduler.run_task(self, self._particle_tick, delay=0, period=1)
        # 运行时自检：假方块解析依赖 server.create_block_data（走 BDS 的方块注册表）
        try:
            stone_id = self._fakeblock.resolve("minecraft:stone")
            self.logger.info(f"假方块解析自检: minecraft:stone -> runtime_id={stone_id}")
        except Exception as exc:
            self.logger.warning(f"假方块解析自检失败: {exc}")

        self.logger.info(
            f"HologramLib 已启用 (endstone 版, core {self._core.version}, "
            f"PrimitiveShapes id={self._core.primitive_shapes_packet_id})"
        )

    def on_disable(self) -> None:
        for task in (self._task, self._holo_task, self._particle_task):
            if task is not None:
                task.cancel()
        self._task = None
        self._holo_task = None
        self._particle_task = None
        if self._playernpc is not None:
            self._playernpc.destroy_all()
        if self._sensing is not None:
            self._sensing.clear()
        if self._npcdialog is not None:
            self._npcdialog.close_all()
        if self._sulfur is not None:
            self._sulfur.destroy_all()
        if self._trade is not None:
            self._trade.close_all()
        if self._entities is not None:
            self._entities.destroy_all()
        if self._fakeinv is not None:
            self._fakeinv.clear_all()
        if self._container is not None:
            self._container.close_all()
        if self._itemdisplay is not None:
            self._itemdisplay.destroy_all()
        if self._itemactor is not None:
            self._itemactor.destroy_all()
        if self._sidebar is not None:
            self._sidebar.remove_all()
        if self._bossbars is not None:
            self._bossbars.destroy_all()
        if self._particles is not None:
            self._particles.destroy_all()
        if self._gradients is not None:
            self._gradients.destroy_all()
        if self._holograms is not None:
            self._holograms.destroy_all()
        if self._api is not None:
            self._api.clear()
        self.logger.info("HologramLib 已停用")

    def _tick(self) -> None:
        """每秒一次（period=20 tick）：处理形状 TTL（后续域在这里挂驱动）。"""
        scene = self._scene
        if scene is None:
            return
        expired: list[int] = []
        for shape_id, desc in scene.items():
            if not desc.has_total_time_left:
                continue
            desc.total_time_left -= 1.0
            if desc.total_time_left <= 0:
                expired.append(shape_id)
        for shape_id in expired:
            self._api.remove(shape_id)

    def _particle_tick(self) -> None:
        """每 tick：粒子形状按自己的 interval 决定是否发。"""
        self._tick_count += 1
        if self._particles is None:
            return
        try:
            self._particles.tick()
        except Exception as exc:
            self.logger.debug(f"particle tick 失败: {exc}")

    def _holo_tick(self) -> None:
        """4Hz：重绘已绘制的悬浮字（逐玩家变量/动态行需要持续刷新）。"""
        if self._holograms is None:
            return
        try:
            self._holograms.tick()
        except Exception as exc:
            self.logger.debug(f"hologram tick 失败: {exc}")
        if self._fakeinv is not None:
            try:
                self._fakeinv.tick()
            except Exception as exc:
                self.logger.debug(f"fakeinv tick 失败: {exc}")
        if self._itemdisplay is not None:
            try:
                self._itemdisplay.tick()
            except Exception as exc:
                self.logger.debug(f"itemdisplay tick 失败: {exc}")
        if self._gradients is not None:
            try:
                self._gradients.tick(0.25)
            except Exception as exc:
                self.logger.debug(f"gradient tick 失败: {exc}")

    # ── 命令 ──────────────────────────────────────────────────────────────
    def on_command(self, sender: CommandSender, command: Command, args: list[str]) -> bool:
        if command.name != "holo":
            return False
        sub = args[0].lower() if args else "stats"
        if sub == "demo":
            base = getattr(sender, "location", None)
            x, y, z = (base.x, base.y + 2, base.z) if base is not None else (0.0, 100.0, 0.0)
            self.api.create_text(x, y, z, "HologramLib / endstone")
            self.api.create_sphere(x, y - 0.5, z, 1.0)
            self.api.create_line(x, y - 1, z, x + 3, y - 1, z)

            holo = self.holograms.create(x, y + 2.0, z)
            self.holograms.add_line(holo, "§bHologramLib on Endstone")
            self.holograms.add_line(holo, "在线 {online} · 你看的是 {player}")
            self.holograms.add_line(holo, "位置 {x}, {y}, {z} · {time}")
            self.holograms.set_line_pool(holo, 2, ["§e动态行 A", "§e动态行 B", "§e动态行 C"], 1000)
            self.holograms.set_depth_test(holo, True)
            self.holograms.draw(holo)

            if hasattr(sender, "name"):
                self.bossbars.add(sender.name, "§6HologramLib / endstone", 0.8, 0, 0)
                self.sidebar.set(
                    sender.name,
                    "§6HologramLib",
                    ["§7形状 §f" + str(self.api.count), "§7悬浮字 §f" + str(len(self.holograms)), "§7粒子 §f" + str(len(self.particles))],
                )
                self.hud.set_element(sender.name, 4, False)  # 隐藏准星做演示
                self.effects.show_effect(sender.name, 1, 200, 0)  # 速度 I
                self.effects.play_celebrate(sender.name, 3.0)

            self.particles.create_point(getattr(sender, "name", "console"), 0, x, y + 3.0, z, "minecraft:endrod", 4, 0)
            self.particles.create_line(getattr(sender, "name", "console"), 0, x - 1, y - 1, z - 1, x + 1, y - 1, z + 1, 1.0,
                                       "minecraft:flame", 2, 0)

            grad = self.gradients.create(x - 2, y - 1.5, z, x + 2, y - 1.5, z, 24)
            self.gradients.set_rainbow(grad, 1.0)
            self.gradients.draw(grad)

            sender.send_message("HologramLib: 已创建 3 个形状 + 1 个悬浮字（3 行，含动态行）+ 1 条彩虹渐变线")
            return True
        if sub == "clear":
            count = self.api.clear()
            sender.send_message(f"HologramLib: 已清理 {count} 个形状")
            return True
        sender.send_message(f"HologramLib: 当前 {self.api.count} 个形状")
        return True
