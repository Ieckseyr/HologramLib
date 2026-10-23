// HoloBoxUITest.js —— HologramLib 虚拟容器（箱子 UI）LSE 冒烟测试（指令版）
//
// 目的：一个脚本把 container* 的 10 个导出全跑一遍，并在界面上给出可点的按钮，
//       让人肉也能验"点击回传 / 就地刷新 / 换标题 / 可交互 / 关闭"。
//
// 三个指令（进服后在聊天栏输入，带不带斜杠都行）：
//   /boxui      跑自检断言，然后打开 54 格测试界面给你手点
//   /boxtest    只跑自检断言（不开界面）
//   /boxstop    关掉所有容器界面并清空点击队列
//
// 前置：HologramLib（本库）+ legacy-script-engine-quickjs
// 说明：本脚本是**测试**，不做业务；所有判断都会打 [PASS]/[FAIL] 到控制台。
'use strict';

const PLUGIN_NAME = 'HoloBoxUITest';
const PLUGIN_VER  = [1, 1, 0];
const NS          = 'HologramLib';
const PREFIX      = '[' + PLUGIN_NAME + '] ';

// ── 0. 前置绑定 ────────────────────────────────────────────────────────────
// 全部 10 个导出；缺任何一个都报出来（版本不匹配时最有用）
const REQUIRED = [
    'containerOpen',
    'containerSetItem',
    'containerSetTitle',
    'containerSetInteractive',
    'containerClose',
    'containerCloseAll',
    'containerIsOpen',
    'containerGetIds',
    'containerPollClicks',
    'containerClearClicks'
];

const H = {};            // 函数表：H.containerOpen(...) 这样调
let apiReady = false;

function tryImport(name) {
    try {
        if (typeof ll.import !== 'function') return null;
        const fn = ll.import(NS, name);
        return typeof fn === 'function' ? fn : null;
    } catch (e) {
        return null;
    }
}

function bindApi() {
    for (let i = 0; i < REQUIRED.length; i++) {
        const n  = REQUIRED[i];
        const fn = tryImport(n);
        if (fn) H[n] = fn;
    }
    const missing = missingApi();
    apiReady = missing.length === 0;
    return { ok: apiReady, missing: missing };
}

function missingApi() {
    const miss = [];
    for (let i = 0; i < REQUIRED.length; i++) {
        if (typeof H[REQUIRED[i]] !== 'function') miss.push(REQUIRED[i]);
    }
    return miss;
}

// 指令入口统一走这里：没就绪就先绑一次，还缺就把缺的名字回给执行者
function ensureApi(out) {
    if (apiReady) return true;
    const r = bindApi();
    if (r.ok) return true;
    const msg = 'HologramLib 未就绪，缺导出: ' + r.missing.join(', ');
    logger.error(PREFIX + msg);
    if (out) out.error(msg);
    return false;
}

// ── 1. 断言小工具 ──────────────────────────────────────────────────────────
let stat = { pass: 0, fail: 0 };

function resetStat() { stat = { pass: 0, fail: 0 }; }

function ok(cond, what, detail) {
    if (cond) {
        stat.pass++;
        logger.info(PREFIX + '  [PASS] ' + what + (detail === undefined ? '' : '  → ' + detail));
        return true;
    }
    stat.fail++;
    logger.error(PREFIX + '  [FAIL] ' + what + (detail === undefined ? '' : '  → ' + detail));
    return false;
}

function show(v) {
    if (typeof v === 'string') return '"' + v + '"';
    if (Array.isArray(v)) return '[' + v.join(', ') + ']';
    return String(v);
}

function truthy(v) { return v === true || v === 1; }

// ── 2. 菜单状态 ────────────────────────────────────────────────────────────
// playerName -> { id, page, interactive }
const menus = {};

const TITLE_BASE = '§8[测试] 箱子 UI';
function pageTitle(p) { return TITLE_BASE + ' §7· 第 ' + p + ' 页'; }

const LORE_HELP = '§7点我试试|§8点击回传 slot 见控制台';

// 54 格界面的固定按钮布局（rows = 6）
const SLOT_REFRESH   = 4;    // 就地刷新一个格子
const SLOT_TITLE     = 8;    // 就地换标题
const SLOT_INTERACT  = 13;   // 切换可交互模式
const SLOT_GIVE      = 22;   // 测试发物品（顺带验业务回调）
const SLOT_PAGE_NEXT = 26;   // 翻页
const SLOT_CLOSE     = 45;   // 关闭本菜单
const SLOT_CLOSE_ALL = 53;   // 关闭全部

// ── 3. 自检断言（不依赖玩家手点）──────────────────────────────────────────
function selfTest(playerName) {
    logger.info(PREFIX + '=========== 自检开始 (player=' + playerName + ') ===========');
    resetStat();

    // T0 前置
    ok(apiReady, 'T0 前置：10 个 container* 导出全部存在');

    H.containerClearClicks();

    // T1 打开
    const id = H.containerOpen(playerName, pageTitle(1), 6);
    ok(typeof id === 'number' && id >= 0, 'T1 containerOpen 返回菜单 id', show(id));
    if (typeof id !== 'number' || id < 0) {
        ok(false, 'T1 打开失败 → 后续用例跳过（玩家不在线？）');
        logger.info(PREFIX + '=========== 自检中止 ===========');
        return -1;
    }
    menus[playerName] = { id: id, page: 1, interactive: false };

    // T2 填格：类型/数量/damage/自定义名/lore
    ok(truthy(H.containerSetItem(id, 0, 'minecraft:diamond_sword', 1, 0, '§b§l试炼之剑', '§7点击回传 slot=0|§8第二行')),
        'T2a containerSetItem 填 0 号格（带 lore）');
    ok(truthy(H.containerSetItem(id, 1, 'minecraft:golden_apple', 16, 0, '§6金苹果', LORE_HELP)),
        'T2b containerSetItem 填 1 号格（带数量）');
    ok(truthy(H.containerSetItem(id, 2, 'minecraft:grass_block', 64, 0, '', '')),
        'T2c containerSetItem 方块（无名字无 lore）');
    ok(truthy(H.containerSetItem(id, 3, 'minecraft:potion', 1, 0, '§d药水', '§7damage=0')),
        'T2d containerSetItem 带 damage 的物品');

    // T3 边界：越界 / 清空
    ok(!truthy(H.containerSetItem(id, 54, 'minecraft:stone', 1, 0, '', '')),
        'T3a containerSetItem 越界(slot=54) 返回 false');
    ok(!truthy(H.containerSetItem(id, -1, 'minecraft:stone', 1, 0, '', '')),
        'T3b containerSetItem 负数槽位返回 false');
    ok(truthy(H.containerSetItem(id, 3, '', 1, 0, '', '')),
        'T3c containerSetItem 空 type = 清空该格');

    // T4 标题
    ok(truthy(H.containerSetTitle(id, pageTitle(2))), 'T4a containerSetTitle 换第 2 页标题');
    ok(truthy(H.containerSetTitle(id, pageTitle(1))), 'T4b containerSetTitle 换回第 1 页');

    // T5 可交互
    ok(truthy(H.containerSetInteractive(id, true)),  'T5a containerSetInteractive(true)');
    ok(truthy(H.containerSetInteractive(id, false)), 'T5b containerSetInteractive(false)');

    // T6 查询
    ok(truthy(H.containerIsOpen(id)), 'T6a containerIsOpen(有效 id) === true');
    const ids = H.containerGetIds();
    ok(Array.isArray(ids) && ids.indexOf(id) >= 0, 'T6b containerGetIds 包含本菜单', show(ids));

    // T7 不存在的 id 一律 false（负向用例）
    const bogus = 999999;
    ok(!truthy(H.containerIsOpen(bogus)),               'T7a containerIsOpen(不存在) === false');
    ok(!truthy(H.containerClose(bogus)),                'T7b containerClose(不存在) === false');
    ok(!truthy(H.containerSetItem(bogus, 0, 'minecraft:stone', 1, 0, '', '')),
        'T7c containerSetItem(不存在) === false');
    ok(!truthy(H.containerSetTitle(bogus, 'x')),        'T7d containerSetTitle(不存在) === false');
    ok(!truthy(H.containerSetInteractive(bogus, true)), 'T7e containerSetInteractive(不存在) === false');

    // T8 轮询队列
    const clicks = H.containerPollClicks();
    ok(Array.isArray(clicks), 'T8a containerPollClicks 返回数组', '长度 ' + (Array.isArray(clicks) ? clicks.length : '?'));

    H.containerClearClicks();
    const afterClear = H.containerPollClicks();
    ok(Array.isArray(afterClear) && afterClear.length === 0, 'T8b containerClearClicks 后队列为空');

    logger.info(PREFIX + '=========== 自检结束: PASS ' + stat.pass + ' / FAIL ' + stat.fail + ' ===========');
    return id;
}

// ── 4. 界面布局（把可点的按钮摆上去）──────────────────────────────────────
function layout(id, page) {
    // 清掉非按钮格，避免翻页残留
    for (let s = 0; s < 54; s++) {
        if (s === SLOT_REFRESH || s === SLOT_TITLE || s === SLOT_INTERACT || s === SLOT_GIVE) continue;
        if (s === SLOT_PAGE_NEXT || s === SLOT_CLOSE || s === SLOT_CLOSE_ALL) continue;
        H.containerSetItem(id, s, '', 1, 0, '', '');
    }

    H.containerSetItem(id, SLOT_REFRESH, 'minecraft:clock', 1, 0,
        '§e就地刷新（slot ' + SLOT_REFRESH + '）', '§7点我 → 该格换成附魔金苹果|§8验证无闪烁刷新');
    H.containerSetItem(id, SLOT_TITLE, 'minecraft:name_tag', 1, 0,
        '§e换标题（slot ' + SLOT_TITLE + '）', '§7点我 → 标题翻页|§8验证就地换标题');
    H.containerSetItem(id, SLOT_INTERACT, 'minecraft:hopper', 1, 0,
        '§e切换可交互（slot ' + SLOT_INTERACT + '）', '§7点我 → 在"只回传"和"能搬东西"之间切');
    H.containerSetItem(id, SLOT_GIVE, 'minecraft:diamond', 1, 0,
        '§b领一颗钻石（slot ' + SLOT_GIVE + '）', '§7点我 → 回调里真的 giveItem');

    H.containerSetItem(id, SLOT_PAGE_NEXT, 'minecraft:paper', 1, 0,
        '§f翻到第 ' + (page === 1 ? 2 : 1) + ' 页（slot ' + SLOT_PAGE_NEXT + '）', '§8page = ' + page);
    H.containerSetItem(id, SLOT_CLOSE, 'minecraft:barrier', 1, 0,
        '§c关闭本菜单（slot ' + SLOT_CLOSE + '）', '§7验证 closed=1 回传 + isOpen=false');
    H.containerSetItem(id, SLOT_CLOSE_ALL, 'minecraft:tnt', 1, 0,
        '§4关闭全部（slot ' + SLOT_CLOSE_ALL + '）', '§7containerCloseAll()');

    for (let p = 0; p < 2; p++) {
        H.containerSetItem(id, 46 + p, p === (page - 1) ? 'minecraft:lime_dye' : 'minecraft:gray_dye', 1, 0,
            p === (page - 1) ? '§a第 ' + (p + 1) + ' 页' : '§7第 ' + (p + 1) + ' 页', '');
    }
}

function openFor(playerName) {
    const id = selfTest(playerName);
    if (id < 0) return -1;
    layout(id, 1);
    logger.info(PREFIX + ' 界面已打开: player=' + playerName + ' menuId=' + id +
        '（点格子看控制台的 [CLICK] 行；/boxstop 关闭）');
    return id;
}

// ── 5. 点击分发 ────────────────────────────────────────────────────────────
const CLICK_RE = /player=(\S+) menuId=(-?\d+) slot=(-?\d+) closed=(\d)/;

function handleClick(line) {
    const m = CLICK_RE.exec(line);
    if (!m) {
        logger.warn(PREFIX + ' [CLICK] 无法解析的行: ' + line);
        return;
    }
    const playerName = m[1];
    const eventId    = parseInt(m[2], 10);
    const slot       = parseInt(m[3], 10);
    const closed     = m[4] === '1';

    logger.info(PREFIX + ' [CLICK] player=' + playerName + ' menuId=' + eventId +
        ' slot=' + slot + ' closed=' + (closed ? 1 : 0));

    const st = menus[playerName];
    if (!st) {
        logger.warn(PREFIX + ' [CLICK] 这个玩家没有本脚本管理的菜单（menuId=' + eventId + '），忽略');
        return;
    }

    // 库回传的 menuId 必须等于打开时拿到的 id。
    //  · 回传 0 = 库没填（历史上出过这个 bug）→ 按玩家认领 + 记一条 FAIL，别让整轮测试白跑
    //  · 回传别的非 0 id = 别人家的菜单 → 忽略
    let menuId = eventId;
    if (eventId === 0) {
        ok(false, 'C9 点击回传的 menuId 与打开时一致', '回传 0（未填），实际 ' + st.id);
        menuId = st.id;
    } else if (eventId !== st.id) {
        logger.warn(PREFIX + ' [CLICK] 不是本脚本的菜单（回传 ' + eventId + '，本脚本 ' + st.id + '），忽略');
        return;
    } else if (!st.idChecked) {
        // 每个菜单只验一次，避免每点一下都刷一条
        ok(true, 'C9 点击回传的 menuId 与打开时一致', 'menuId=' + eventId);
        st.idChecked = true;
    }

    if (closed) {
        // 界面被关闭（客户端 ESC / 我们主动 close 都会到这里）
        // 库在派发前已经把菜单摘掉了 → isOpen 应当是 false
        ok(!truthy(H.containerIsOpen(menuId)), 'C0 关闭回传后 isOpen(menuId) === false');
        delete menus[playerName];
        return;
    }

    if (slot === SLOT_REFRESH) {
        ok(truthy(H.containerSetItem(menuId, SLOT_REFRESH, 'minecraft:enchanted_golden_apple', 1, 0,
            '§6已刷新 ' + new Date().toTimeString().slice(0, 8), '§7同一个格子就地换掉了')), 'C1 点击后就地刷新该格');
    } else if (slot === SLOT_TITLE) {
        st.page = st.page === 1 ? 2 : 1;
        ok(truthy(H.containerSetTitle(menuId, pageTitle(st.page))), 'C2 点击后换标题（第 ' + st.page + ' 页）');
    } else if (slot === SLOT_INTERACT) {
        st.interactive = !st.interactive;
        ok(truthy(H.containerSetInteractive(menuId, st.interactive)),
            'C3 切换可交互 → ' + (st.interactive ? '可搬东西' : '只回传'));
    } else if (slot === SLOT_GIVE) {
        giveItem(playerName, 'minecraft:diamond', 1);
    } else if (slot === SLOT_PAGE_NEXT) {
        st.page = st.page === 1 ? 2 : 1;
        layout(menuId, st.page);
        ok(truthy(H.containerSetTitle(menuId, pageTitle(st.page))), 'C4 翻页（重排 + 换标题）');
    } else if (slot === SLOT_CLOSE) {
        ok(truthy(H.containerClose(menuId)), 'C5 containerClose 返回 true');
        ok(!truthy(H.containerIsOpen(menuId)), 'C5 关闭后 isOpen === false');
        delete menus[playerName];
    } else if (slot === SLOT_CLOSE_ALL) {
        H.containerCloseAll();
        ok(true, 'C6 containerCloseAll 已调用');
        for (const k in menus) delete menus[k];
    } else {
        logger.info(PREFIX + ' [CLICK] slot=' + slot + '（布局外的格子，仅记录）');
    }
}

// 发物品（可选能力：拿不到 mc.newItem 时只记一条日志，不算失败）
function giveItem(playerName, type, count) {
    try {
        const pl = mc.getPlayer(playerName);
        if (!pl) { logger.warn(PREFIX + ' 发物品失败：玩家不在线'); return; }
        const item = mc.newItem(type, count);
        if (!item) { logger.warn(PREFIX + ' mc.newItem 不可用，跳过发物品'); return; }
        pl.giveItem(item);
        ok(true, 'C7 回调里 giveItem 成功', type + ' ×' + count);
    } catch (e) {
        logger.warn(PREFIX + ' giveItem 异常: ' + (e && e.message));
    }
}

// ── 6. 轮询（200ms 一拍）──────────────────────────────────────────────────
let tickCounter = 0;
function onTick() {
    if (!apiReady) return;
    if ((++tickCounter % 4) !== 0) return;   // 20tps / 4 = 5 次每秒
    let lines;
    try {
        lines = H.containerPollClicks();
    } catch (e) {
        logger.error(PREFIX + ' containerPollClicks 异常: ' + (e && e.message));
        return;
    }
    if (!Array.isArray(lines) || lines.length === 0) return;
    for (let i = 0; i < lines.length; i++) handleClick(lines[i]);
}

// ── 7. 三个指令的实现 ─────────────────────────────────────────────────────
function permTypeAny() {
    // PermType 万一不可用就退回两参形式（默认 Any）
    if (typeof PermType !== 'undefined' && PermType && typeof PermType.Any !== 'undefined') return PermType.Any;
    return undefined;
}

function cmdOpen(origin, out) {
    const player = origin ? origin.player : null;
    if (!player) { out.error('这个指令只能由玩家在游戏内执行'); return true; }
    if (!ensureApi(out)) return true;

    const name = player.realName;
    const id   = openFor(name);
    if (id < 0) {
        out.error('打开失败（你不在线？看控制台）');
        return true;
    }
    out.success('箱子 UI 已打开（menuId=' + id + '）：点格子看控制台；/boxstop 关闭');
    logger.info(PREFIX + ' /boxui: player=' + name + ' menuId=' + id);
    return true;
}

function cmdTest(origin, out) {
    const player = origin ? origin.player : null;
    if (!player) { out.error('这个指令只能由玩家在游戏内执行'); return true; }
    if (!ensureApi(out)) return true;

    const id = selfTest(player.realName);
    if (id < 0) {
        out.error('打开失败（你不在线？看控制台）');
        return true;
    }
    H.containerClose(id);
    out.success('自检完成：PASS ' + stat.pass + ' / FAIL ' + stat.fail + '（详情看控制台）');
    logger.info(PREFIX + ' /boxtest: PASS ' + stat.pass + ' FAIL ' + stat.fail);
    return true;
}

function cmdStop(origin, out) {
    if (!ensureApi(out)) return true;
    H.containerCloseAll();
    H.containerClearClicks();
    for (const k in menus) delete menus[k];
    out.success('已关闭所有容器界面并清空点击队列');
    logger.info(PREFIX + ' /boxstop: 已清理');
    return true;
}

// 注册三个无参指令（不依赖 ParamType / setEnum，最不容易受版本差异影响）
const COMMANDS = [
    { name: 'boxui',   desc: 'HologramLib 箱子 UI 测试: 自检 + 打开测试界面', fn: cmdOpen },
    { name: 'boxtest', desc: 'HologramLib 箱子 UI 测试: 只跑断言',            fn: cmdTest },
    { name: 'boxstop', desc: 'HologramLib 箱子 UI 测试: 关闭全部界面',        fn: cmdStop }
];

function registerCommands() {
    if (typeof mc.newCommand !== 'function') {
        return { ok: false, why: 'mc.newCommand 不可用（脚本引擎版本过低）' };
    }
    for (let i = 0; i < COMMANDS.length; i++) {
        const spec = COMMANDS[i];
        const perm = permTypeAny();
        const cmd  = (perm === undefined)
            ? mc.newCommand(spec.name, spec.desc)
            : mc.newCommand(spec.name, spec.desc, perm);
        cmd.overload([]);
        cmd.setCallback(function (_cmd, origin, output) {
            try {
                return spec.fn(origin, output);
            } catch (e) {
                logger.error(PREFIX + ' /' + spec.name + ' 异常: ' + (e && e.message) + '\n' + (e && e.stack));
                if (output) output.error('内部异常，看控制台');
                return false;
            }
        });
        cmd.setup();
    }
    return { ok: true };
}

// ── 8. 启动 ────────────────────────────────────────────────────────────────
function start() {
    if (typeof ll !== 'undefined' && typeof ll.registerPlugin === 'function') {
        try {
            ll.registerPlugin(PLUGIN_NAME, 'HologramLib 虚拟容器（箱子 UI）冒烟测试（指令版）', PLUGIN_VER, {});
        } catch (e) {
            logger.warn(PREFIX + ' ll.registerPlugin 失败（忽略）: ' + (e && e.message));
        }
    }

    const r = bindApi();
    if (r.ok) {
        logger.info(PREFIX + ' v' + PLUGIN_VER.join('.') + ' 已启动: 前置就绪');
    } else {
        logger.warn(PREFIX + ' v' + PLUGIN_VER.join('.') + ' 已启动，但前置未就绪，缺: ' + r.missing.join(', ') +
            '（执行 /boxui 时会再试一次）');
    }

    // 指令
    let cmdState;
    try {
        cmdState = registerCommands();
    } catch (e) {
        cmdState = { ok: false, why: (e && e.message) || String(e) };
    }
    if (cmdState.ok) {
        logger.info(PREFIX + ' 指令已注册: /boxui /boxtest /boxstop');
    } else {
        logger.error(PREFIX + ' 指令注册失败（' + cmdState.why + '）→ 退回聊天前缀 .boxui / .boxtest / .boxstop');
        try { mc.listen('onChat', onChatFallback); } catch (e) { logger.error(PREFIX + ' onChat 监听失败: ' + (e && e.message)); }
    }

    try { mc.listen('onTick', onTick); } catch (e) { logger.error(PREFIX + ' onTick 监听失败: ' + (e && e.message)); }

    // 玩家退服：库自己会清理该玩家的容器，这里只清脚本侧记录
    try {
        mc.listen('onLeft', function (p) {
            if (p && menus[p.realName]) delete menus[p.realName];
        });
    } catch (e) { /* 可选 */ }
}

// 给玩家发一条消息（不同引擎版本的方法名不一致，逐个试，最后退化成写日志）
function playerTell(player, msg) {
    try { if (typeof player.sendMessage === 'function') { player.sendMessage(msg); return; } } catch (e) { /* 下一个 */ }
    try { if (typeof player.tell === 'function') { player.tell(msg); return; } } catch (e) { /* 写日志 */ }
    logger.info(PREFIX + msg);
}

// 只在指令注册失败时才启用的聊天兜底
function onChatFallback(player, msg) {
    if (!player) return true;
    const text = (msg || '').trim().toLowerCase();
    if (text !== '.boxui' && text !== '.boxtest' && text !== '.boxstop') return true;
    const out = {
        success: function (m) { playerTell(player, '§a' + m); },
        error: function (m) { playerTell(player, '§c' + m); }
    };
    if (text === '.boxui') cmdOpen({ player: player }, out);
    else if (text === '.boxtest') cmdTest({ player: player }, out);
    else cmdStop({ player: player }, out);
    return false;
}

start();
