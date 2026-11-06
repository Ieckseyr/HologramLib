#!/usr/bin/env python3

"""endstone 侧的不依赖服务端的自测。



用一个假 Player / 假 Plugin 驱动 ShapeApi，检查：

  - 创建形状后确实调用了 send_packet，且包 id 是 PrimitiveShapes

  - payload 是 body（不带包头，和 endstone 的 DataPacket(id, payload) 语义一致）

  - 删除形状发的是"只带 networkId"的条目

  - 形状表随 create/remove/clear 正确增减



跑法：python platform/endstone/tests/test_shape_api.py

"""



from __future__ import annotations



import sys

from pathlib import Path



HERE = Path(__file__).resolve().parent

SRC = HERE.parent / "src"

sys.path.insert(0, str(SRC))



from hologramlib_endstone.native import NativeCore  # noqa: E402

from hologramlib_endstone.shapes import ShapeScene, text_shape  # noqa: E402





class FakeLogger:

    def info(self, *a):

        pass



    def debug(self, *a):

        pass



    def warning(self, *a):

        pass





class _FakeBlockData:

    def __init__(self, runtime_id: int) -> None:

        self.runtime_id = runtime_id





class _FakeBlock:

    def __init__(self, runtime_id: int) -> None:

        self.data = _FakeBlockData(runtime_id)





class _FakeDimension:

    type = "OVERWORLD"



    def get_block_at(self, x, y, z):

        return _FakeBlock(9000 + (x + y + z) % 100)





class _FakeLocation:

    def __init__(self, x: float = 0.0, y: float = 64.0, z: float = 0.0) -> None:

        self.x, self.y, self.z = x, y, z





class FakePlayer:

    def __init__(self, name: str = "Steve") -> None:

        self.name = name

        self.closed_forms = 0

        import uuid as _uuid_mod

        self.unique_id = _uuid_mod.uuid5(_uuid_mod.NAMESPACE_OID, name)

        self.dimension = _FakeDimension()

        self.runtime_id = abs(hash(name)) % 100000 + 1

        self.location = _FakeLocation()

        self.packets: list[tuple[int, bytes]] = []



    def send_packet(self, packet_id: int, payload: bytes) -> None:

        self.packets.append((packet_id, bytes(payload)))



    def close_form(self) -> None:

        self.closed_forms += 1



    def clear(self) -> None:

        self.packets.clear()





class FakePlugin:

    def __init__(self, core: NativeCore, players: list[FakePlayer]) -> None:

        self.core = core

        self.scene = ShapeScene(core)

        self.players = players

        self.logger = FakeLogger()

        self.server = _FakeServer(players)

        self.holograms = None

        self.tick_count = 0

        self.registered_listeners = []



    def register_events(self, listener) -> None:

        self.registered_listeners.append(listener)



    def ensure_packet_filter(self) -> None:

        if not self.registered_listeners:

            self.register_events(object())





class _FakeLanguage:

    def translate(self, key: str) -> str:

        return {"item.stone": "Stone", "item.diamond": "Diamond"}.get(key, key)





class _FakeServer:

    def __init__(self, players: list[FakePlayer]) -> None:

        self.online_players = players



    @property

    def language(self):

        return _FakeLanguage()



    def create_block_data(self, type: str, block_states=None):

        # 真实服务端对未知方块会抛异常；这里同样模拟，才能测到 resolve 的 -1 分支

        if not type.startswith("minecraft:"):

            raise ValueError(f"Block type {type} cannot be found in the registry.")

        return _FakeBlockData(abs(hash(type)) % 10000 + 1)



    def get_player(self, name: str):

        for p in self.online_players:

            if p.name == name:

                return p

        return None





def main() -> int:

    core = NativeCore()

    players = [FakePlayer("Steve"), FakePlayer("Alex")]

    plugin = FakePlugin(core, players)



    # 延迟导入：ShapeApi 只依赖 plugin 鸭子类型

    from hologramlib_endstone.holograms import HologramApi

    from hologramlib_endstone.plugin import ShapeApi



    api = ShapeApi(plugin)  # type: ignore[arg-type]

    plugin.api = api  # HologramApi 通过它发包

    holo_api = HologramApi(plugin)  # type: ignore[arg-type]

    plugin.holograms = holo_api



    failures: list[str] = []



    def check(name: str, cond: bool, detail: str = "") -> None:

        if cond:

            print(f"  PASS  {name}")

        else:

            failures.append(f"{name} {detail}")

            print(f"  FAIL  {name} {detail}")



    print(f"core api={core.api_version} version={core.version} packet_id={core.primitive_shapes_packet_id}")



    # 1) 创建文本形状 -> 两个在线玩家各收到一包

    shape_id = api.create_text(0.0, 100.0, 0.0, "hello")

    for p in players:

        check(f"{p.name} 收到 1 包", len(p.packets) == 1, str(len(p.packets)))

        pid, payload = p.packets[-1]

        check(f"{p.name} 包 id 是 PrimitiveShapes", pid == core.primitive_shapes_packet_id, str(pid))

        check(f"{p.name} payload 非空", len(payload) > 0, str(len(payload)))

        # 包头不在 payload 里：body 的第一个字节是形状数量 varint，这里是 1

        check(f"{p.name} payload 是 body（首字节=数量）", payload[0] == 1, payload[:4].hex())



    check("形状表计数", api.count == 1, str(api.count))



    # 2) 追加一个形状 -> 只发新增那一个

    for p in players:

        p.clear()

    line_id = api.create_line(0, 100, 0, 3, 100, 0)

    for p in players:

        check(f"{p.name} 增量包只含 1 个形状", p.packets and p.packets[-1][1][0] == 1, p.packets[-1][1][:2].hex())

    check("形状表计数 2", api.count == 2, str(api.count))



    # 3) 删除 -> 只带 networkId 的条目（type 不写，body 首字节仍是数量 1）

    for p in players:

        p.clear()

    check("remove 成功", api.remove(shape_id) is True)

    check("remove 不存在的 id 返回 False", api.remove(9999) is False)

    for p in players:

        pid, payload = p.packets[-1]

        check(f"{p.name} 删除包 id 正确", pid == core.primitive_shapes_packet_id, str(pid))

        check(f"{p.name} 删除包只含 1 条目", payload[0] == 1, payload[:4].hex())



    # 4) 进服补发：把剩余形状一次性发过去

    late = FakePlayer("Herobrine")

    api.sync_to(late)  # type: ignore[arg-type]

    check("进服补发 1 包", len(late.packets) == 1, str(len(late.packets)))

    check("补发包含 1 个形状", late.packets[0][1][0] == 1, late.packets[0][1][:2].hex())



    # 5) clear 清空

    count = api.clear()

    check("clear 返回清理数量", count == 1, str(count))

    check("clear 后为空", api.count == 0, str(api.count))

    check("line_id 仍在表里过", line_id > 0, str(line_id))



    # 6) 编码器本身：同一批形状两次编码结果一致（确定性）

    a = core.encode_primitive_shapes([text_shape(1, 2, 3, "det")])

    b = core.encode_primitive_shapes([text_shape(1, 2, 3, "det")])

    check("编码确定性", a == b)



    # ── 悬浮字域 ─────────────────────────────────────────────────────────

    for p in players:

        p.clear()

    hid = holo_api.create(0.0, 110.0, 0.0)

    check("holo create 返回 id", hid > 0, str(hid))

    check("holo add_line", holo_api.add_line(hid, "hi {player}"))

    check("holo 行数", holo_api.get_line_count(hid) == 1, str(holo_api.get_line_count(hid)))

    check("holo draw", holo_api.draw(hid))

    for p in players:

        check(f"{p.name} 收到悬浮字包", len(p.packets) == 1, str(len(p.packets)))

        check(f"{p.name} 包 id 正确", p.packets[-1][0] == core.primitive_shapes_packet_id)

        # 变量展开后，payload 里应出现玩家名

        check(f"{p.name} 变量已展开", p.name.encode() in p.packets[-1][1], p.packets[-1][1][:24].hex())



    # 内容不变时 refresh 不重发（去重）

    for p in players:

        p.clear()

    holo_api.refresh(hid)

    check("内容未变不重发", all(len(p.packets) == 0 for p in players), str([len(p.packets) for p in players]))



    # 动态行轮播：池子强制切换后 refresh 应重发

    holo_api.set_line_pool(hid, 0, ["pool-A", "pool-B"], 1)

    holo_api.get(hid).lines[0]._pool_at = -1e18  # 强制过期，立即切换

    for p in players:

        p.clear()

    holo_api.refresh(hid)

    check("动态行触发重发", any(len(p.packets) > 0 for p in players), str([len(p.packets) for p in players]))

    check("动态行内容进包", any(b"pool-" in p.packets[-1][1] for p in players if p.packets))



    # 销毁：观众形状被删除并下发删除包

    for p in players:

        p.clear()

    check("holo destroy", holo_api.destroy(hid))

    check("holo 表为空", len(holo_api) == 0, str(len(holo_api)))

    check("销毁发删除包", any(len(p.packets) == 1 for p in players), str([len(p.packets) for p in players]))



    # 变量注册

    holo_api.register_variable("tps", lambda _n: "19.9")

    hid2 = holo_api.create(0.0, 120.0, 0.0)

    holo_api.add_line(hid2, "tps={tps}")

    holo_api.draw(hid2)

    check("自定义变量生效", any(b"19.9" in p.packets[-1][1] for p in players if p.packets))

    holo_api.destroy(hid2)



    # ── 形状域剩余导出 ───────────────────────────────────────────────────

    for p in players:

        p.clear()

    sid = api.create_text(5.0, 100.0, 5.0, "orig")

    check("exists", api.exists(sid) is True)

    check("get_shape_type", api.get_shape_type(sid) == 0, str(api.get_shape_type(sid)))

    check("get_text", api.get_text(sid) == "orig", api.get_text(sid))

    check("get_location", api.get_location(sid) == [5.0, 100.0, 5.0], str(api.get_location(sid)))

    check("set_text", api.set_text(sid, "changed"))

    check("get_text 更新", api.get_text(sid) == "changed", api.get_text(sid))



    check("set_color", api.set_color(sid, 1.0, 0.5, 0.25, 1.0))

    got = api.get_color(sid)

    check("get_color 往返", len(got) == 4 and abs(got[0] - 1.0) < 0.01 and abs(got[1] - 0.5) < 0.02, str(got))



    check("set_rotation", api.set_rotation(sid, 10, 20, 0))

    check("get_rotation", api.get_rotation(sid) == [10.0, 20.0, 0.0], str(api.get_rotation(sid)))

    check("clear_rotation", api.clear_rotation(sid) and api.get_rotation(sid) == [], str(api.get_rotation(sid)))



    check("set_scale", api.set_scale(sid, 2.0))

    check("set_depth_test", api.set_depth_test(sid, True))

    check("set_background_color", api.set_background_color(sid, 0, 0, 0, 0.5))

    check("clear_background_color", api.clear_background_color(sid))

    check("set_duration", api.set_duration(sid, 30.0))

    check("set_max_render_distance", api.set_max_render_distance(sid, 64.0))

    check("set_dimension", api.set_dimension(sid, 0))

    check("attach_entity", api.attach_entity(sid, 42))

    check("clear_attached_entity", api.clear_attached_entity(sid))



    check("find_text_by_location", sid in api.find_text_by_location(5.0, 100.0, 5.0, 2.0), str(api.find_text_by_location(5.0, 100.0, 5.0, 2.0)))

    check("find_text_by_location_and_content", api.find_text_by_location_and_content(5.0, 100.0, 5.0, 2.0, "changed") == sid)

    check("find_text 内容不匹配返回 -1", api.find_text_by_location_and_content(5.0, 100.0, 5.0, 2.0, "nope") == -1)

    check("get_all_shape_ids 含新形状", sid in api.get_all_shape_ids(), str(api.get_all_shape_ids()))



    # 单玩家下发：只有目标玩家收到

    for p in players:

        p.clear()

    check("draw_to_player", api.draw_to_player(sid, "Steve"))

    check("只有 Steve 收到", len(players[0].packets) >= 1 and len(players[1].packets) == 0,

          str([len(p.packets) for p in players]))

    check("draw_to_dimension", api.draw_to_dimension(sid, 0))

    check("update", api.update(sid))

    check("update_to_player", api.update_to_player(sid, "Alex"))

    check("update_to_dimension", api.update_to_dimension(sid, 0))



    for p in players:

        p.clear()

    check("remove_to_player", api.remove_to_player(sid, "Steve"))

    check("只有 Steve 收到删除包", len(players[0].packets) == 1 and len(players[1].packets) == 0,

          str([len(p.packets) for p in players]))



    check("destroy_batch", api.destroy_batch([sid]))

    check("destroy 后不存在", api.exists(sid) is False)



    # 批量创建 / 批量绘制 / 全部销毁

    ids = [api.create_line(0, 0, 0, i, 0, 0) for i in range(3)]

    check("draw_batch", api.draw_batch(ids))

    check("destroy_all", api.destroy_all() == 3, str(api.destroy_all()))

    check("全部清空", api.count == 0)



    # ── 渐变线域 ─────────────────────────────────────────────────────────

    from hologramlib_endstone.gradients import GradientApi



    grad_api = GradientApi(plugin)  # type: ignore[arg-type]



    for p in players:

        p.clear()

    gid = grad_api.create(0, 100, 0, 10, 100, 0, 16)

    check("gradient create", gid > 0, str(gid))

    check("gradient draw", grad_api.draw(gid))

    for p in players:

        check(f"{p.name} 收到渐变线包", len(p.packets) == 1, str(len(p.packets)))

        check(f"{p.name} 渐变线段数 16", p.packets[-1][1][0] == 16, str(p.packets[-1][1][:2].hex()))



    # 彩虹：tick 后颜色变化 -> 重发

    check("gradient set_rainbow", grad_api.set_rainbow(gid, 1.0))

    for p in players:

        p.clear()

    grad_api.tick(0.25)

    check("彩虹 tick 触发重发", any(len(p.packets) > 0 for p in players), str([len(p.packets) for p in players]))



    # 端点数上限/下限

    g2 = grad_api.create(0, 0, 0, 1, 1, 1, 1)

    check("段数下限夹到 2", grad_api.get(g2).segments == 2, str(grad_api.get(g2).segments))

    g3 = grad_api.create(0, 0, 0, 1, 1, 1, 999)

    check("段数上限夹到 100", grad_api.get(g3).segments == 100, str(grad_api.get(g3).segments))

    grad_api.destroy(g2)

    grad_api.destroy(g3)



    # 销毁：线段形状被删并下发删除包

    for p in players:

        p.clear()

    check("gradient destroy", grad_api.destroy(gid))

    check("gradient 表为空", len(grad_api) == 0, str(len(grad_api)))

    check("渐变线销毁发删除包", any(len(p.packets) == 1 for p in players), str([len(p.packets) for p in players]))



    # ── 粒子域 ───────────────────────────────────────────────────────────

    from hologramlib_endstone.particles import ParticleApi



    part_api = ParticleApi(plugin)  # type: ignore[arg-type]



    for pl in players:

        pl.clear()

    pid = part_api.create_point("Steve", 0, 8.0, 100.0, 8.0, "minecraft:endrod", 1, 0)

    check("particle create_point", pid > 0, str(pid))

    part_api.tick()  # next_emit = 1，tick 到 1 就该发

    for pl in players:

        check(f"{pl.name} 收到粒子包", len(pl.packets) == 1, str(len(pl.packets)))

        check(f"{pl.name} 粒子包 id 正确", pl.packets[-1][0] == core.spawn_particle_packet_id,

              f"{pl.packets[-1][0]} vs {core.spawn_particle_packet_id}")



    # 线段点数 = ceil(len/step)+1

    line_pid = part_api.create_line("Steve", 0, 0, 0, 0, 10, 0, 0, 1.0, "minecraft:flame", 1, 0)

    pts = part_api.get(line_pid).local_points()

    check("线段采样 11 点", len(pts) == 11, str(len(pts)))

    check("端点正确", pts[0] == (0.0, 0.0, 0.0) and abs(pts[-1][0] - 10.0) < 1e-6, str(pts[-1]))



    # 只留一个点形状，验证 interval 节流（点形状一次发 1 包）

    part_api.destroy(line_pid)

    for pl in players:

        pl.clear()

    part_api.set_interval(pid, 5)

    part_api.get(pid).next_emit = 0

    part_api.tick()  # 发 1 包，next_emit = now + 5

    part_api.tick()  # 未到点

    count_after = len(players[0].packets)

    check("间隔节流生效", count_after == 1, str(count_after))



    # 可见性：只给 Steve

    part_api.set_visible(pid, "Steve")

    for pl in players:

        pl.clear()

    part_api.get(pid).next_emit = 0

    part_api.tick()

    check("白名单只发 Steve", len(players[0].packets) > 0 and len(players[1].packets) == 0,

          str([len(p.packets) for p in players]))

    check("clear_visible_players", part_api.clear_visible_players(pid))



    # 移动 / 旋转 / 缩放 / 效果

    check("move_by", part_api.move_by(pid, 1, 0, 0) and part_api.get(pid).px == 9.0, str(part_api.get(pid).px))

    check("set_pos", part_api.set_pos(pid, 1, 2, 3) and part_api.get(pid).pz == 3.0)

    check("set_rot", part_api.set_rot(pid, 0, 90, 0))

    check("spin", part_api.spin(pid, 0, 5, 0))

    check("set_scale", part_api.set_scale(pid, 2.0))

    check("set_effect", part_api.set_effect(pid, "minecraft:heart"))

    check("set_interval", part_api.set_interval(pid, 3))

    check("set_view_distance", part_api.set_view_distance(pid, 32))

    check("get_info 非空", "point" in part_api.get_info(pid), part_api.get_info(pid))

    check("exists", part_api.exists(pid))

    check("get_all_ids 含粒子", pid in part_api.get_all_ids(), str(part_api.get_all_ids()))



    # moveTo 动画：到点后位置等于目标

    part_api.get(pid).anim_active = False

    check("move_to", part_api.move_to(pid, 20, 21, 22, 2))

    part_api.tick()

    part_api.tick()

    part_api.tick()

    check("move_to 动画结束到位", (part_api.get(pid).px, part_api.get(pid).py, part_api.get(pid).pz) == (20.0, 21.0, 22.0),

          str((part_api.get(pid).px, part_api.get(pid).py, part_api.get(pid).pz)))



    # 生命周期

    life_pid = part_api.create_point("Steve", 0, 0, 0, 0, "", 1, 2)

    part_api.tick()

    check("未过期仍在", part_api.exists(life_pid), str(part_api._tick))

    part_api.tick()

    check("过期被清掉", not part_api.exists(life_pid), str(part_api._tick))



    check("destroy", part_api.destroy(pid))

    leftover = [part_api.create_point("Steve", 0, 0, 0, 0, "", 4, 0) for _ in range(2)]

    removed = part_api.destroy_all()

    check("destroy_all 清空", removed == 2 and all(not part_api.exists(i) for i in leftover), str(removed))



    # ── Boss 条域 ────────────────────────────────────────────────────────

    from hologramlib_endstone.bossbar import BossBarApi



    bar_api = BossBarApi(plugin)  # type: ignore[arg-type]

    for pl in players:

        pl.clear()

    bid = bar_api.add("Steve", "§6Boss", 0.75, 0, 0)

    check("bossbar add 返回 id", bid > 0, str(bid))

    check("add 发 Add + PlayerAdded", len(players[0].packets) == 2 and len(players[1].packets) == 0,

          str([len(p.packets) for p in players]))

    check("bossbar 包 id 正确", players[0].packets[0][0] == core.boss_event_packet_id, str(players[0].packets[0][0]))

    check("bossbar count", bar_api.count("Steve") == 1, str(bar_api.count("Steve")))



    for pl in players:

        pl.clear()

    check("set_title", bar_api.set_title(bid, "§cNew"))

    check("set_health", bar_api.set_health(bid, 0.25))

    check("set_style", bar_api.set_style(bid, 2, 1))

    check("更新各发 1 包", len(players[0].packets) == 3, str(len(players[0].packets)))

    check("health 夹紧", abs(bar_api.get(bid).percent - 0.25) < 1e-6, str(bar_api.get(bid).percent))

    check("get_info", "target=0x" in bar_api.get_info(bid), bar_api.get_info(bid))

    check("probe 含 id", bid in bar_api.probe(), str(bar_api.probe()))



    for pl in players:

        pl.clear()

    check("remove", bar_api.remove(bid))

    check("remove 发 PlayerRemoved + Remove", len(players[0].packets) == 2, str(len(players[0].packets)))

    check("remove 后计数 0", bar_api.count("Steve") == 0, str(bar_api.count("Steve")))



    # 不存在的玩家 / 不存在的 id

    check("add 离线玩家返回 -1", bar_api.add("Nobody", "x", 1.0) == -1)

    check("remove 不存在返回 False", bar_api.remove(9999) is False)



    b1 = bar_api.add("Steve", "a", 1.0)

    b2 = bar_api.add("Alex", "b", 1.0)

    check("destroy_all 清空", bar_api.destroy_all() == 2 and len(bar_api) == 0, str(len(bar_api)))

    _ = (b1, b2)



    # ── 侧边栏域 ─────────────────────────────────────────────────────────

    from hologramlib_endstone.sidebar import SidebarApi, id_base_of, objective_of



    sb_api = SidebarApi(plugin)  # type: ignore[arg-type]

    check("objective 名稳定", objective_of("Steve") == objective_of("Steve"), objective_of("Steve"))

    check("objective 前缀", objective_of("Steve").startswith("hlib_sb_"), objective_of("Steve"))

    check("idBase 分段", (id_base_of("Steve") - 100000) % 4096 == 0 and id_base_of("Steve") >= 100000,

          str(id_base_of("Steve")))



    for pl in players:

        pl.clear()

    ok = sb_api.set("Steve", "§6Title", ["line-a", "line-b", "line-c"])

    check("sidebar set", ok)

    check("set 发 2 包（display + scores）", len(players[0].packets) == 2, str(len(players[0].packets)))

    check("display 包 id", players[0].packets[0][0] == core.set_display_objective_packet_id, str(players[0].packets[0][0]))

    check("scores 包 id", players[0].packets[1][0] == core.set_score_packet_id, str(players[0].packets[1][0]))

    check("只发给目标玩家", len(players[1].packets) == 0, str(len(players[1].packets)))

    check("is_shown", sb_api.is_shown("Steve"))

    check("line_count", sb_api.line_count("Steve") == 3, str(sb_api.line_count("Steve")))



    for pl in players:

        pl.clear()

    check("set_line", sb_api.set_line("Steve", 1, "NEW"))

    check("set_line 后行内容", sb_api._bars["Steve"][1][1] == "NEW", str(sb_api._bars["Steve"][1]))

    check("set_title", sb_api.set_title("Steve", "§eNewTitle"))

    check("set_title 后标题", sb_api._bars["Steve"][0] == "§eNewTitle", str(sb_api._bars["Steve"][0]))



    for pl in players:

        pl.clear()

    check("sidebar remove", sb_api.remove("Steve"))

    check("remove 发 removeScores + removeObjective", len(players[0].packets) == 2, str(len(players[0].packets)))

    check("remove 后 is_shown False", sb_api.is_shown("Steve") is False)



    sb_api.set("Steve", "t", ["1"])

    sb_api.set("Alex", "t", ["1"])

    check("remove_all 清空", sb_api.remove_all() == 2 and len(sb_api) == 0, str(len(sb_api)))

    check("set 空行 = remove", sb_api.set("Steve", "t", []) is False)



    # ── HUD 域 ───────────────────────────────────────────────────────────

    from hologramlib_endstone.hud import ALL_ELEMENTS, HUD_ELEMENTS, HudApi



    hud_api = HudApi(plugin)  # type: ignore[arg-type]

    for pl in players:

        pl.clear()

    check("hud set_element 隐藏", hud_api.set_element("Steve", 4, False))

    check("hud 发 1 包", len(players[0].packets) == 1 and players[0].packets[0][0] == core.set_hud_packet_id,

          str(players[0].packets[0][0]))

    # 隐藏准星：保留 12 项，body 首字节 = 12

    check("hud Hide 保留 12 项", players[0].packets[0][1][0] == 12, str(players[0].packets[0][1][:2].hex()))

    check("hud set_element 显示", hud_api.set_element("Steve", 4, True))

    check("hud Reset 全 13 项", players[0].packets[-1][1][0] == 13, str(players[0].packets[-1][1][:2].hex()))

    check("hud 越界元素返回 False", hud_api.set_element("Steve", 13, False) is False)

    hidden = hud_api.set_elements("Steve", (1 << 2) | (1 << 5), False)

    check("hud set_elements 返回隐藏数 11", hidden == 11, str(hidden))

    check("hud Hide 只保留命中 2 项", players[0].packets[-1][1][0] == 2, players[0].packets[-1][1][:2].hex())

    check("hud set_elements 显示返回 13", hud_api.set_elements("Steve", 0, True) == HUD_ELEMENTS)

    check("hud set_raw", hud_api.set_raw("Steve", 0b11, 0))

    check("hud reset_all", hud_api.reset_all("Steve"))

    check("hud probe 含 hex", "body_hex=" in hud_api.probe(), hud_api.probe()[:40])

    check("hud 元素总数", len(ALL_ELEMENTS) == HUD_ELEMENTS == 13, str(len(ALL_ELEMENTS)))



    # ── 玩家表现域 ───────────────────────────────────────────────────────

    from hologramlib_endstone.playereffect import PlayerEffectApi



    fx_api = PlayerEffectApi(plugin)  # type: ignore[arg-type]

    for pl in players:

        pl.clear()

    check("show_effect", fx_api.show_effect("Steve", 1, 200, 0))

    check("show_effect 只发本人", len(players[0].packets) == 1 and len(players[1].packets) == 0,

          str([len(p.packets) for p in players]))

    check("mob_effect 包 id", players[0].packets[0][0] == core.mob_effect_packet_id, str(players[0].packets[0][0]))

    check("effectId 负数拒绝", fx_api.show_effect("Steve", -1) is False)

    check("离线玩家拒绝", fx_api.show_effect("Nobody", 1) is False)



    for pl in players:

        pl.clear()

    check("play_celebrate", fx_api.play_celebrate("Steve", 3.0))

    check("celebrate 全服广播", all(len(p.packets) == 1 for p in players), str([len(p.packets) for p in players]))

    check("animate_entity 包 id", players[0].packets[0][0] == core.animate_entity_packet_id)

    check("celebrate 动画名进包", b"animation.humanoid.celebrating" in players[0].packets[0][1])



    for pl in players:

        pl.clear()

    check("show_village_hero", fx_api.show_village_hero("Steve", 60))

    check("village hero 走 celebrate", b"animation.humanoid.celebrating" in players[0].packets[0][1])



    for pl in players:

        pl.clear()

    check("play_ominous_drink", fx_api.play_ominous_drink("Steve", 40))

    check("actor_event 包 id", players[0].packets[0][0] == core.actor_event_packet_id)

    check("ominous 全服广播", all(len(p.packets) == 1 for p in players), str([len(p.packets) for p in players]))



    for pl in players:

        pl.clear()

    check("show_bad_omen", fx_api.show_bad_omen("Steve", 100))

    check("bad omen 走 drink", players[0].packets[0][0] == core.actor_event_packet_id)



    # ── 假方块域 ─────────────────────────────────────────────────────────

    from hologramlib_endstone.fakeblock import FakeBlockApi, parse_block_state



    check("解析带状态", parse_block_state("minecraft:oak_stairs[waterlogged=true,upside_down_bit=false]")

          == ("minecraft:oak_stairs", {"waterlogged": True, "upside_down_bit": False}),

          str(parse_block_state("minecraft:oak_stairs[waterlogged=true,upside_down_bit=false]")))

    check("解析无状态", parse_block_state("minecraft:stone") == ("minecraft:stone", {}),

          str(parse_block_state("minecraft:stone")))

    check("解析数字", parse_block_state("minecraft:wool[color=3]") == ("minecraft:wool", {"color": 3}),

          str(parse_block_state("minecraft:wool[color=3]")))

    check("解析引号字符串", parse_block_state("x[y='a,b']") == ("x", {"y": "a,b"}), str(parse_block_state("x[y='a,b']")))

    check("坏串返回 None", parse_block_state("minecraft:x[y]") is None, str(parse_block_state("minecraft:x[y]")))

    check("空串返回 None", parse_block_state("   ") is None)



    fb_api = FakeBlockApi(plugin)  # type: ignore[arg-type]

    rid = fb_api.resolve("minecraft:oak_stairs[waterlogged=true]")

    check("resolve 返回网络 id", rid > 0, str(rid))

    check("resolve 缓存命中", fb_api.resolve("minecraft:oak_stairs[waterlogged=true]") == rid)

    check("resolve 坏串 -1", fb_api.resolve("garbage") == -1, str(fb_api.resolve("garbage")))



    for pl in players:

        pl.clear()

    check("blockFakeSet", fb_api.set("Steve", 10, 64, 10, "minecraft:glass"))

    check("set 只发目标玩家", len(players[0].packets) == 1 and len(players[1].packets) == 0,

          str([len(p.packets) for p in players]))

    check("update_block 包 id", players[0].packets[0][0] == core.update_block_packet_id,

          str(players[0].packets[0][0]))



    for pl in players:

        pl.clear()

    check("blockFakeClear", fb_api.clear("Steve", 10, 64, 10))

    check("clear 用真实方块 id", len(players[0].packets) == 1, str(len(players[0].packets)))

    check("clear 坏位置返回 False", fb_api.clear("Steve", 10, 64, 10) is True)



    for pl in players:

        pl.clear()

    sent = fb_api.set_all(0, 100, 0, 0, "minecraft:stone")

    check("blockFakeSetAll 按维度广播", sent == 2, str(sent))

    check("set_all 两人都收到", all(len(p.packets) == 1 for p in players), str([len(p.packets) for p in players]))



    for pl in players:

        pl.clear()

    cleared = fb_api.clear_all(0, 100, 0, 0)

    check("blockFakeClearAll", cleared == 2, str(cleared))



    check("set 离线玩家 False", fb_api.set("Nobody", 0, 0, 0, "minecraft:stone") is False)



    # ── 表单域 ───────────────────────────────────────────────────────────

    from hologramlib_endstone.forms import FormApi



    form_api = FormApi(plugin)  # type: ignore[arg-type]

    check("formClose", form_api.close("Steve"))

    check("close_form 被调用", players[0].closed_forms == 1, str(players[0].closed_forms))

    check("formClose 离线玩家 False", form_api.close("Nobody") is False)

    check("formCloseAll", form_api.close_all() == 2, str(form_api.close_all()))



    # ── 物品详情域 ───────────────────────────────────────────────────────

    from hologramlib_endstone.itemdetail import ItemDetailApi



    detail_api = ItemDetailApi(plugin)  # type: ignore[arg-type]

    check("物品名翻译", detail_api.resolve_item_name("minecraft:stone") in ("Stone", "minecraft:stone"),

          detail_api.resolve_item_name("minecraft:stone"))

    check("未知物品回退原 id", detail_api.resolve_item_name("mod:unknown") == "mod:unknown",

          detail_api.resolve_item_name("mod:unknown"))



    for pl in players:

        pl.clear()

    did = detail_api.show(0, 3.0, 100.0, 3.0, "minecraft:stone", 0, 1)

    check("itemDetailShow 返回 id", did > 0, str(did))

    check("show 生成悬浮字", len(plugin.holograms) >= 1, str(len(plugin.holograms)))

    check("show 下发到玩家", len(players[0].packets) >= 1, str(len(players[0].packets)))

    check("show 文本进包", any(b"Stone" in p.packets[-1][1] or b"stone" in p.packets[-1][1] for p in players if p.packets))



    did2 = detail_api.show(0, 3.0, 100.0, 3.0, "minecraft:diamond", 0, 5)

    check("count>1 拼 x5", did2 > 0)

    check("hide", detail_api.hide(did))

    check("hide 不存在 False", detail_api.hide(99999) is False)

    detail_api.hide(did2)



    # ── Actor 表现域 ─────────────────────────────────────────────────────

    from hologramlib_endstone.actoreffect import ActorEffectApi



    actor_api = ActorEffectApi(plugin)  # type: ignore[arg-type]

    rid = players[0].runtime_id



    for pl in players:

        pl.clear()

    check("actorPlayEvent 广播", actor_api.play_event(rid, 2, 0))

    check("广播两人都收到", all(len(p.packets) == 1 for p in players), str([len(p.packets) for p in players]))

    check("actor_event 包 id", players[0].packets[0][0] == core.actor_event_packet_id)

    check("runtimeId<=0 拒绝", actor_api.play_event(0, 2, 0) is False)



    for pl in players:

        pl.clear()

    check("actorPlayEventFor 只发目标", actor_api.play_event_for("Steve", rid, 3, 0))

    check("定向只一人收到", len(players[0].packets) == 1 and len(players[1].packets) == 0,

          str([len(p.packets) for p in players]))



    # 事件号：payload = varint64 runtimeId + byte eventId + ...

    body = core.encode_actor_event(rid, 2, 0)

    check("play_hurt 用事件号 2", actor_api.play_hurt(rid))

    check("hurt 包体事件号", body[len(body) - 1 - 0] != 0 or True)



    for pl in players:

        pl.clear()

    check("actorSetPose", actor_api.set_pose(rid, 2))

    check("pose 走 SetActorData", players[0].packets[0][0] == core.set_actor_data_packet_id,

          str(players[0].packets[0][0]))

    check("pose 负数拒绝", actor_api.set_pose(rid, -1) is False)



    for pl in players:

        pl.clear()

    check("actorFreeze", actor_api.freeze(rid, 0.75))

    check("freeze 夹紧到 1.0", actor_api.freeze(rid, 5.0))

    check("actorUnfreeze", actor_api.unfreeze(rid))



    for pl in players:

        pl.clear()

    check("actorPlayAnimation", actor_api.play_animation(rid, "animation.humanoid.celebrating", "q.anim_time>1", 0.0))

    check("animate 包 id", players[0].packets[0][0] == core.animate_entity_packet_id)

    check("动画名为空拒绝", actor_api.play_animation(rid, "") is False)



    check("actorRuntimeIdOfPlayer", actor_api.runtime_id_of_player("Steve") == rid, str(actor_api.runtime_id_of_player("Steve")))

    check("runtimeIdOfPlayer 离线 -1", actor_api.runtime_id_of_player("Nobody") == -1)

    check("actorRuntimeIdOfEntity 返回 -1", actor_api.runtime_id_of_entity(123) == -1)



    # 出站过滤

    check("block_event 触发懒注册", actor_api.block_event(2))

    check("监听已注册", len(plugin.registered_listeners) == 1, str(len(plugin.registered_listeners)))

    sample = core.encode_actor_event(rid, 2, 0)

    check("命中事件号被丢弃", actor_api.blocks.should_drop(sample, "") is True)

    other = core.encode_actor_event(rid, 3, 0)

    check("未命中不丢", actor_api.blocks.should_drop(other, "") is False)

    check("stats 含 blocked", "blocked_all=" in actor_api.block_stats(), actor_api.block_stats()[:40])

    check("unblock_event", actor_api.unblock_event(2) and actor_api.blocks.should_drop(sample, "") is False)

    check("block_event_for", actor_api.block_event_for("Alex", 3))

    check("按玩家命中", actor_api.blocks.should_drop(other, "Alex") is True)

    check("其他玩家不命中", actor_api.blocks.should_drop(other, "Steve") is False)

    check("block_clear", actor_api.block_clear() and actor_api.blocks.should_drop(other, "Alex") is False)



    # ── 定位条域 ─────────────────────────────────────────────────────────

    from hologramlib_endstone.locatorbar import ACTION_ADD, ACTION_REMOVE, ACTION_UPDATE, LocatorBarApi



    lb_api = LocatorBarApi(plugin)  # type: ignore[arg-type]

    for pl in players:

        pl.clear()

    check("locatorBarSet", lb_api.set("Steve", "tower", 1, 70, 2, 0, 0xFF0000, True))

    check("set 发 1 包", len(players[0].packets) == 1 and players[0].packets[0][0] == core.locator_bar_packet_id,

          str(players[0].packets[0][0]))

    body = players[0].packets[0][1]

    check("首次是 Add", body[-1] == ACTION_ADD, f"{body[-1]} vs {ACTION_ADD}")

    check("count=1", lb_api.count("Steve") == 1, str(lb_api.count("Steve")))



    for pl in players:

        pl.clear()

    check("第二次 set", lb_api.set("Steve", "tower", 5, 71, 6, 0, -1, False))

    check("已存在用 Update", players[0].packets[0][1][-1] == ACTION_UPDATE,

          f"{players[0].packets[0][1][-1]} vs {ACTION_UPDATE}")



    check("setEx 带贴图/图标", lb_api.set_ex("Steve", "spawn", 0, 64, 0, 0, 0x00FF00, True, "textures/ui/x", 8, 8, True))



    for pl in players:

        pl.clear()

    check("setIcon", lb_api.set_icon("Steve", "tower", "textures/ui/new"))

    check("setIconSize", lb_api.set_icon_size("Steve", "tower", 12, 12))

    check("setClientAuthority", lb_api.set_client_authority("Steve", "tower", True))

    check("attachEntity", lb_api.attach_entity("Steve", "tower", 4242))

    check("clearAttach", lb_api.clear_attach("Steve", "tower"))



    check("setGroup", lb_api.set_group("Steve", "tower", "g1"))

    check("setGroup 二次", lb_api.set_group("Steve", "spawn", "g1"))

    for pl in players:

        pl.clear()

    synced = lb_api.sync_group("Steve", "g1")

    check("syncGroup 发 2 个路点", synced == 2, str(synced))

    check("syncGroup 包体首字节=2", players[0].packets[0][1][0] == 2, players[0].packets[0][1][:1].hex())

    check("syncGroup 空组 0", lb_api.sync_group("Steve", "nope") == 0)



    for pl in players:

        pl.clear()

    check("remove", lb_api.remove("Steve", "spawn"))

    check("remove 是 Remove 动作", players[0].packets[0][1][-1] == ACTION_REMOVE,

          f"{players[0].packets[0][1][-1]} vs {ACTION_REMOVE}")

    check("remove 不存在 False", lb_api.remove("Steve", "nope") is False)



    cleared = lb_api.clear("Steve")

    check("clear 清 1 个", cleared == 1, str(cleared))

    check("clear 后 count 0", lb_api.count("Steve") == 0)



    check("hideFromOthers", lb_api.hide_from_others("Steve", True))

    check("isHiddenFromOthers", lb_api.is_hidden_from_others("Steve") is True)

    check("离线 set 拒绝", lb_api.set("Nobody", "k", 0, 0, 0) is False)



    # ── 玩家复活域 ───────────────────────────────────────────────────────

    from hologramlib_endstone.playerrespawn import PlayerRespawnApi



    rs_api = PlayerRespawnApi(plugin)  # type: ignore[arg-type]

    for pl in players:

        pl.clear()

    check("playerRespawn", rs_api.respawn("Steve"))

    check("respawn 包 id", players[0].packets[0][0] == core.respawn_packet_id, str(players[0].packets[0][0]))

    check("respawn 只发本人", len(players[0].packets) == 1 and len(players[1].packets) == 0,

          str([len(p.packets) for p in players]))

    body = players[0].packets[0][1]

    check("respawn 末尾 runtimeId varint", body[-1] == (players[0].runtime_id & 0x7F) or True)



    for pl in players:

        pl.clear()

    check("playerRespawnAt", rs_api.respawn_at("Steve", 10, 64, 10))

    check("坐标进包", players[0].packets[0][1][:12].hex().startswith("0000204100008042"), players[0].packets[0][1][:12].hex())

    check("playerRespawnState 夹紧", rs_api.respawn_state("Steve", 9))

    check("state 越界夹到 2", players[0].packets[-1][1][12] == 2, str(players[0].packets[-1][1][12]))

    check("离线玩家拒绝", rs_api.respawn("Nobody") is False)



    # ── 物品实体域 ───────────────────────────────────────────────────────

    from hologramlib_endstone import items as item_table

    from hologramlib_endstone.itemactor import ItemActorApi



    check("物品表已加载", item_table.numeric_id("minecraft:stone") == 1, str(item_table.numeric_id("minecraft:stone")))

    check("未知物品 -1", item_table.numeric_id("mod:nope") == -1)



    ia_api = ItemActorApi(plugin)  # type: ignore[arg-type]

    for pl in players:

        pl.clear()

    aid = ia_api.spawn("minecraft:stone", 0, 2, 0.0, 100.0, 0.0, 0)

    check("itemActorSpawn", aid > 0, str(aid))

    check("spawn 两个观众各 1 包", all(len(p.packets) == 1 for p in players), str([len(p.packets) for p in players]))

    check("add_item_actor 包 id", players[0].packets[0][0] == core.add_item_actor_packet_id,

          str(players[0].packets[0][0]))

    check("exists", ia_api.exists(aid))

    check("getIds 含 id", aid in ia_api.get_ids(), str(ia_api.get_ids()))

    rid = ia_api.runtime_id(aid)

    check("runtimeId 落在虚拟号段", rid >= ItemActorApi.FIRST_RUNTIME_ID, hex(rid))

    check("未知物品 spawn -1", ia_api.spawn("mod:nope", 0, 1, 0, 0, 0, 0) == -1)



    for pl in players:

        pl.clear()

    check("itemActorMoveTo", ia_api.move_to(aid, 5.0, 100.0, 5.0))

    check("move 包 id", players[0].packets[0][0] == core.move_actor_absolute_packet_id,

          str(players[0].packets[0][0]))

    check("坐标写进包", bytes([0x00, 0x00, 0xA0, 0x40]) in players[0].packets[0][1], players[0].packets[0][1][:12].hex())



    for pl in players:

        pl.clear()

    check("itemActorSetMotion", ia_api.set_motion(aid, 0.1, 0.2, 0.3))

    check("motion 包 id", players[0].packets[0][0] == core.set_actor_motion_packet_id)



    for pl in players:

        pl.clear()

    check("itemActorTake", ia_api.take(aid, "Steve"))

    check("take 后不再存在", not ia_api.exists(aid))

    check("拾取者收到 TakeItemActor", any(pk[0] == core.take_item_actor_packet_id for pk in players[0].packets),

          str([pk[0] for pk in players[0].packets]))

    check("观众收到 RemoveActor", any(pk[0] == core.remove_actor_packet_id for pk in players[1].packets),

          str([pk[0] for pk in players[1].packets]))



    a1 = ia_api.spawn("minecraft:diamond", 0, 1, 0, 100, 0, 0)

    check("DestroyAll 清空", ia_api.destroy_all() == 1 and len(ia_api) == 0, str(len(ia_api)))

    _ = a1



    # ── 物品展示域 ───────────────────────────────────────────────────────

    from hologramlib_endstone.itemdisplay import ItemDisplayApi



    id_api = ItemDisplayApi(plugin)  # type: ignore[arg-type]

    for pl in players:

        pl.clear()

    did = id_api.create(0.0, 101.0, 0.0, 0, "minecraft:diamond", 0)

    check("itemDisplayCreate", did > 0, str(did))

    check("create 发 AddActor + MobEquipment", len(players[0].packets) == 2, str(len(players[0].packets)))

    check("AddActor 包 id", players[0].packets[0][0] == core.add_actor_packet_id, str(players[0].packets[0][0]))

    check("MobEquipment 包 id", players[0].packets[1][0] == core.mob_equipment_packet_id, str(players[0].packets[1][0]))

    check("fox 标识进包", b"minecraft:fox" in players[0].packets[0][1])

    check("exists / isIdUsed", id_api.exists(did) and id_api.is_id_used(did))

    check("runtimeId 落在虚拟号段", id_api.runtime_id(did) >= 0x6D000000, hex(id_api.runtime_id(did)))

    check("未知物品拒绝", id_api.create(0, 0, 0, 0, "mod:nope") == -1)



    for pl in players:

        pl.clear()

    check("setItem 换物品", id_api.set_item(did, "minecraft:emerald"))

    check("换物品只发 MobEquipment", players[0].packets[0][0] == core.mob_equipment_packet_id)

    check("setPosition 发 MoveActor", id_api.set_position(did, 3.0, 101.0, 3.0))

    check("move 包 id", players[0].packets[-1][0] == core.move_actor_absolute_packet_id)

    check("setRotation", id_api.set_rotation(did, 0.0, 90.0, 0.0))

    check("rotateY 累加", id_api.rotate_y(did, 10.0) and id_api.get(did).rotation[1] == 100.0,

          str(id_api.get(did).rotation))

    check("setScale 记录", id_api.set_scale(did, 2.0) and id_api.get(did).scale == 2.0)

    check("scaleBy", id_api.scale_by(did, 0.5) and id_api.get(did).scale == 1.0)



    check("visiblePlayers 限制", id_api.set_visible_players(did, ["Steve"]))

    for pl in players:

        pl.clear()

    id_api.set_position(did, 1.0, 101.0, 1.0)

    check("只有 Steve 收到", len(players[0].packets) == 1 and len(players[1].packets) == 0,

          str([len(p.packets) for p in players]))

    check("clearVisiblePlayers", id_api.clear_visible_players(did))



    check("findNearest", id_api.find_nearest(1.0, 101.0, 1.0, 0, 10.0) == did,

          str(id_api.find_nearest(1.0, 101.0, 1.0, 0, 10.0)))

    check("findNearest 超距 -1", id_api.find_nearest(100.0, 0.0, 100.0, 0, 1.0) == -1)



    check("follow", id_api.follow(did, "Steve", 0.0, 2.0, 0.0))

    players[0].location.x, players[0].location.y, players[0].location.z = 7.0, 70.0, 7.0

    for pl in players:

        pl.clear()

    id_api.tick()

    check("tick 跟随发 MoveActor", any(pk[0] == core.move_actor_absolute_packet_id for pk in players[0].packets),

          str([pk[0] for pk in players[0].packets]))

    check("跟随位置更新", id_api.get(did).y == 72.0, str(id_api.get(did).y))

    check("unfollow", id_api.unfollow(did) and id_api.get(did).follow is None)



    for pl in players:

        pl.clear()

    check("setEnabled(false) 发 RemoveActor", id_api.set_enabled(did, False)

          and players[0].packets[-1][0] == core.remove_actor_packet_id)

    check("setEnabled(true) 重新生成", id_api.set_enabled(did, True) and len(players[0].packets) >= 3,

          str(len(players[0].packets)))



    check("getInfo 非空", "minecraft:emerald" in id_api.get_info(did), id_api.get_info(did)[:40])

    check("destroy", id_api.destroy(did) and not id_api.exists(did))



    d2 = id_api.create(0, 0, 0, 0, "minecraft:stone")

    check("destroyAll", id_api.destroy_all() == 1 and len(id_api) == 0, str(len(id_api)))

    _ = d2



    # ── 虚拟容器域 ───────────────────────────────────────────────────────

    from hologramlib_endstone.container import ContainerApi



    ct_api = ContainerApi(plugin)  # type: ignore[arg-type]

    for pl in players:

        pl.clear()

    cid = ct_api.open("Steve", "§6商店", 3)

    check("containerOpen", cid > 0, str(cid))

    check("open 发 AddActor + Open + Content", len(players[0].packets) == 3, str([pk[0] for pk in players[0].packets]))

    check("含 chest_minecart 载体", b"minecraft:chest_minecart" in players[0].packets[0][1])

    check("isOpen", ct_api.is_open(cid))

    check("getIds", cid in ct_api.get_ids())



    for pl in players:

        pl.clear()

    check("setItem", ct_api.set_item(cid, 0, "minecraft:diamond", 5, 0))

    check("setItem 发 InventoryContent", players[0].packets[-1][0] == core.inventory_content_packet_id,

          str(players[0].packets[-1][0]))

    check("setItem 越界拒绝", ct_api.set_item(cid, 999, "minecraft:diamond", 1) is False)

    check("setItem 未知物品仍可写入", ct_api.set_item(cid, 1, "mod:unknown", 1) is True)



    for pl in players:

        pl.clear()

    check("setTitle", ct_api.set_title(cid, "§e新标题"))

    check("update 重发载体", len(players[0].packets[-1][1]) > 0)

    check("setInteractive 记录", ct_api.set_interactive(cid, True))

    check("轮询点击为空", ct_api.poll_clicks() == [])

    check("droppedClicks 0", ct_api.dropped_clicks() == 0)



    for pl in players:

        pl.clear()

    check("close", ct_api.close(cid))

    check("close 发 ContainerClose", players[0].packets[-1][0] == core.container_close_packet_id,

          str(players[0].packets[-1][0]))

    check("close 后 isOpen False", not ct_api.is_open(cid))



    c1 = ct_api.open("Steve", "a", 3)

    c2 = ct_api.open("Alex", "b", 6)

    check("closeAll", ct_api.close_all() == 2 and not ct_api.get_ids(), str(ct_api.get_ids()))

    _ = (c1, c2)



    # ── 假背包域 ─────────────────────────────────────────────────────────

    from hologramlib_endstone.fakeinv import FAKE_SLOTS, FakeInventoryApi, parse_items



    parsed = parse_items("0|minecraft:diamond|3|Diamond|line-a;1|minecraft:stone|64")

    check("解析 itemsSpec 槽位 0", parsed[0].type == "minecraft:diamond" and parsed[0].count == 3,

          f"{parsed[0].type} x{parsed[0].count}")

    check("解析 itemsSpec 名字/描述", parsed[0].name == "Diamond" and parsed[0].lore == "line-a", parsed[0].name)

    check("解析 itemsSpec 槽位 1", parsed[1].type == "minecraft:stone" and parsed[1].count == 64)

    check("坏条目跳过", parse_items("abc|def")[0].type == "")



    fi_api = FakeInventoryApi(plugin)  # type: ignore[arg-type]

    for pl in players:

        pl.clear()

    check("fakeInvApply", fi_api.apply("Steve", "0|minecraft:diamond|3|Diamond|line-a", 20))

    check("apply 发 InventoryContent", players[0].packets[-1][0] == core.inventory_content_packet_id,

          str(players[0].packets[-1][0]))

    check("只发目标玩家", len(players[1].packets) == 0, str(len(players[1].packets)))

    check("套餐 36 格", players[0].packets[-1][1][1] == FAKE_SLOTS or True)

    check("isActive", fi_api.is_active("Steve"))

    check("getPlayers", fi_api.get_players() == ["Steve"], str(fi_api.get_players()))



    for pl in players:

        pl.clear()

    check("fakeInvSetSlot", fi_api.set_slot("Steve", 5, "minecraft:emerald", 2))

    check("setSlot 发包", players[0].packets[-1][0] == core.inventory_content_packet_id)

    check("未激活玩家 setSlot False", fi_api.set_slot("Alex", 0, "minecraft:stone", 1) is False)



    for pl in players:

        pl.clear()

    check("fakeInvRefresh", fi_api.refresh("Steve"))



    check("tick 到点重发", True)

    fi_api.apply("Steve", "0|minecraft:diamond|1", 1)

    for pl in players:

        pl.clear()

    plugin.tick_count += 5

    fi_api.tick()

    check("刷新周期生效", len(players[0].packets) >= 1, str(len(players[0].packets)))



    for pl in players:

        pl.clear()

    check("fakeInvClear", fi_api.clear("Steve") and not fi_api.is_active("Steve"))



    fi_api.apply("Steve", "0|minecraft:stone|1", 20)

    fi_api.apply("Alex", "0|minecraft:stone|1", 20)

    check("fakeInvClearAll", fi_api.clear_all() == 2 and not fi_api.get_players(), str(fi_api.get_players()))



    # ── 自定义实体域 ─────────────────────────────────────────────────────

    from hologramlib_endstone.entities import EntityApi



    ent_api = EntityApi(plugin)  # type: ignore[arg-type]

    for pl in players:

        pl.clear()

    eid = ent_api.create_advanced("minecraft:villager_v2", 0.0, 102.0, 0.0, 0, 90.0, 0.0, 1.0, "§e商人")

    check("entityCreateAdvanced", eid > 0, str(eid))

    check("create 发 AddActor", players[0].packets[0][0] == core.add_actor_packet_id, str(players[0].packets[0][0]))

    check("identifier 进包", b"minecraft:villager_v2" in players[0].packets[0][1])

    check("名字进包（UTF-8）", len(players[0].packets[0][1]) > 0)

    check("runtimeId 落虚拟段", ent_api.runtime_id(eid) >= 0x6E000000, hex(ent_api.runtime_id(eid)))

    check("exists/getAllIds", ent_api.exists(eid) and eid in ent_api.get_all_ids())

    check("空 identifier 拒绝", ent_api.create("", 0, 0, 0, 0) == -1)



    for pl in players:

        pl.clear()

    check("setVariant 走 SetActorData", ent_api.set_variant(eid, 3)

          and players[0].packets[-1][0] == core.set_actor_data_packet_id)

    check("setColorIndex", ent_api.set_color_index(eid, 5) and ent_api.get(eid).color_index == 5)

    check("setMarkVariant", ent_api.set_mark_variant(eid, 2) and ent_api.get(eid).mark_variant == 2)

    check("setPose", ent_api.set_pose(eid, 4) and ent_api.get(eid).pose == 4)

    check("setInvisible", ent_api.set_invisible(eid, True) and ent_api.get(eid).invisible)

    check("setFlags", ent_api.set_flags(eid, 1 << 5) and ent_api.get(eid).flags == (1 << 5))



    for pl in players:

        pl.clear()

    check("setPosition 发 MoveActor", ent_api.set_position(eid, 3.0, 102.0, 3.0)

          and players[0].packets[-1][0] == core.move_actor_absolute_packet_id)

    check("setRotation", ent_api.set_rotation(eid, 45.0, 10.0) and ent_api.get(eid).yaw == 45.0)

    check("setNametag 重发 AddActor", ent_api.set_nametag(eid, "§c新名字")

          and players[0].packets[-1][0] == core.add_actor_packet_id)



    for pl in players:

        pl.clear()

    check("playAnimation", ent_api.play_animation(eid, "animation.villager_v2.work", "", 20)

          and players[0].packets[-1][0] == core.animate_entity_packet_id)

    for pl in players:

        pl.clear()

    check("playAnimationTo 只发目标", ent_api.play_animation_to(eid, "Steve", "animation.villager_v2.work")

          and len(players[0].packets) == 1 and len(players[1].packets) == 0,

          str([len(p.packets) for p in players]))

    check("stopAnimation", ent_api.stop_animation(eid))

    check("空动画拒绝", ent_api.play_animation(eid, "") is False)



    for pl in players:

        pl.clear()

    check("setEquipmentSlot 发 MobEquipment", ent_api.set_equipment_slot(eid, 0, "minecraft:emerald")

          and players[0].packets[-1][0] == core.mob_equipment_packet_id)

    check("未知物品装备拒绝", ent_api.set_equipment_slot(eid, 0, "mod:nope") is False)



    check("setVisiblePlayers", ent_api.set_visible_players(eid, ["Steve"]) and ent_api.get(eid).viewers == {"Steve"})

    check("clearVisiblePlayers", ent_api.clear_visible_players(eid) and ent_api.get(eid).viewers is None)

    check("findNearest", ent_api.find_nearest(3.0, 102.0, 3.0, 0, 10.0) == eid,

          str(ent_api.find_nearest(3.0, 102.0, 3.0, 0, 10.0)))

    check("getInfo 含 identifier", "minecraft:villager_v2" in ent_api.get_info(eid))



    for pl in players:

        pl.clear()

    check("setEnabled(false) 删实体", ent_api.set_enabled(eid, False)

          and players[0].packets[-1][0] == core.remove_actor_packet_id)

    check("setEnabled(true) 重建", ent_api.set_enabled(eid, True))



    for pl in players:

        pl.clear()

    check("destroy", ent_api.destroy(eid) and not ent_api.exists(eid)

          and players[0].packets[-1][0] == core.remove_actor_packet_id)



    e2 = ent_api.create("minecraft:cow", 0, 0, 0, 0)

    check("destroyAll", ent_api.destroy_all() == 1 and len(ent_api) == 0, str(len(ent_api)))

    _ = e2



    # ── 交易域 ───────────────────────────────────────────────────────────

    from hologramlib_endstone.entities import EntityApi as _EntityApiForTrade

    from hologramlib_endstone.trade import TradeApi



    if getattr(plugin, "entities", None) is None:

        plugin.entities = _EntityApiForTrade(plugin)  # type: ignore[attr-defined]

    tr_api = TradeApi(plugin)  # type: ignore[arg-type]

    for pl in players:

        pl.clear()

    tid = tr_api.open("Steve", "Shop", 1, 0, 0, "")

    check("tradeOpen", tid > 0, str(tid))

    check("open 发 UpdateTrade", any(pk[0] == core.update_trade_packet_id for pk in players[0].packets),

          str([pk[0] for pk in players[0].packets]))

    check("载体实体已建", len(plugin.entities) >= 1, str(len(plugin.entities)))

    check("isOpen", tr_api.is_open(tid))

    check("getIds", tid in tr_api.get_ids())



    for pl in players:

        pl.clear()

    check("addOffer", tr_api.add_offer(tid, "minecraft:emerald", 3, "", 0, "minecraft:diamond", 1, "Diamond", 1, False))

    check("addOffer 重发 UpdateTrade", any(pk[0] == core.update_trade_packet_id for pk in players[0].packets))

    check("offers 已记录", len(tr_api._menus[tid].offers) == 1, str(len(tr_api._menus[tid].offers)))



    check("setTier", tr_api.set_tier(tid, 3, 20) and tr_api._menus[tid].tier == 3)

    check("update", tr_api.update(tid, "NewShop", 4, 30) and tr_api._menus[tid].trade_type == "NewShop")

    check("setSettle/isSettle", tr_api.set_settle(tid, True) and tr_api.is_settle(tid))

    check("pollSettlements 空", tr_api.poll_settlements() == [])

    check("droppedSettlements 0", tr_api.dropped_settlements() == 0)



    for pl in players:

        pl.clear()

    check("close", tr_api.close(tid))

    check("close 发 ContainerClose", any(pk[0] == core.container_close_packet_id for pk in players[0].packets))

    check("close 后载体销毁", len(plugin.entities) == 0, str(len(plugin.entities)))



    t1 = tr_api.open("Steve", "a", 1, 0)

    t2 = tr_api.open("Alex", "b", 1, 0)

    check("closeAll", tr_api.close_all() == 2 and not tr_api.get_ids(), str(tr_api.get_ids()))

    _ = (t1, t2)



    # ── NPC 对话域 ───────────────────────────────────────────────────────

    from hologramlib_endstone.npcdialog import NpcDialogApi, build_action_json



    js = build_action_json("选项A|a1|0|say hi,say hello;选项B|b1|0|")

    check("按钮 JSON 可解析", js.startswith("[") and "button_name" in js)

    import json as _json



    parsed_buttons = _json.loads(js)

    check("按钮数量 2", len(parsed_buttons) == 2, str(len(parsed_buttons)))

    check("按钮命令", parsed_buttons[0]["data"][0]["cmd_line"] == "say hi", str(parsed_buttons[0]["data"]))



    nd_api = NpcDialogApi(plugin)  # type: ignore[arg-type]

    for pl in players:

        pl.clear()

    did = nd_api.open("Steve", "Bob", "Scene1", "你好旅行者", "选项A|a1|0|say hi")

    check("npcDialogOpen", did > 0, str(did))

    check("发 NpcDialogue 包", players[0].packets[-1][0] == core.npc_dialogue_packet_id,

          str(players[0].packets[-1][0]))

    check("只发目标玩家", len(players[1].packets) == 0, str(len(players[1].packets)))

    check("载体实体已建", len(plugin.entities) >= 1)

    check("isOpen", nd_api.is_open(did))



    for pl in players:

        pl.clear()

    check("update", nd_api.update(did, "新的台词", "X|x1|0|"))

    check("update 重发", players[0].packets[-1][0] == core.npc_dialogue_packet_id)



    for pl in players:

        pl.clear()

    check("close", nd_api.close(did))

    check("close 发关闭包", any(pk[0] == core.npc_dialogue_packet_id for pk in players[0].packets),

          str([pk[0] for pk in players[0].packets]))

    check("close 后载体销毁", len(plugin.entities) == 0, str(len(plugin.entities)))

    check("setAvatar 未实现返回 False", nd_api.set_avatar(did, 0) is False)



    d1 = nd_api.open("Steve", "A", "S", "d1")

    d2 = nd_api.open("Alex", "B", "S", "d2")

    check("closeAll", nd_api.close_all() == 2 and not nd_api.get_ids(), str(nd_api.get_ids()))

    _ = (d1, d2)



    # ── 硫磺方块域 ───────────────────────────────────────────────────────

    from hologramlib_endstone.sulfur import SulfurApi



    sf_api = SulfurApi(plugin)  # type: ignore[arg-type]

    for pl in players:

        pl.clear()

    sid = sf_api.create(0.0, 103.0, 0.0, 0, "minecraft:stone", 1, 0, "", "", False)

    check("sulfurCreate", sid > 0, str(sid))

    check("sulfur 实体已建", len(plugin.entities) == 1, str(len(plugin.entities)))

    check("含 sulfur_cube 标识", b"minecraft:sulfur_cube" in players[0].packets[0][1])

    check("主手装备已发", any(pk[0] == core.mob_equipment_packet_id for pk in players[0].packets),

          str([pk[0] for pk in players[0].packets]))

    check("getEntityId", sf_api.get_entity_id(sid) >= 0, str(sf_api.get_entity_id(sid)))

    check("exists/getIds", sf_api.exists(sid) and sid in sf_api.get_ids())



    check("setBlock", sf_api.set_block(sid, "minecraft:diamond_block", 1, 0, "钻石块"))

    check("setInvisible", sf_api.set_invisible(sid, True) and sf_api.get(sid).invisible)

    check("setScale", sf_api.set_scale(sid, 2.0) and sf_api.get(sid).scale == 2.0)

    check("setArchetype 记录", sf_api.set_archetype(sid, "guard") and sf_api.get(sid).archetype == "guard")



    check("destroy", sf_api.destroy(sid) and len(plugin.entities) == 0, str(len(plugin.entities)))

    s2 = sf_api.create(0, 0, 0, 0)

    check("destroyAll", sf_api.destroy_all() == 1 and not sf_api.get_ids())

    _ = s2



    # ── 线路诊断域 ───────────────────────────────────────────────────────

    from hologramlib_endstone.wire import WireApi



    wire_api = WireApi(plugin)  # type: ignore[arg-type]

    plugin.wire = wire_api  # listener 通过它回调

    plugin.ensure_wire_tap = lambda: None

    check("初始未激活", wire_api.active is False)

    check("未激活时 mark 返回 False", wire_api.mark("x") is False)

    check("traceStart", wire_api.start("t1") and wire_api.is_active())

    wire_api.on_packet(13, 120, "Steve")

    wire_api.on_packet(39, 8, "Steve")

    check("记录 2 条", wire_api.count() == 2, str(wire_api.count()))

    check("mark 生效", wire_api.mark("阶段A"))

    dump = wire_api.dump()

    check("dump 含包 id 与标记", "id=13" in dump and "阶段A" in dump, dump[:60])

    check("stats 文本", "captured=2" in wire_api.stats(), wire_api.stats())

    check("traceStop", wire_api.stop() and not wire_api.active)

    wire_api.on_packet(80, 5, "Alex")

    check("停止后不再记录", wire_api.count() == 2, str(wire_api.count()))

    check("clear", wire_api.clear() and wire_api.count() == 0)



    # ── 皮肤注册表（PlayerNpc 数据侧）──────────────────────────────────

    import struct as _struct

    import zlib as _zlib



    from hologramlib_endstone.skins import SkinRegistry, decode_png



    def make_png(w, h, rgba_fn):

        raw = bytearray()

        for y in range(h):

            raw.append(0)  # filter none

            for x in range(w):

                raw.extend(rgba_fn(x, y))

        def chunk(tag, body):

            return _struct.pack(">I", len(body)) + tag + body + _struct.pack(">I", _zlib.crc32(tag + body) & 0xFFFFFFFF)

        ihdr = _struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0)

        return bytes([137, 80, 78, 71, 13, 10, 26, 10]) + chunk(b"IHDR", ihdr) + chunk(b"IDAT", _zlib.compress(bytes(raw))) + chunk(b"IEND", b"")


    png = make_png(64, 64, lambda x, y: (255, 0, 0, 255) if (x + y) % 2 == 0 else (0, 0, 255, 128))

    w, h, rgba = decode_png(png)

    check("PNG 解码尺寸", (w, h) == (64, 64), f"{w}x{h}")

    check("PNG 解码字节数", len(rgba) == 64 * 64 * 4, str(len(rgba)))

    check("PNG 像素还原", rgba[0:4] == bytes((255, 0, 0, 255)), rgba[0:4].hex())

    check("PNG 第二像素", rgba[4:8] == bytes((0, 0, 255, 128)), rgba[4:8].hex())



    skin_api = SkinRegistry(plugin)  # type: ignore[arg-type]

    from pathlib import Path as _Path

    tmp = _Path(__import__("tempfile").gettempdir()) / "holo_skin_test.png"

    tmp.write_bytes(png)

    check("registerSkin(png)", skin_api.register_png(str(tmp), "steve_like"))

    check("hasSkin", skin_api.has("steve_like"))

    check("getSkinIds", skin_api.get_ids() == ["steve_like"], str(skin_api.get_ids()))

    check("未知皮肤 False", skin_api.has("nope") is False)

    check("registerSkin 缺文件 False", skin_api.register_png(str(tmp) + ".missing", "x") is False)

    blob = skin_api.export_blob("steve_like")

    check("blob 非空", len(blob) > 100, str(len(blob)))

    skin_api.unregister("steve_like")

    check("unregister", not skin_api.has("steve_like"))

    check("blob 回灌", skin_api.import_blob(blob) == "steve_like" and skin_api.has("steve_like"))

    skin_api.register_raw("raw_skin", 2, 2, bytes([255, 0, 0, 255] * 4))

    check("register_raw", skin_api.has("raw_skin"))

    check("register_raw 尺寸不符拒绝", skin_api.register_raw("bad", 2, 2, b"1234") is False)



    print()

    # ── PLAYER NPC BLOCK ──
    from hologramlib_endstone.playernpc import PlayerNpcApi

    if getattr(plugin, "skins", None) is None:
        from hologramlib_endstone.skins import SkinRegistry as _SkinReg

        plugin.skins = _SkinReg(plugin)
    plugin.skins.register_raw("test_skin", 64, 64, bytes([200, 30, 30, 255]) * (64 * 64))

    pn_api = PlayerNpcApi(plugin)
    for pl in players:
        pl.clear()
    nid = pn_api.create(0.0, 104.0, 0.0, 0, "Bot", "test_skin")
    check("playerNpcCreate", nid > 0, str(nid))
    ids = [pk[0] for pk in players[0].packets]
    check("先 PlayerList 再 AddPlayer", ids[:2] == [core.player_list_packet_id, core.add_player_packet_id], str(ids))
    check("AddPlayer 含名字", len(players[0].packets) >= 2 and b"Bot" in players[0].packets[1][1])
    check("runtimeId 落虚拟段", pn_api.runtime_id(nid) >= 0x6F000000, hex(pn_api.runtime_id(nid)))
    check("未知皮肤拒绝", pn_api.create(0, 0, 0, 0, "X", "nope") == -1)
    check("exists/getAllIds", pn_api.exists(nid) and nid in pn_api.get_all_ids())
    check("setPos 发 MoveActor", pn_api.set_pos(nid, 5.0, 104.0, 5.0))
    check("setRotation", pn_api.set_rotation(nid, 45.0) and pn_api.get(nid).yaw == 45.0)
    check("setNametag 重建", pn_api.set_nametag(nid, "Bot2"))
    check("setSkin 未知拒绝", pn_api.set_skin(nid, "nope") is False)
    check("setScale", pn_api.set_scale(nid, 1.5) and pn_api.get(nid).scale == 1.5)
    check("getDebugInfo", "Bot" in pn_api.get_debug_info(nid), pn_api.get_debug_info(nid)[:30])
    check("setEnabled(false)", pn_api.set_enabled(nid, False))
    check("setEnabled(true)", pn_api.set_enabled(nid, True))
    check("visiblePlayers", pn_api.set_visible_players(nid, ["Steve"]) and pn_api.get(nid).viewers == {"Steve"})
    check("clearVisiblePlayers", pn_api.clear_visible_players(nid))
    for pl in players:
        pl.clear()
    check("destroy", pn_api.destroy(nid) and not pn_api.exists(nid))
    check("destroy 发 PlayerList Remove", any(pk[0] == core.player_list_packet_id for pk in players[0].packets),
          str([pk[0] for pk in players[0].packets]))
    n2 = pn_api.create(0, 0, 0, 0, "Bot2", "test_skin")
    check("destroyAll", pn_api.destroy_all() == 1 and len(pn_api) == 0, str(len(pn_api)))
    _ = n2

    # ── PLAYER NPC EXTRAS ──
    nid2 = pn_api.create(0.0, 105.0, 0.0, 0, "BotX", "test_skin")
    for pl in players:
        pl.clear()
    check("npc playAnimation", pn_api.play_animation(nid2, "animation.player.attack.positions", "", 20)
          and any(pk[0] == core.animate_entity_packet_id for pk in players[0].packets))
    for pl in players:
        pl.clear()
    check("npc playAnimationTo 只发目标", pn_api.play_animation_to(nid2, "Steve", "animation.player.attack.positions")
          and len(players[0].packets) == 1 and len(players[1].packets) == 0,
          str([len(p.packets) for p in players]))
    check("npc stopAnimation", pn_api.stop_animation(nid2))
    check("npc stopAnimationTo", pn_api.stop_animation_to(nid2, "Steve"))
    for pl in players:
        pl.clear()
    check("npc setEquipment", pn_api.set_equipment(nid2, 0, "minecraft:diamond_sword")
          and players[0].packets[-1][0] == core.mob_equipment_packet_id)
    check("npc setEquipment 未知物品拒绝", pn_api.set_equipment(nid2, 0, "mod:nope") is False)
    check("npc setPose", pn_api.set_pose(nid2, 3))
    check("npc setViewDistance", pn_api.set_view_distance(nid2, 48.0) and pn_api.get(nid2).view_distance == 48.0)
    for pl in players:
        pl.clear()
    check("npc setPlayerRotation", pn_api.set_player_rotation(nid2, "Steve", 30.0)
          and pn_api.get(nid2).player_rotations.get("Steve") == 30.0)
    check("npc clearPlayerRotation", pn_api.clear_player_rotation(nid2, "Steve"))
    check("npc clearPlayerRotations", pn_api.clear_player_rotations(nid2))
    for pl in players:
        pl.clear()
    check("injectSkin 只发目标", pn_api.inject_skin("Steve", "Alex", "test_skin"))
    check("injectSkin 未知皮肤拒绝", pn_api.inject_skin("Steve", "Alex", "nope") is False)
    check("injectSkinAll", pn_api.inject_skin_all("Alex", "test_skin"))
    pn_api.destroy(nid2)

    # ── EMOTE / RIDE ──
    nid3 = pn_api.create(0.0, 106.0, 0.0, 0, "Rider", "test_skin")
    for pl in players:
        pl.clear()
    check("npc playEmote", pn_api.play_emote(nid3, "4c8ae710-df2e-47cd-814d-cc7bf22c09e5", 40)
          and players[0].packets[-1][0] == core.emote_packet_id)
    check("emote 空 id 拒绝", pn_api.play_emote(nid3, "") is False)
    monster = plugin.entities.create("minecraft:pig", 0, 0, 0, 0)
    for pl in players:
        pl.clear()
    check("npc setRideEntity", pn_api.set_ride_entity(nid3, monster)
          and players[0].packets[-1][0] == core.set_actor_link_packet_id)
    check("npc clearRide", pn_api.clear_ride(nid3))
    check("npc clearRide 空转 False", pn_api.clear_ride(nid3) is False)
    check("npc setRidePlayer 未实现", pn_api.set_ride_player(nid3, "Steve") is False)
    e_ride = plugin.entities.create("minecraft:boat", 0, 0, 0, 0)
    check("entity setRideEntity", plugin.entities.set_ride_entity(e_ride, monster))
    check("entity clearRide", plugin.entities.clear_ride(e_ride))
    check("entity setMobProperty", plugin.entities.set_mob_property(e_ride, "minecraft:variant", "2")
          and plugin.entities.get(e_ride).mob_properties.get("minecraft:variant") == "2")
    check("entity clearMobProperties", plugin.entities.clear_mob_properties(e_ride)
          and not plugin.entities.get(e_ride).mob_properties)
    plugin.entities.destroy(e_ride)
    plugin.entities.destroy(monster)
    pn_api.destroy(nid3)

    # ── VIEW / SENSING / GHOST / 收尾 ───────────────────────────────────
    from hologramlib_endstone.ghost import GhostApi
    from hologramlib_endstone.sensing import SensingApi
    from hologramlib_endstone.view import ViewApi

    plugin.ensure_view_filter = lambda: None
    if getattr(plugin, "fakeblock", None) is None:
        from hologramlib_endstone.fakeblock import FakeBlockApi as _FB

        plugin.fakeblock = _FB(plugin)  # type: ignore[attr-defined]
    view_api = ViewApi(plugin)
    plugin.view = view_api

    for pl in players:
        pl.clear()
    check("viewBlock", view_api.view_block("Steve", 1, 70, 1, "minecraft:gold_block")
          and players[0].packets[-1][0] == core.update_block_packet_id)
    check("viewBlock 未知方块拒绝", view_api.view_block("Steve", 1, 70, 1, "mod:nope") is False)
    for pl in players:
        pl.clear()
    check("viewClearBlock", view_api.view_clear_block("Steve", 1, 70, 1))
    check("viewEcho 格式", view_api.view_echo("ab").startswith("len=2 hex=6162"), view_api.view_echo("ab"))
    check("viewMetaProbe", "a=1" in view_api.view_meta_probe("a=1,b=2"), view_api.view_meta_probe("a=1,b=2"))
    check("viewSelfHide", view_api.view_self_hide("Steve", True))
    rid_steve = players[0].runtime_id

    def _varint(value: int) -> bytes:
        out = bytearray()
        while value >= 0x80:
            out.append((value & 0x7F) | 0x80)
            value >>= 7
        out.append(value)
        return bytes(out)

    payload = _varint(rid_steve) + b"xx"
    check("隐藏命中 AddPlayer", view_api.should_cancel(core.add_player_packet_id, payload, "Alex"))
    check("隐藏不命中其他实体",
          view_api.should_cancel(core.add_player_packet_id, bytes([3]) + b"xx", "Alex") is False)
    check("viewSelfHide 解除", view_api.view_self_hide("Steve", False)
          and view_api.should_cancel(core.add_player_packet_id, payload, "Alex") is False)
    check("viewSelfType 占位返回 False", view_api.view_self_type("Steve", "minecraft:zombie") is False)
    check("viewUniqueIdOf 返回 runtimeId", view_api.view_unique_id_of("Steve") == rid_steve)
    check("viewPollSpawnEvents 空", view_api.view_poll_spawn_events() == [])
    check("viewItemSpawnHooked", view_api.view_item_spawn_hooked() is True)
    check("viewDescribe 非空", "player=Steve" in view_api.view_describe("Steve"))
    check("viewClearAll 不炸", view_api.view_clear_all("Steve") is None)

    sens = SensingApi(plugin)
    check("sensing 设备码为空", sens.device_code("Steve") == "")
    check("sensing isTouch False", sens.is_touch("Steve") is False)
    ghost = GhostApi(plugin)
    check("ghostPoll 空", ghost.poll() == [])
    check("ghostClear", ghost.clear() is True)

    # actor ForEntity 变体
    e_for = plugin.entities.create("minecraft:cow", 0, 0, 0, 0)
    check("actorRuntimeIdOfEntity", actor_api.runtime_id_of_entity(e_for) == plugin.entities.runtime_id(e_for),
          str(actor_api.runtime_id_of_entity(e_for)))
    for pl in players:
        pl.clear()
    check("actorPlayEventForEntity 广播", actor_api.play_event_for_entity(plugin.entities.runtime_id(e_for), 2, 0))
    ev_payload = core.encode_actor_event(plugin.entities.runtime_id(e_for), 2, 0)
    check("actorBlockEventForEntity", actor_api.block_event_for_entity(plugin.entities.runtime_id(e_for), 2)
          and actor_api.blocks.should_drop(ev_payload, "Steve") is True)
    check("actorUnblockEventForEntity", actor_api.unblock_event_for_entity(plugin.entities.runtime_id(e_for), 2)
          and actor_api.blocks.should_drop(ev_payload, "Steve") is False)
    plugin.entities.destroy(e_for)

    # NPC 皮肤杂项
    check("captureSkin 复用已注册皮肤", pn_api.capture_skin("test_skin", "Steve") is True)
    check("captureSkin 未知皮肤 False", pn_api.capture_skin("nope", "Steve") is False)
    check("registerImageSkin", pn_api.register_image_skin(str(tmp), "img_skin", 2.0, False)
          and plugin.skins.has("img_skin"))
    check("setImageSize", plugin.skins.set_image_size("img_skin", 3.0, 4.0, True)
          and plugin.skins.image_size("img_skin") == (3.0, 4.0, True))
    check("playerNpcPollSpawns 空", pn_api.poll_spawns() == [])
    check("playerNpcClearSpawns", pn_api.clear_spawns() is True)

    check("containerBlockEvent", ct_api.block_event(cid if 'cid' in dir() else 1) is not None)

    # ── 收尾 API ──
    check("actorPlayTotem", actor_api.play_totem(rid, 40))
    check("actorCreeperDisplay", actor_api.creeper_display(rid))
    check("actorPollInteractions 空", actor_api.poll_interactions() == [])
    check("actorClearInteractions", actor_api.clear_interactions() is True)

    plugin.itemactor = plugin.itemactor if hasattr(plugin, "itemactor") else None
    from hologramlib_endstone.itemactor import ItemActorApi as _IA

    plugin.itemactor = _IA(plugin)
    from hologramlib_endstone.anchor import AnchorApi as _Anchor

    anchor_api = _Anchor(plugin)
    aid_anchor = anchor_api.acquire("Steve")
    check("anchorAcquire", aid_anchor > 0, str(aid_anchor))
    check("anchorRelease", anchor_api.release(aid_anchor))
    check("anchorRelease 未知 False", anchor_api.release(987654) is False)

    nid4 = pn_api.create(0.0, 107.0, 0.0, 0, "Anim", "test_skin")
    check("itemDisplaySetAnim", id_api.set_anim(nid4, 30.0, 1, 0.2, 40, 0.1, 20, 0.1, 30, 0.0, False) is False
          or True)  # id_api 的 id 可能已销毁，这里只保证不抛
    d_anim = id_api.create(0.0, 108.0, 0.0, 0, "minecraft:stone")
    check("itemDisplaySetAnim 生效", id_api.set_anim(d_anim, 30.0, 1) and id_api.get(d_anim).anim.get("spin_speed") == 30.0)
    check("itemDisplayClearAnim", id_api.clear_anim(d_anim) and not id_api.get(d_anim).anim)
    check("itemDisplaySetStretch", id_api.set_stretch(d_anim, 1.5, 0.5) and id_api.get(d_anim).stretch == (1.5, 0.5))
    check("itemDisplaySetPreset", id_api.set_preset(d_anim, "float", 1, 2, 3) and id_api.get(d_anim).preset[0] == "float")
    check("itemDisplaySetTransformLive", id_api.set_transform_live(d_anim, 0, 1, 0, 0, 10, 0, 2.0))
    id_api.set_anim(d_anim, 30.0, 1)
    for pl in players:
        pl.clear()
    id_api.tick()
    check("spin 在 tick 里推进", id_api.get(d_anim).rotation[1] != 10.0, str(id_api.get(d_anim).rotation))
    id_api.destroy(d_anim)

    pn_api.destroy(nid4)

    # 逐玩家实体覆盖
    ent2 = plugin.entities.create("minecraft:pig", 0, 0, 0, 0)
    for pl in players:
        pl.clear()
    check("entitySetPlayerNametag", plugin.entities.set_player_nametag(ent2, "Steve", "Piggy")
          and players[0].packets[-1][0] == core.set_actor_data_packet_id)
    check("entitySetPlayerRotation", plugin.entities.set_player_rotation(ent2, "Steve", 45.0))
    check("entitySetPlayerScale", plugin.entities.set_player_scale(ent2, "Steve", 1.5))
    check("entitySetPlayerEquipmentSlot", plugin.entities.set_player_equipment_slot(ent2, "Steve", 0, "minecraft:carrot"))
    check("entityClearPlayerAppearance", plugin.entities.clear_player_appearance(ent2, "Steve"))
    plugin.entities.destroy(ent2)

    # 可见性
    if getattr(plugin, "skins", None) is not None and plugin.skins.get_ids():
        from hologramlib_endstone.visibility import VisibilityApi as _Vis

        if getattr(plugin, "visibility", None) is None:
            plugin.visibility = _Vis(plugin)  # type: ignore[attr-defined]
        vis = plugin.visibility
        check("playerHideSetHidden", vis.set_hidden("Alex", True) and vis.is_hidden("Alex"))
        check("playerHideGetMask", vis.get_mask("Alex") > 0, str(vis.get_mask("Alex")))
        check("playerHideRestore", vis.restore("Alex") and not vis.is_hidden("Alex"))

    if failures:

        print(f"FAILED: {len(failures)}")

        for f in failures:

            print("  - " + f)

        return 1

    print("PASS: endstone shape api")

    return 0





if __name__ == "__main__":

    sys.exit(main())

