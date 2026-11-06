"""HoloTestEndstone —— HologramLib endstone 版全量调用测试。

用法（服务端内）：
    /hobotest safe           只跑纯包域（默认，客户端压力小）
    /hobotest all            全部分组（含 HUD / 背包 / 容器 / 交易这些会动客户端界面的）
    /hobotest <group>        跑一组
    /hobotest bind           只解析 444 个方法，不发包
    /hobotest delay <ms>     调调用间隔（默认 20ms）
    /hobotest stop           中止
    /hobotest clean          清理测试造出来的东西

执行顺序：每组内先 create/spawn/open，再 set，再 get/is/count，最后 destroy/close。
`$ID` 取当前域最近一次创建返回的 id，不再固定 1。
标记为 XFAIL 的接口是已知受限实现（见文档「Endstone 版 · 覆盖率」），返回 false 不算失败。
"""

from __future__ import annotations

import time

from endstone.command import CommandSender
from endstone.plugin import Plugin

from .cases import CASES

DEFAULT_DELAY_MS = 20

# 会动客户端界面 / 全屏效果的域：只在 all 或显式点名时跑
HEAVY_GROUPS = {
    "HUD", "FakeInventory", "Container", "Trade", "NpcDialog", "Form",
    "Visibility", "View", "PlayerNpc", "PlayerRespawn",
}

SAFE_GROUPS = [
    "Shape", "Hologram", "GradientLine", "Particle", "BossBar", "Sidebar",
    "LocatorBar", "ItemDetail", "ItemActor", "ItemDisplay", "Entity", "Sulfur",
    "ActorEffect", "Misc", "Wire",
]

# 需要客户端交互才能验证的
SKIP_CASES = {
    "containerPollClicks", "containerClearClicks", "containerDroppedClicks", "containerBlockEvent",
    "fakeInvPollClicks", "fakeInvClearClicks", "fakeInvDroppedClicks",
    "tradePollSettlements", "tradeDroppedSettlements", "tradeIsSettle",
    "npcDialogPollClicks", "npcDialogClearClicks", "npcDialogDroppedClicks",
    "actorPollInteractions", "actorClearInteractions",
    "ghostPollInteractions", "ghostClearInteractions",
    "entityPollSpawns", "entityClearSpawns",
    "playerNpcPollSpawns", "playerNpcClearSpawns",
    "sensingDeviceCode", "sensingDeviceOf", "sensingIsTouch",
    "viewPollSpawnEvents",
}

# 已知受限实现：返回 false/空是设计如此，不算失败
XFAIL_CASES = {
    "viewSelfType", "viewSelfSkin", "viewSelfName", "viewTargetType",
    "viewEntity", "viewClearEntity", "viewClearId",
    "playerNpcCaptureSkin", "playerNpcSetRidePlayer", "entitySetRidePlayer",
    "playerNpcSetImageSize",
}

PRIORITY = (
    ("create", 0), ("spawn", 0), ("add", 0), ("open", 0), ("register", 0), ("acquire", 0),
    ("set", 1), ("play", 1), ("draw", 1), ("emit", 1),
    ("get", 3), ("is", 3), ("has", 3), ("count", 3), ("info", 3), ("find", 3), ("describe", 3),
    ("destroy", 4), ("remove", 4), ("clear", 4), ("close", 4), ("unregister", 4), ("release", 4),
)

DOMAIN_PREFIX = (
    ("playerNpc", "playernpc"), ("playerHide", "visibility"), ("player", "playereffect"),
    ("itemDisplay", "itemdisplay"), ("itemDetail", "itemdetail"), ("itemActor", "itemactor"),
    ("shape", "shape"), ("holo", "hologram"), ("gradient", "gradient"), ("particle", "particle"),
    ("bossBar", "bossbar"), ("sidebar", "sidebar"), ("hud", "hud"),
    ("actor", "actor"), ("locatorBar", "locatorbar"), ("blockFake", "fakeblock"), ("form", "form"),
    ("container", "container"), ("fakeInv", "fakeinv"), ("entity", "entity"), ("trade", "trade"),
    ("npcDialog", "npcdialog"), ("sulfur", "sulfur"), ("view", "view"),
    ("sensing", "sensing"), ("ghost", "ghost"), ("wire", "wire"),
    ("anchor", "anchor"), ("easterEgg", "actor"),
)


def domain_of(label: str) -> str:
    for prefix, domain in DOMAIN_PREFIX:
        if label.startswith(prefix):
            return domain
    return "misc"


def priority_of(method: str) -> int:
    for prefix, rank in PRIORITY:
        if method.startswith(prefix):
            return rank
    return 2



CREATOR = {
    "shape": ("api", "create_text", lambda c: [c["$X"], c["$Y"], c["$Z"], "hobotest"]),
    "hologram": ("holograms", "create", lambda c: [c["$X"], c["$Y"], c["$Z"]]),
    "gradient": ("gradients", "create", lambda c: [c["$X"], c["$Y"], c["$Z"], c["$X"], c["$Y"], c["$Z"], 8]),
    "particle": ("particles", "create_point", lambda c: [c["$PLAYER"], 0, c["$X"], c["$Y"], c["$Z"]]),
    "bossbar": ("bossbar", "add", lambda c: [c["$PLAYER"], "hobotest", 1.0]),
    "locatorbar": ("locatorbar", "set", lambda c: [c["$PLAYER"], "hobotestkey", c["$X"], c["$Y"], c["$Z"]]),
    "itemdisplay": ("itemdisplay", "create", lambda c: [c["$X"], c["$Y"], c["$Z"], 0, "minecraft:diamond"]),
    "itemactor": ("itemactor", "spawn", lambda c: ["minecraft:diamond", 1, c["$X"], c["$Y"], c["$Z"], 0]),
    "entity": ("entities", "create", lambda c: ["minecraft:pig", c["$X"], c["$Y"], c["$Z"], 0]),
    "sulfur": ("sulfur", "create", lambda c: [c["$X"], c["$Y"], c["$Z"], 0]),
    "container": ("container", "open", lambda c: [c["$PLAYER"], 3, "hobotest"]),
    "trade": ("trade", "open", lambda c: [c["$PLAYER"], "Trade", 1, 0]),
    "npcdialog": ("npcdialog", "open", lambda c: [c["$PLAYER"], "hobotest", "scene", "hi"]),
    "playernpc": ("playernpc", "create", lambda c: [c["$X"], c["$Y"], c["$Z"], 0, "hobotest", ""]),
    "fakeinv": ("fakeinv", "apply", lambda c: [c["$PLAYER"], []]),
}

class HoloTestPlugin(Plugin):
    api_version = "0.11"
    name = "HoloTestEndstone"
    version = "1.2.0"
    description = "HologramLib endstone 版全量调用测试（仅本地，不发布）"
    authors = ["HologramLib"]

    commands = {
        "hobotest": {
            "description": "HologramLib endstone 全量调用测试",
            "usages": [
                "/hobotest", "/hobotest safe", "/hobotest all", "/hobotest bind",
                "/hobotest clean", "/hobotest stop", "/hobotest delay <ms>", "/hobotest <group>",
            ],
            "permissions": ["hobotest.command"],
        }
    }

    def on_enable(self) -> None:
        self.register_events(self)
        self.logger.info(f"HoloTest v{self.version} · {sum(len(v) for v in CASES.values())} 条用例 · 间隔 {DEFAULT_DELAY_MS}ms")
        self._lib = None
        self._delay_ms = DEFAULT_DELAY_MS
        self._running = False
        self._lines: list[str] = []
        self._pass = self._fail = self._skip = self._xfail = 0

    # ── 命令 ──────────────────────────────────────────────────────────────
    def on_command(self, sender: CommandSender, command, args) -> bool:
        lib = self._lib_plugin()
        if lib is None:
            sender.send_message("§c[HoloTest] 找不到 HologramLib 插件")
            return True

        action = args[0] if args else "safe"
        if action == "bind":
            self._bind_report(sender, lib)
        elif action == "clean":
            self._clean(sender, lib)
        elif action == "stop":
            self._running = False
            sender.send_message("§e[HoloTest] 已请求中止")
        elif action == "delay":
            if len(args) > 1:
                try:
                    self._delay_ms = max(0, int(args[1]))
                except ValueError:
                    pass
            sender.send_message(f"§e[HoloTest] 间隔 {self._delay_ms}ms")
        elif action == "all":
            self._run(sender, lib, list(CASES))
        elif action == "safe":
            self._run(sender, lib, [g for g in SAFE_GROUPS if g in CASES])
        elif action in CASES:
            self._run(sender, lib, [action])
        else:
            sender.send_message("§e[HoloTest] 分组：§f" + ", ".join(sorted(CASES)))
        return True

    # ── 内部 ──────────────────────────────────────────────────────────────
    def _lib_plugin(self):
        if self._lib is None:
            try:
                self._lib = self.server.plugin_manager.get_plugin("hologramlib")
            except Exception:
                self._lib = None
        return self._lib

    def _first_player(self):
        for player in list(self.server.online_players):
            return player
        return None

    def _bind_report(self, sender, lib) -> None:
        missing = []
        total = 0
        for rows in CASES.values():
            for label, attr, method, _args in rows:
                total += 1
                target = getattr(lib, attr, None)
                if target is None or not hasattr(target, method):
                    missing.append(f"{label}({attr}.{method})")
        sender.send_message(f"§e[HoloTest] 解析 {total} 个方法，缺失 {len(missing)} 个")
        for item in missing[:20]:
            sender.send_message("§c  " + item)

    def _context(self, player) -> dict:
        loc = player.location
        return {
            "$PLAYER": player.name,
            "$X": float(loc.x),
            "$Y": float(loc.y),
            "$Z": float(loc.z),
            "$DIM": 0,
            "$ID": 0,
            "$UID": int(getattr(player, "runtime_id", 0) or 0),
        }

    def _ensure_id(self, lib, ctx, domain: str) -> None:
        spec = CREATOR.get(domain)
        if spec is None:
            return
        attr, method, args_of = spec
        target = getattr(lib, attr, None)
        fn = getattr(target, method, None) if target is not None else None
        if fn is None:
            return
        try:
            value = fn(*args_of(ctx))
        except Exception:
            return
        if isinstance(value, int) and value > 0:
            ctx[f"$ID_{domain}"] = value
            ctx["$ID"] = value

    def _ordered(self, rows):
        return sorted(rows, key=lambda row: (priority_of(row[2]), domain_of(row[0])))

    def _run(self, sender, lib, groups) -> None:
        player = self._first_player()
        if player is None:
            sender.send_message("§c[HoloTest] 需要至少一个在线玩家")
            return

        heavy = [g for g in groups if g in HEAVY_GROUPS]
        if heavy:
            sender.send_message("§e[HoloTest] 含界面域：§f" + ", ".join(heavy) + " §e（客户端会卡一下，属正常）")

        self._running = True
        self._pass = self._fail = self._skip = self._xfail = 0
        self._lines = []
        started = time.time()

        for group in groups:
            if not self._running:
                self._lines.append("NOTE 已手工中止")
                break
            rows = CASES.get(group, [])
            if not rows:
                continue
            ctx = self._context(player)
            wire = getattr(lib, "wire", None)
            if wire is not None:
                wire.clear()
                wire.start(f"hobotest:{group}")

            for label, attr, method, args in self._ordered(rows):
                if not self._running:
                    break
                if label in SKIP_CASES:
                    self._skip += 1
                    continue
                target = getattr(lib, attr, None)
                fn = getattr(target, method, None) if target is not None else None
                if fn is None:
                    self._fail += 1
                    self._lines.append(f"FAIL {label} 方法缺失 {attr}.{method}")
                    continue

                domain = domain_of(label)
                if any(a == "$ID" for a in args) and not ctx.get(f"$ID_{domain}"):
                    self._ensure_id(lib, ctx, domain)

                call_args = []
                for arg in args:
                    if arg == "$ID":
                        call_args.append(ctx.get(f"$ID_{domain}") or ctx["$ID"] or 0)
                    elif isinstance(arg, str) and arg.startswith("$"):
                        call_args.append(ctx.get(arg, arg))
                    else:
                        call_args.append(arg)
                try:
                    value = fn(*call_args)
                except Exception as exc:
                    self._record(label, False, f"异常 {exc}")
                    time.sleep(self._delay_ms / 1000.0)
                    continue

                if isinstance(value, int) and value > 0:
                    ctx[f"$ID_{domain}"] = value
                    ctx["$ID"] = value
                self._record(label, value, None)
                time.sleep(self._delay_ms / 1000.0)

            time.sleep(self._delay_ms / 1000.0)
            if wire is not None:
                self._lines.append(f"NOTE {group} 出站包 {wire.count()}")
                wire.stop()

        elapsed = time.time() - started
        summary = (f"§a[HoloTest] §fPASS={self._pass} §cFAIL={self._fail} "
                   f"§6XFAIL={self._xfail} §eSKIP={self._skip} §7({elapsed:.1f}s)")
        sender.send_message(summary)
        for line in self._lines:
            self.logger.info("[hobotest] " + line)
            if line.startswith(("FAIL", "NOTE")) and len(self._lines) < 80:
                sender.send_message(("§c" if line.startswith("FAIL") else "§7") + line)
        self._lines = []

    def _record(self, label: str, value, note: str | None) -> None:
        bad = value is False or (isinstance(value, int) and value < 0)
        if note is not None:
            bad = True
        if bad and label in XFAIL_CASES:
            self._xfail += 1
            self._lines.append(f"XFAIL {label} 已知受限（返回 {value!r}）")
            return
        if bad:
            self._fail += 1
            self._lines.append(f"FAIL {label} " + (note or f"返回 {value!r}"))
            return
        self._pass += 1

    def _clean(self, sender, lib) -> None:
        cleared = 0
        for attr, method in (
            ("api", "destroy_all"), ("holograms", "destroy_all"), ("gradients", "clear_all"),
            ("particles", "destroy_all"), ("itemdisplay", "destroy_all"), ("itemactor", "destroy_all"),
            ("entities", "destroy_all"), ("sulfur", "destroy_all"), ("container", "close_all"),
            ("trade", "close_all"), ("npcdialog", "close_all"), ("playernpc", "destroy_all"),
        ):
            target = getattr(lib, attr, None)
            fn = getattr(target, method, None) if target is not None else None
            if fn is None:
                continue
            try:
                fn()
                cleared += 1
            except Exception:
                pass
        for player in list(self.server.online_players):
            for attr, method in (("bossbar", "remove_all"), ("locatorbar", "clear"), ("visibility", "restore")):
                target = getattr(lib, attr, None)
                fn = getattr(target, method, None) if target is not None else None
                if fn is None:
                    continue
                try:
                    fn(player.name)
                    cleared += 1
                except Exception:
                    pass
            try:
                lib.hud.reset_all(player.name)
            except Exception:
                pass
        sender.send_message(f"§a[HoloTest] 已清理 {cleared} 处")
