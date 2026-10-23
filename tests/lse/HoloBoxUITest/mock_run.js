// mock_run.js —— 用假 mc/ll/logger 跑 HoloBoxUITest.js，验证逻辑（不连服务器）
//
// 模拟了 LLSE 的指令 API：mc.newCommand(name, desc, perm) -> { overload, setCallback, setup }
// 注册后可以用 callCmd('/boxui') 直接触发，等价于玩家在游戏里敲指令。
const fs = require('fs');

const log = [];
const mkLogger = (lv) => (m) => log.push(`[${lv}] ${m}`);

// ── 假容器管理器（语义对齐 C++ 侧）──
const menus = new Map();   // id -> { title, slots, items:Map, interactive, player }
const queue = [];          // 点击事件行
let nextId = 1;

const containerApi = {
    containerOpen(playerName, title, rows) {
        if (!players.has(playerName)) return -1;
        const id = nextId++;
        const slots = rows >= 6 ? 54 : 27;
        menus.set(id, { title, slots, items: new Map(), interactive: false, player: playerName });
        return id;
    },
    containerSetItem(id, slot, type, count, damage, name, loreCsv) {
        const m = menus.get(id);
        if (!m) return false;
        if (slot < 0 || slot >= m.slots) return false;
        if (type === '') m.items.delete(slot);
        else m.items.set(slot, { type, count, damage, name, lore: loreCsv });
        return true;
    },
    containerSetTitle(id, title) {
        const m = menus.get(id);
        if (!m) return false;
        m.title = title;
        return true;
    },
    containerSetInteractive(id, on) {
        const m = menus.get(id);
        if (!m) return false;
        m.interactive = !!on;
        return true;
    },
    containerClose(id) { return menus.delete(id); },
    containerCloseAll() { menus.clear(); },
    containerIsOpen(id) { return menus.has(id); },
    containerGetIds() { return Array.from(menus.keys()); },
    containerPollClicks() {
        const out = queue.slice();
        queue.length = 0;
        return out;
    },
    containerClearClicks() { queue.length = 0; }
};

// ── 假玩家 / 指令 / mc / ll ──
const players = new Map();
const handlers = {};
const commands = new Map();   // name -> { desc, perm, overloads, cb, setup }
const outbox = [];            // 指令输出（success/error）

function makePlayer(name) {
    const p = {
        realName: name,
        msgs: [],
        given: [],
        sendMessage(m) { p.msgs.push(m); log.push(`[chat→${name}] ${m}`); },
        giveItem(item) { p.given.push(item); }
    };
    players.set(name, p);
    return p;
}

global.PermType = { Any: 0, Member: 1, GameMasters: 2, Admin: 3, Host: 4 };
global.ll = {
    import(ns, name) {
        if (ns !== 'HologramLib') return null;
        return containerApi[name] || null;
    },
    registerPlugin() {}
};
global.mc = {
    listen(ev, fn) { handlers[ev] = fn; return true; },
    getPlayer(name) { return players.get(name) || null; },
    newItem(type, count) { return { type, count }; },
    newCommand(name, desc, perm) {
        const rec = { desc, perm, overloads: [], cb: null, setup: false };
        commands.set(name, rec);
        return {
            overload(params) { rec.overloads.push(params); },
            setCallback(cb) { rec.cb = cb; },
            setup() { rec.setup = true; }
        };
    }
};
global.logger = { info: mkLogger('INFO'), warn: mkLogger('WARN'), error: mkLogger('ERROR') };

// ── 载入脚本（它自己会 start()）──
const src = fs.readFileSync(__dirname + '/HoloBoxUITest.js', 'utf8');
new Function(src)();

// 指令输出收集器
function makeOut() {
    return {
        success(m) { outbox.push('success: ' + m); log.push('[cmd✓] ' + m); },
        error(m) { outbox.push('error: ' + m); log.push('[cmd✗] ' + m); }
    };
}

// 触发一个指令，返回它的返回值
function callCmd(name, player) {
    const rec = commands.get(name);
    if (!rec || !rec.cb) { log.push('[ERROR] 指令未注册: ' + name); return false; }
    const origin = player ? { player: player, isPlayer: true } : { player: null, isPlayer: false };
    return rec.cb({ name }, origin, makeOut(), {});
}

function tick(n) { for (let i = 0; i < n; i++) handlers.onTick(); }
function click(player, menuId, slot, closed) {
    // 库的真实顺序: handleContainerClose 先摘菜单, 再 dispatch 事件
    if (closed) menus.delete(menuId);
    queue.push(`player=${player} menuId=${menuId} slot=${slot} closed=${closed ? 1 : 0}`);
    tick(4);   // 每 4 tick 轮询一次
}

// ── 场景 ──
const p = makePlayer('Steve');

console.log('--- 0) 指令注册 ---');
const names = Array.from(commands.keys());
console.log('  注册的指令 =', JSON.stringify(names));
console.log('  都 setup 过 =', names.every((n) => commands.get(n).setup));
console.log('  都是无参重载 =', names.every((n) => commands.get(n).overloads.length === 1 &&
    Array.isArray(commands.get(n).overloads[0]) && commands.get(n).overloads[0].length === 0));

console.log('--- 1) /boxtest（只跑断言）---');
callCmd('boxtest', p);
console.log('  菜单数 =', menus.size, '（应为 0：/boxtest 结尾会 close）');

console.log('--- 2) /boxui（开界面 + 摆按钮）---');
callCmd('boxui', p);
const id = containerApi.containerGetIds()[0];
console.log('  menuId =', id, ' 标题 =', menus.get(id).title, ' 格子数 =', menus.get(id).items.size);

console.log('--- 3) 点功能按钮 ---');
click('Steve', id, 4, false);
console.log('  slot4 物品 =', JSON.stringify(menus.get(id).items.get(4)));
click('Steve', id, 8, false);
console.log('  标题 =', menus.get(id).title);
click('Steve', id, 13, false);
console.log('  interactive =', menus.get(id).interactive);
click('Steve', id, 22, false);
console.log('  Steve 收到 =', JSON.stringify(p.given));
click('Steve', id, 26, false);
console.log('  标题 =', menus.get(id).title);

console.log('--- 4) 关闭回传 ---');
click('Steve', id, -1, true);
console.log('  菜单数 =', menus.size, '（应为 0）');

console.log('--- 5) 布局外格子 + 未知菜单（只应告警）---');
callCmd('boxui', p);
const id2 = containerApi.containerGetIds()[0];
click('Steve', id2, 30, false);
click('Steve', 99999, 0, false);

console.log('--- 6) /boxstop ---');
callCmd('boxstop', p);
console.log('  菜单数 =', menus.size, '（应为 0）');

console.log('--- 6.5) 库回传 menuId 恒为 0（历史 bug 的回归用例）---');
callCmd('boxui', p);
const idBad = containerApi.containerGetIds()[0];
click('Steve', 0, 4, false);          // 故意用 0 当 menuId
console.log('  slot4 是否仍被处理 =', !!menus.get(idBad).items.get(4));
callCmd('boxstop', p);

console.log('--- 7) 控制台执行（origin.player = null）---');
callCmd('boxui', null);
console.log('  最后一条输出 =', outbox[outbox.length - 1]);

console.log('--- 8) 缺前置时的行为（重新加载一个实例）---');
const missing = new Set(['containerSetInteractive', 'containerCloseAll']);
global.ll.import = (ns, name) => (missing.has(name) ? null : (containerApi[name] || null));
for (const k in handlers) delete handlers[k];
commands.clear();
log.push('--- 新实例 ---');
new Function(src)();
callCmd('boxtest', p);
console.log('  最后一条输出 =', outbox[outbox.length - 1]);

// ── 结果统计 ──
const pass = log.filter((l) => l.includes('[PASS]')).length;
const fail = log.filter((l) => l.includes('[FAIL]')).length;
console.log('\n=== 汇总 ===');
console.log('PASS 行:', pass, ' FAIL 行:', fail, '（第 6.5 步的 C9 是刻意构造的，应为 1 条）');
const problems = log.filter((l) => l.includes('[FAIL]'));
if (problems.length) {
    console.log('失败明细:');
    problems.forEach((l) => console.log('  ', l));
}
const warns = log.filter((l) => l.startsWith('[WARN]'));
console.log('WARN 行:', warns.length);
warns.slice(0, 8).forEach((l) => console.log('  ', l));
// 第 8 步故意制造"缺前置"，那条 ERROR 是预期内的
const errs = log.filter((l) => l.startsWith('[ERROR]') && !l.includes('[FAIL]') && !l.includes('未就绪'));
if (errs.length) { console.log('未预期 ERROR:'); errs.forEach((l) => console.log('  ', l)); }
