// tools/shim_test.js —— lse/HologramLib.js 的离线自测（不需要服务器）
//
//   node tools/shim_test.js
//
// 验的是导入层自己的行为：懒绑定、参数转发、缺函数/缺 lrca 的降级、解构、清单一致。

const path = require("path");
const assert = require("assert");

let imports = 0;
let calls = [];
let warnings = [];

// 假 lrca：只有 manifest 里前两个函数"存在"
global.logger = { warn: (m) => warnings.push(m) };

function fakeImport(ns, name) {
  imports += 1;
  assert.strictEqual(ns, "HologramLib", "命名空间写错");
  if (name === "holoInfo") {
    return () => JSON.stringify({ apiVersion: "1.32.0", apiVersionNumber: 0x012600, abiStamp: 1 });
  }
  if (name === "itemActorSpawn" || name === "formClose" || name === "playerHideSetHidden") {
    return (...args) => {
      calls.push([name, args]);
      return name === "itemActorSpawn" ? 7 : true;
    };
  }
  return null; // 其余按"库里没有"处理
}

global.ll = { import: fakeImport };

const HL = require(path.join(__dirname, "..", "HologramLib.js"));

let pass = 0;
function ok(label, fn) {
  try {
    fn();
    pass += 1;
    console.log("  PASS  " + label);
  } catch (err) {
    console.log("  FAIL  " + label + "  →  " + err.message);
    process.exitCode = 1;
  }
}

console.log("[1] 懒绑定：只在第一次取用时 import");
{
  const before = imports;
  const spawn = HL.itemActorSpawn;
  assert.strictEqual(typeof spawn, "function");
  // 第一次取用会多做一次 holoInfo 探测（版本核对），所以是 1~2 次
  assert.ok(imports - before >= 1 && imports - before <= 2, `第一次取用 import 了 ${imports - before} 次`);
  const id = spawn("minecraft:diamond", 0, 1, 100.5, 65, 100.5, 0, 0, 0.4, 0);
  assert.strictEqual(id, 7);
  assert.deepStrictEqual(calls[0][1].length, 10, "参数应该原样转发");
  ok("首个函数 import 一次并转发参数", () => {});
}

console.log("[2] 缓存：再取同一个函数不再 import");
{
  const before = imports;
  const a = HL.itemActorSpawn;
  const b = HL.itemActorSpawn;
  assert.strictEqual(a, b, "同一个名字应该拿到同一个函数");
  assert.strictEqual(imports, before, "不应该重复 import");
  ok("重复取用零开销", () => {});
}

console.log("[3] 未使用到的函数不占绑定");
{
  // 只用到两个函数 → import 次数应该远小于 337
  assert.ok(imports <= 5, `import ${imports} 次，超过预期`);
  assert.ok(HL.list().length >= 330, "清单里应该有一整批导出");
  ok(`用了 2 个函数，实际 import ${imports} 次（全量是 ${HL.list().length}）`, () => {});
}

console.log("[4] 存在但运行时缺失的函数 → 安全空操作 + 只警告一次");
{
  const before = warnings.length;
  const fn = HL.holoCreate;
  assert.strictEqual(typeof fn, "function", "缺函数也要返回函数, 不能让脚本加载期就炸");
  assert.strictEqual(fn(1, 2, 3), undefined);
  HL.holoCreate(4, 5, 6);
  assert.strictEqual(warnings.length, before + 1, "同一个函数只警告一次");
  ok("缺函数降级为空操作", () => {});
}

console.log("[5] 拼错的名字 → undefined + 警告");
{
  const before = warnings.length;
  assert.strictEqual(HL.itemActorSpwan, undefined, "未知名字应该拿到 undefined");
  assert.strictEqual(warnings.length, before + 1);
  assert.ok(warnings[warnings.length - 1].includes("未知导出"), warnings.slice(-1)[0]);
  ok("拼错提示", () => {});
}

console.log("[6] 元信息：has / list / apiVersion");
{
  assert.strictEqual(HL.has("itemActorSpawn"), true);
  assert.strictEqual(HL.has("noSuchThing"), false);
  assert.ok(/^\d+\.\d+\.\d+$/.test(HL.apiVersion), "apiVersion 应该是 x.y.z");
  assert.ok(HL.list().includes("locatorBarSet"));
  ok(`apiVersion=${HL.apiVersion}, list=${HL.list().length}`, () => {});
}

console.log("[7] 解构一次 = 完全避开 Proxy");
{
  const { formClose } = HL;
  const before = imports;
  assert.strictEqual(formClose("Steve"), true);
  assert.strictEqual(imports, before, "解构之后调用不应该再 import");
  ok("解构后调用不再经过导入", () => {});
}

console.log("[8] CLI 读法：exports.json 与 JS 清单一致");
{
  const manifest = require(path.join(__dirname, "..", "exports.json"));
  const names = new Set(HL.list());
  const missing = manifest.exports.filter((e) => !names.has(e.name));
  assert.strictEqual(missing.length, 0, "exports.json 里有 JS 清单没有的: " + missing.slice(0, 3).map(e => e.name));
  assert.strictEqual(manifest.count, HL.list().length);
  ok(`exports.json 与清单一致（${manifest.count} 条）`, () => {});
}

console.log("[9] 没有 lrca 时整套导入层仍然可用（全是空操作）");
{
  delete global.ll;
  delete require.cache[require.resolve(path.join(__dirname, "..", "HologramLib.js"))];
  const HL2 = require(path.join(__dirname, "..", "HologramLib.js"));
  assert.strictEqual(typeof HL2.itemActorSpawn, "function");
  assert.strictEqual(HL2.itemActorSpawn("x"), undefined);
  ok("缺 lrca 降级为空操作", () => {});
}

console.log("[10] 按类分组（GMLIB 风格）：类.方法 可用");
{
  global.ll = { import: fakeImport }; // [9] 把 global.ll 删了, 这里恢复
  const { ItemActor, Visibility, LocatorBar } = HL;
  assert.ok(ItemActor && Visibility && LocatorBar, "三个分组应该都能解构出来");
  assert.strictEqual(typeof ItemActor.spawn, "function");
  assert.strictEqual(typeof Visibility.setHidden, "function");
  assert.strictEqual(typeof LocatorBar.set, "function");
  assert.strictEqual(ItemActor.spawn("minecraft:diamond"), 7, "分组方法要转到真导出");
  assert.strictEqual(Visibility.setHidden("Steve", true), true);
  assert.ok(Object.keys(HL.groups).length >= 19, `分组数 ${Object.keys(HL.groups).length}`);
  ok(`分组 ${Object.keys(HL.groups).length} 个，类.方法可用`, () => {});
}

console.log("[11] 分组对象冻结（不会被误改）");
{
  const before = HL.groups.ItemActor.spawn;
  try { HL.groups.ItemActor.spawn = null; } catch (_) { /* 严格模式抛错, 非严格静默 */ }
  assert.strictEqual(HL.groups.ItemActor.spawn, before, "分组方法不应该被替换");
  ok("分组对象只读", () => {});
}


console.log(`\nSHIM-TEST: ${pass} 项通过${process.exitCode ? "（有失败）" : ""}`);
