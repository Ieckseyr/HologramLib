// HologramLib.js —— 脚本侧导入层（自动生成，不要手改）
//
// 生成器: tools/gen_lse_shim.py    目标版本: API 1.32.0 / ABI 戳 1
//
// 用法一（推荐，按类分组 —— 同 GMLIB-LegacyRemoteCallApi 的写法）:
//
//   const { ItemActor, Visibility, LocatorBar } = require("./HologramLib.js");
//   ItemActor.spawn("minecraft:diamond", 0, 1, 100.5, 65, 100.5, 0, 0, 0.4, 0);
//   Visibility.setHidden("Steve", true);
//
// 用法二（平铺，名字就是导出名）:
//
//   const HL = require("./HologramLib.js");
//   HL.itemActorSpawn("minecraft:diamond", 0, 1, 100.5, 65, 100.5, 0, 0, 0.4, 0);
//
// 设计要点:
//   · ll.import 只在**第一次取某个函数**时做一次, 之后用缓存 —— 脚本用几个就绑几个,
//     不会像"开局循环导入全部导出"那样付几百次绑定;
//   · 库里没有 lrca / 函数名写错 → 拿到的是**安全空操作**（返回 undefined, 记一条日志）, 
//     不会让整个脚本在加载期就抛异常;
//   · 想完全避开 Proxy 的那一点点开销, 在启动时解构一次即可:
//         const { holoCreate, holoSetLineText } = require("./HologramLib.js");
'use strict';

const NAMESPACE = "HologramLib";
const API_VERSION = "1.32.0";
const ABI_STAMP = "1";

const NAMES = [
  "containerBlockEvent",
  "containerClearClicks",
  "containerClose",
  "containerCloseAll",
  "containerGetIds",
  "containerIsOpen",
  "containerOpen",
  "containerPollClicks",
  "containerSetInteractive",
  "containerSetItem",
  "containerSetTitle",
  "entityClearMobProperties",
  "entityClearPlayerAppearance",
  "entityClearPlayerRotation",
  "entityClearPlayerRotations",
  "entityClearRide",
  "entityClearSpawns",
  "entityClearVisiblePlayers",
  "entityCreate",
  "entityCreateAdvanced",
  "entityCreateRandom",
  "entityCreateWithId",
  "entityDestroy",
  "entityDestroyAll",
  "entityExists",
  "entityFindNearest",
  "entityGetAllIds",
  "entityGetInfo",
  "entityIsIdUsed",
  "entityPlayAnimation",
  "entityPlayAnimationTo",
  "entityPollSpawns",
  "entityScaleBy",
  "entitySetColorIndex",
  "entitySetEnabled",
  "entitySetEquipmentSlot",
  "entitySetFlags",
  "entitySetIdentifier",
  "entitySetInvisible",
  "entitySetMarkVariant",
  "entitySetMobProperty",
  "entitySetNametag",
  "entitySetPlayerEquipmentSlot",
  "entitySetPlayerNametag",
  "entitySetPlayerRotation",
  "entitySetPlayerScale",
  "entitySetPose",
  "entitySetPosition",
  "entitySetRideEntity",
  "entitySetRidePlayer",
  "entitySetRotation",
  "entitySetScale",
  "entitySetVariant",
  "entitySetViewDistance",
  "entitySetVisiblePlayer",
  "entitySetVisiblePlayers",
  "entityStopAnimation",
  "entityStopAnimationTo",
  "fakeInvApply",
  "fakeInvClear",
  "fakeInvClearAll",
  "fakeInvClearClicks",
  "fakeInvGetPlayers",
  "fakeInvIsActive",
  "fakeInvPollClicks",
  "fakeInvRefresh",
  "fakeInvSetSlot",
  "formClose",
  "formCloseAll",
  "ghostClearInteractions",
  "ghostPollInteractions",
  "gradientCreate",
  "gradientDestroy",
  "gradientDestroyAll",
  "gradientDraw",
  "gradientDrawToDimension",
  "gradientRemove",
  "gradientSetColor",
  "gradientSetEndpoints",
  "gradientSetGradient",
  "gradientSetRainbow",
  "gradientTick",
  "holoAddLine",
  "holoApiVersion",
  "holoClearBackgroundColor",
  "holoClearFollowPlayer",
  "holoClearLines",
  "holoClearRotation",
  "holoCreate",
  "holoDestroy",
  "holoDestroyAll",
  "holoDraw",
  "holoDrawToDimension",
  "holoDrawToPlayer",
  "holoGetLineCount",
  "holoInfo",
  "holoRefresh",
  "holoRemove",
  "holoRemoveLine",
  "holoSetBackgroundColor",
  "holoSetColor",
  "holoSetDepthTest",
  "holoSetDimension",
  "holoSetFollowPlayer",
  "holoSetLineParseVariables",
  "holoSetLinePool",
  "holoSetLineText",
  "holoSetLocation",
  "holoSetRotation",
  "holoSetScale",
  "itemActorDestroy",
  "itemActorDestroyAll",
  "itemActorExists",
  "itemActorGetIds",
  "itemActorMoveTo",
  "itemActorSetMotion",
  "itemActorSpawn",
  "itemActorTake",
  "itemDetailHide",
  "itemDetailShow",
  "itemDisplayClearAnim",
  "itemDisplayClearVisiblePlayers",
  "itemDisplayCreate",
  "itemDisplayCreateAdvanced",
  "itemDisplayCreateFull",
  "itemDisplayCreateRandom",
  "itemDisplayCreateSeamless",
  "itemDisplayCreateWithId",
  "itemDisplayDestroy",
  "itemDisplayDestroyAll",
  "itemDisplayEmitForEntity",
  "itemDisplayExists",
  "itemDisplayFindNearest",
  "itemDisplayFollow",
  "itemDisplayGetAllIds",
  "itemDisplayGetInfo",
  "itemDisplayIsIdUsed",
  "itemDisplayRotateY",
  "itemDisplayScaleBy",
  "itemDisplayScaleTo",
  "itemDisplaySetAnim",
  "itemDisplaySetBaseOffset",
  "itemDisplaySetEnabled",
  "itemDisplaySetExtend",
  "itemDisplaySetGlint",
  "itemDisplaySetHitbox",
  "itemDisplaySetItem",
  "itemDisplaySetItemWithNbt",
  "itemDisplaySetMode",
  "itemDisplaySetOffset",
  "itemDisplaySetOffsetLive",
  "itemDisplaySetPosition",
  "itemDisplaySetPreset",
  "itemDisplaySetRotation",
  "itemDisplaySetRotationLive",
  "itemDisplaySetScale",
  "itemDisplaySetScaleLive",
  "itemDisplaySetStretch",
  "itemDisplaySetStretchLive",
  "itemDisplaySetTransformLive",
  "itemDisplaySetViewDistance",
  "itemDisplaySetVisiblePlayer",
  "itemDisplaySetVisiblePlayers",
  "itemDisplayUnfollow",
  "locatorBarClear",
  "locatorBarCount",
  "locatorBarHideFromOthers",
  "locatorBarIsHiddenFromOthers",
  "locatorBarRemove",
  "locatorBarSet",
  "npcDialogClearClicks",
  "npcDialogClose",
  "npcDialogCloseAll",
  "npcDialogGetIds",
  "npcDialogIsOpen",
  "npcDialogOpen",
  "npcDialogPollClicks",
  "npcDialogSetAvatar",
  "npcDialogUpdate",
  "particleCreateBox",
  "particleCreateBoxFaces",
  "particleCreateLine",
  "particleCreatePlane",
  "particleCreatePoint",
  "particleCreatePoly",
  "particleCreateRect",
  "particleDestroy",
  "particleFollow",
  "particleGetInfo",
  "particleMoveBy",
  "particleMoveTo",
  "particleSetEffect",
  "particleSetInterval",
  "particleSetLifetime",
  "particleSetPos",
  "particleSetRot",
  "particleSetScale",
  "particleSetViewDistance",
  "particleSetVisible",
  "particleSpin",
  "particleUnfollow",
  "playerHideGetMask",
  "playerHideIsHidden",
  "playerHideRestore",
  "playerHideSetHidden",
  "playerHideSetMask",
  "playerNpcCaptureSkin",
  "playerNpcClearPlayerRotation",
  "playerNpcClearPlayerRotations",
  "playerNpcClearRide",
  "playerNpcClearSpawns",
  "playerNpcClearVisiblePlayers",
  "playerNpcCreate",
  "playerNpcCreateEx",
  "playerNpcCreateRandom",
  "playerNpcCreateWithId",
  "playerNpcDestroy",
  "playerNpcDestroyAll",
  "playerNpcExists",
  "playerNpcGetAllIds",
  "playerNpcGetDebugInfo",
  "playerNpcGetSkinBlobB64",
  "playerNpcGetSkinIds",
  "playerNpcHasSkin",
  "playerNpcImportSkins",
  "playerNpcInjectSkin",
  "playerNpcInjectSkinAll",
  "playerNpcIsIdUsed",
  "playerNpcPlayAnimation",
  "playerNpcPlayAnimationTo",
  "playerNpcPlayEmote",
  "playerNpcPollSpawns",
  "playerNpcRegisterImageSkin",
  "playerNpcRegisterSkin",
  "playerNpcRegisterSkinData",
  "playerNpcRegisterSkinDataOpaque",
  "playerNpcRegisterSkinFromBlobB64",
  "playerNpcRegisterSkinFromBlobFile",
  "playerNpcSetEnabled",
  "playerNpcSetEquipment",
  "playerNpcSetNametag",
  "playerNpcSetPlayerRotation",
  "playerNpcSetPos",
  "playerNpcSetPosLight",
  "playerNpcSetPose",
  "playerNpcSetRideEntity",
  "playerNpcSetRidePlayer",
  "playerNpcSetRideUniqueId",
  "playerNpcSetRotLight",
  "playerNpcSetRotation",
  "playerNpcSetScale",
  "playerNpcSetSkin",
  "playerNpcSetViewDistance",
  "playerNpcSetVisiblePlayer",
  "playerNpcSetVisiblePlayers",
  "playerNpcStopAnimation",
  "playerNpcStopAnimationTo",
  "playerNpcUnregisterSkin",
  "sensingDeviceCode",
  "sensingDeviceOf",
  "sensingIsTouch",
  "shapeClearBackgroundColor",
  "shapeClearRotation",
  "shapeCreateArrow",
  "shapeCreateBox",
  "shapeCreateCircle",
  "shapeCreateLine",
  "shapeCreateSphere",
  "shapeCreateText",
  "shapeDestroy",
  "shapeDestroyAll",
  "shapeDestroyBatch",
  "shapeDraw",
  "shapeDrawBatch",
  "shapeDrawToDimension",
  "shapeDrawToPlayer",
  "shapeExists",
  "shapeFindTextByLocation",
  "shapeFindTextByLocationAndContent",
  "shapeGetAllShapeIds",
  "shapeGetColor",
  "shapeGetLocation",
  "shapeGetRotation",
  "shapeGetShapeType",
  "shapeGetText",
  "shapeRemove",
  "shapeRemoveToDimension",
  "shapeRemoveToPlayer",
  "shapeSetBackgroundColor",
  "shapeSetColor",
  "shapeSetDepthTest",
  "shapeSetDimension",
  "shapeSetDuration",
  "shapeSetLocation",
  "shapeSetRotation",
  "shapeSetScale",
  "shapeSetText",
  "shapeUpdate",
  "shapeUpdateToDimension",
  "shapeUpdateToPlayer",
  "sulfurCreate",
  "sulfurDestroy",
  "sulfurDestroyAll",
  "sulfurExists",
  "sulfurGetEntityId",
  "sulfurGetIds",
  "sulfurSetArchetype",
  "sulfurSetBlock",
  "sulfurSetInvisible",
  "sulfurSetScale",
  "tradeAddOffer",
  "tradeClose",
  "tradeCloseAll",
  "tradeGetIds",
  "tradeIsOpen",
  "tradeIsSettle",
  "tradeOpen",
  "tradePollSettlements",
  "tradeSetSettle",
  "tradeSetTier",
  "viewBlock",
  "viewClearAll",
  "viewClearBlock",
  "viewClearEntity",
  "viewClearId",
  "viewDescribe",
  "viewEcho",
  "viewEntity",
  "viewItemSpawnHooked",
  "viewMetaProbe",
  "viewPollSpawnEvents",
  "viewSelf",
  "viewSelfClear",
  "viewSelfHide",
  "viewSelfName",
  "viewSelfSkin",
  "viewSelfType",
  "viewTargetType",
  "viewUniqueIdOf"
];

const NAME_SET = new Set(NAMES);
const bound = Object.create(null);
let warned = Object.create(null);

function logOnce(key, message) {
  if (warned[key]) return;
  warned[key] = true;
  try {
    if (typeof logger !== "undefined" && logger.warn) logger.warn("[HologramLib] " + message);
    else console.warn("[HologramLib] " + message);
  } catch (_) {}
}

let versionChecked = false;

// 第一次真正绑定时, 拿 DLL 的 holoInfo() 跟生成时的版本对一次（只付一次 RPC）
function ensureVersion() {
  if (versionChecked) return;
  versionChecked = true;
  if (typeof ll === "undefined" || typeof ll.import !== "function") return;
  try {
    const probe = ll.import(NAMESPACE, "holoInfo");
    if (typeof probe !== "function") {
      logOnce("no-info", "这版库没有 holoInfo（比导入层旧）—— 缺的函数会降级为空操作");
      return;
    }
    const info = JSON.parse(probe());
    if (info && info.apiVersion && info.apiVersion !== API_VERSION) {
      logOnce(
        "api-mismatch",
        "导入层按 API " + API_VERSION + " 生成，DLL 是 " + info.apiVersion + " —— 对不上的函数会降级为空操作"
      );
    }
    if (info && info.abiStamp !== undefined && String(info.abiStamp) !== ABI_STAMP) {
      logOnce("stamp-mismatch", "ABI 戳不一致（导入层 " + ABI_STAMP + " / DLL " + info.abiStamp + "），建议重新生成导入层");
    }
  } catch (_) {
    /* 探测失败就不管，正常绑定照样走 */
  }
}

function importFn(name) {
  // lrca 没加载 / ll.import 不可用 → 安全空操作
  if (typeof ll === "undefined" || typeof ll.import !== "function") {
    logOnce("no-lrca", "ll.import 不可用（缺 lrca?）—— 调用 " + name + " 会被忽略");
    return function noop() {};
  }
  let fn = null;
  try {
    fn = ll.import(NAMESPACE, name);
  } catch (err) {
    logOnce("throw:" + name, "导入 " + name + " 抛异常: " + err);
  }
  if (typeof fn !== "function") {
    logOnce("missing:" + name, "库里没有 " + name + "（版本不匹配? API " + API_VERSION + "）");
    return function noop() {};
  }
  return fn;
}

function bind(name) {
  if (bound[name]) return bound[name];
  ensureVersion();
  const fn = importFn(name);
  bound[name] = fn;
  return fn;
}

// ── 按类分组（GMLIB 风格：类.方法）──
// 每组一个普通对象, 方法同样是懒绑定; 组对象本身是常量, 解构之后再没有 Proxy 开销。
function makeGroup(methods) {
  const group = Object.create(null);
  for (const method in methods) {
    const exportName = methods[method];
    group[method] = function () {
      return bind(exportName).apply(null, arguments);
    };
  }
  return Object.freeze(group);
}

const GROUP_MEMBERS = {
  Container: {blockEvent:"containerBlockEvent",clearClicks:"containerClearClicks",close:"containerClose",closeAll:"containerCloseAll",getIds:"containerGetIds",isOpen:"containerIsOpen",open:"containerOpen",pollClicks:"containerPollClicks",setInteractive:"containerSetInteractive",setItem:"containerSetItem",setTitle:"containerSetTitle"},
  Entity: {clearMobProperties:"entityClearMobProperties",clearPlayerAppearance:"entityClearPlayerAppearance",clearPlayerRotation:"entityClearPlayerRotation",clearPlayerRotations:"entityClearPlayerRotations",clearRide:"entityClearRide",clearSpawns:"entityClearSpawns",clearVisiblePlayers:"entityClearVisiblePlayers",create:"entityCreate",createAdvanced:"entityCreateAdvanced",createRandom:"entityCreateRandom",createWithId:"entityCreateWithId",destroy:"entityDestroy",destroyAll:"entityDestroyAll",exists:"entityExists",findNearest:"entityFindNearest",getAllIds:"entityGetAllIds",getInfo:"entityGetInfo",isIdUsed:"entityIsIdUsed",playAnimation:"entityPlayAnimation",playAnimationTo:"entityPlayAnimationTo",pollSpawns:"entityPollSpawns",scaleBy:"entityScaleBy",setColorIndex:"entitySetColorIndex",setEnabled:"entitySetEnabled",setEquipmentSlot:"entitySetEquipmentSlot",setFlags:"entitySetFlags",setIdentifier:"entitySetIdentifier",setInvisible:"entitySetInvisible",setMarkVariant:"entitySetMarkVariant",setMobProperty:"entitySetMobProperty",setNametag:"entitySetNametag",setPlayerEquipmentSlot:"entitySetPlayerEquipmentSlot",setPlayerNametag:"entitySetPlayerNametag",setPlayerRotation:"entitySetPlayerRotation",setPlayerScale:"entitySetPlayerScale",setPose:"entitySetPose",setPosition:"entitySetPosition",setRideEntity:"entitySetRideEntity",setRidePlayer:"entitySetRidePlayer",setRotation:"entitySetRotation",setScale:"entitySetScale",setVariant:"entitySetVariant",setViewDistance:"entitySetViewDistance",setVisiblePlayer:"entitySetVisiblePlayer",setVisiblePlayers:"entitySetVisiblePlayers",stopAnimation:"entityStopAnimation",stopAnimationTo:"entityStopAnimationTo"},
  FakeInventory: {apply:"fakeInvApply",clear:"fakeInvClear",clearAll:"fakeInvClearAll",clearClicks:"fakeInvClearClicks",getPlayers:"fakeInvGetPlayers",isActive:"fakeInvIsActive",pollClicks:"fakeInvPollClicks",refresh:"fakeInvRefresh",setSlot:"fakeInvSetSlot"},
  Form: {close:"formClose",closeAll:"formCloseAll"},
  Ghost: {clearInteractions:"ghostClearInteractions",pollInteractions:"ghostPollInteractions"},
  GradientLine: {create:"gradientCreate",destroy:"gradientDestroy",destroyAll:"gradientDestroyAll",draw:"gradientDraw",drawToDimension:"gradientDrawToDimension",remove:"gradientRemove",setColor:"gradientSetColor",setEndpoints:"gradientSetEndpoints",setGradient:"gradientSetGradient",setRainbow:"gradientSetRainbow",tick:"gradientTick"},
  Hologram: {addLine:"holoAddLine",apiVersion:"holoApiVersion",clearBackgroundColor:"holoClearBackgroundColor",clearFollowPlayer:"holoClearFollowPlayer",clearLines:"holoClearLines",clearRotation:"holoClearRotation",create:"holoCreate",destroy:"holoDestroy",destroyAll:"holoDestroyAll",draw:"holoDraw",drawToDimension:"holoDrawToDimension",drawToPlayer:"holoDrawToPlayer",getLineCount:"holoGetLineCount",info:"holoInfo",refresh:"holoRefresh",remove:"holoRemove",removeLine:"holoRemoveLine",setBackgroundColor:"holoSetBackgroundColor",setColor:"holoSetColor",setDepthTest:"holoSetDepthTest",setDimension:"holoSetDimension",setFollowPlayer:"holoSetFollowPlayer",setLineParseVariables:"holoSetLineParseVariables",setLinePool:"holoSetLinePool",setLineText:"holoSetLineText",setLocation:"holoSetLocation",setRotation:"holoSetRotation",setScale:"holoSetScale"},
  ItemActor: {destroy:"itemActorDestroy",destroyAll:"itemActorDestroyAll",exists:"itemActorExists",getIds:"itemActorGetIds",moveTo:"itemActorMoveTo",setMotion:"itemActorSetMotion",spawn:"itemActorSpawn",take:"itemActorTake"},
  ItemDetail: {hide:"itemDetailHide",show:"itemDetailShow"},
  ItemDisplay: {clearAnim:"itemDisplayClearAnim",clearVisiblePlayers:"itemDisplayClearVisiblePlayers",create:"itemDisplayCreate",createAdvanced:"itemDisplayCreateAdvanced",createFull:"itemDisplayCreateFull",createRandom:"itemDisplayCreateRandom",createSeamless:"itemDisplayCreateSeamless",createWithId:"itemDisplayCreateWithId",destroy:"itemDisplayDestroy",destroyAll:"itemDisplayDestroyAll",emitForEntity:"itemDisplayEmitForEntity",exists:"itemDisplayExists",findNearest:"itemDisplayFindNearest",follow:"itemDisplayFollow",getAllIds:"itemDisplayGetAllIds",getInfo:"itemDisplayGetInfo",isIdUsed:"itemDisplayIsIdUsed",rotateY:"itemDisplayRotateY",scaleBy:"itemDisplayScaleBy",scaleTo:"itemDisplayScaleTo",setAnim:"itemDisplaySetAnim",setBaseOffset:"itemDisplaySetBaseOffset",setEnabled:"itemDisplaySetEnabled",setExtend:"itemDisplaySetExtend",setGlint:"itemDisplaySetGlint",setHitbox:"itemDisplaySetHitbox",setItem:"itemDisplaySetItem",setItemWithNbt:"itemDisplaySetItemWithNbt",setMode:"itemDisplaySetMode",setOffset:"itemDisplaySetOffset",setOffsetLive:"itemDisplaySetOffsetLive",setPosition:"itemDisplaySetPosition",setPreset:"itemDisplaySetPreset",setRotation:"itemDisplaySetRotation",setRotationLive:"itemDisplaySetRotationLive",setScale:"itemDisplaySetScale",setScaleLive:"itemDisplaySetScaleLive",setStretch:"itemDisplaySetStretch",setStretchLive:"itemDisplaySetStretchLive",setTransformLive:"itemDisplaySetTransformLive",setViewDistance:"itemDisplaySetViewDistance",setVisiblePlayer:"itemDisplaySetVisiblePlayer",setVisiblePlayers:"itemDisplaySetVisiblePlayers",unfollow:"itemDisplayUnfollow"},
  LocatorBar: {clear:"locatorBarClear",count:"locatorBarCount",hideFromOthers:"locatorBarHideFromOthers",isHiddenFromOthers:"locatorBarIsHiddenFromOthers",remove:"locatorBarRemove",set:"locatorBarSet"},
  NpcDialog: {clearClicks:"npcDialogClearClicks",close:"npcDialogClose",closeAll:"npcDialogCloseAll",getIds:"npcDialogGetIds",isOpen:"npcDialogIsOpen",open:"npcDialogOpen",pollClicks:"npcDialogPollClicks",setAvatar:"npcDialogSetAvatar",update:"npcDialogUpdate"},
  Particle: {createBox:"particleCreateBox",createBoxFaces:"particleCreateBoxFaces",createLine:"particleCreateLine",createPlane:"particleCreatePlane",createPoint:"particleCreatePoint",createPoly:"particleCreatePoly",createRect:"particleCreateRect",destroy:"particleDestroy",follow:"particleFollow",getInfo:"particleGetInfo",moveBy:"particleMoveBy",moveTo:"particleMoveTo",setEffect:"particleSetEffect",setInterval:"particleSetInterval",setLifetime:"particleSetLifetime",setPos:"particleSetPos",setRot:"particleSetRot",setScale:"particleSetScale",setViewDistance:"particleSetViewDistance",setVisible:"particleSetVisible",spin:"particleSpin",unfollow:"particleUnfollow"},
  PlayerNpc: {captureSkin:"playerNpcCaptureSkin",clearPlayerRotation:"playerNpcClearPlayerRotation",clearPlayerRotations:"playerNpcClearPlayerRotations",clearRide:"playerNpcClearRide",clearSpawns:"playerNpcClearSpawns",clearVisiblePlayers:"playerNpcClearVisiblePlayers",create:"playerNpcCreate",createEx:"playerNpcCreateEx",createRandom:"playerNpcCreateRandom",createWithId:"playerNpcCreateWithId",destroy:"playerNpcDestroy",destroyAll:"playerNpcDestroyAll",exists:"playerNpcExists",getAllIds:"playerNpcGetAllIds",getDebugInfo:"playerNpcGetDebugInfo",getSkinBlobB64:"playerNpcGetSkinBlobB64",getSkinIds:"playerNpcGetSkinIds",hasSkin:"playerNpcHasSkin",importSkins:"playerNpcImportSkins",injectSkin:"playerNpcInjectSkin",injectSkinAll:"playerNpcInjectSkinAll",isIdUsed:"playerNpcIsIdUsed",playAnimation:"playerNpcPlayAnimation",playAnimationTo:"playerNpcPlayAnimationTo",playEmote:"playerNpcPlayEmote",pollSpawns:"playerNpcPollSpawns",registerImageSkin:"playerNpcRegisterImageSkin",registerSkin:"playerNpcRegisterSkin",registerSkinData:"playerNpcRegisterSkinData",registerSkinDataOpaque:"playerNpcRegisterSkinDataOpaque",registerSkinFromBlobB64:"playerNpcRegisterSkinFromBlobB64",registerSkinFromBlobFile:"playerNpcRegisterSkinFromBlobFile",setEnabled:"playerNpcSetEnabled",setEquipment:"playerNpcSetEquipment",setNametag:"playerNpcSetNametag",setPlayerRotation:"playerNpcSetPlayerRotation",setPos:"playerNpcSetPos",setPosLight:"playerNpcSetPosLight",setPose:"playerNpcSetPose",setRideEntity:"playerNpcSetRideEntity",setRidePlayer:"playerNpcSetRidePlayer",setRideUniqueId:"playerNpcSetRideUniqueId",setRotLight:"playerNpcSetRotLight",setRotation:"playerNpcSetRotation",setScale:"playerNpcSetScale",setSkin:"playerNpcSetSkin",setViewDistance:"playerNpcSetViewDistance",setVisiblePlayer:"playerNpcSetVisiblePlayer",setVisiblePlayers:"playerNpcSetVisiblePlayers",stopAnimation:"playerNpcStopAnimation",stopAnimationTo:"playerNpcStopAnimationTo",unregisterSkin:"playerNpcUnregisterSkin"},
  Sensing: {deviceCode:"sensingDeviceCode",deviceOf:"sensingDeviceOf",isTouch:"sensingIsTouch"},
  Shape: {clearBackgroundColor:"shapeClearBackgroundColor",clearRotation:"shapeClearRotation",createArrow:"shapeCreateArrow",createBox:"shapeCreateBox",createCircle:"shapeCreateCircle",createLine:"shapeCreateLine",createSphere:"shapeCreateSphere",createText:"shapeCreateText",destroy:"shapeDestroy",destroyAll:"shapeDestroyAll",destroyBatch:"shapeDestroyBatch",draw:"shapeDraw",drawBatch:"shapeDrawBatch",drawToDimension:"shapeDrawToDimension",drawToPlayer:"shapeDrawToPlayer",exists:"shapeExists",findTextByLocation:"shapeFindTextByLocation",findTextByLocationAndContent:"shapeFindTextByLocationAndContent",getAllShapeIds:"shapeGetAllShapeIds",getColor:"shapeGetColor",getLocation:"shapeGetLocation",getRotation:"shapeGetRotation",getShapeType:"shapeGetShapeType",getText:"shapeGetText",remove:"shapeRemove",removeToDimension:"shapeRemoveToDimension",removeToPlayer:"shapeRemoveToPlayer",setBackgroundColor:"shapeSetBackgroundColor",setColor:"shapeSetColor",setDepthTest:"shapeSetDepthTest",setDimension:"shapeSetDimension",setDuration:"shapeSetDuration",setLocation:"shapeSetLocation",setRotation:"shapeSetRotation",setScale:"shapeSetScale",setText:"shapeSetText",update:"shapeUpdate",updateToDimension:"shapeUpdateToDimension",updateToPlayer:"shapeUpdateToPlayer"},
  Sulfur: {create:"sulfurCreate",destroy:"sulfurDestroy",destroyAll:"sulfurDestroyAll",exists:"sulfurExists",getEntityId:"sulfurGetEntityId",getIds:"sulfurGetIds",setArchetype:"sulfurSetArchetype",setBlock:"sulfurSetBlock",setInvisible:"sulfurSetInvisible",setScale:"sulfurSetScale"},
  Trade: {addOffer:"tradeAddOffer",close:"tradeClose",closeAll:"tradeCloseAll",getIds:"tradeGetIds",isOpen:"tradeIsOpen",isSettle:"tradeIsSettle",open:"tradeOpen",pollSettlements:"tradePollSettlements",setSettle:"tradeSetSettle",setTier:"tradeSetTier"},
  View: {block:"viewBlock",clearAll:"viewClearAll",clearBlock:"viewClearBlock",clearEntity:"viewClearEntity",clearId:"viewClearId",describe:"viewDescribe",echo:"viewEcho",entity:"viewEntity",itemSpawnHooked:"viewItemSpawnHooked",metaProbe:"viewMetaProbe",pollSpawnEvents:"viewPollSpawnEvents",self:"viewSelf",selfClear:"viewSelfClear",selfHide:"viewSelfHide",selfName:"viewSelfName",selfSkin:"viewSelfSkin",selfType:"viewSelfType",targetType:"viewTargetType",uniqueIdOf:"viewUniqueIdOf"},
  Visibility: {getMask:"playerHideGetMask",isHidden:"playerHideIsHidden",restore:"playerHideRestore",setHidden:"playerHideSetHidden",setMask:"playerHideSetMask"}
};

const groups = {};
for (const groupName in GROUP_MEMBERS) {
  groups[groupName] = makeGroup(GROUP_MEMBERS[groupName]);
}

const api = {
  /** 全部分组（也可以直接解构到变量） */
  groups: groups,
  /** 库的 API 版本（如 "1.32.0"）；拿不到时是生成时的那个值 */
  apiVersion: API_VERSION,
  /** 虚表布局戳；消费方升级库时要重新编译（脚本不用管） */
  abiStamp: ABI_STAMP,
  /** 这个库到底导出了哪些函数（数组，来自源码） */
  list: function list() { return NAMES.slice(); },
  /** 有没有这个函数（不看运行时, 只看生成时的清单） */
  has: function has(name) { return NAME_SET.has(name); },
  /** 显式绑定一个函数（等价于取属性, 但可读性更好） */
  bind: bind,
};

// 每个分组也挂成 api 的自有属性, 于是下面两种写法都成立:
//   const { ItemActor } = require("./HologramLib.js");
//   const ItemActor = require("./HologramLib.js").ItemActor;
for (const groupName in groups) {
  api[groupName] = groups[groupName];
}

module.exports = new Proxy(api, {
  get: function (target, prop) {
    if (typeof prop !== "string") return target[prop];
    if (prop in target) return target[prop];
    if (!NAME_SET.has(prop)) {
      logOnce("unknown:" + prop, "未知导出 " + prop + "（拼错? 或者这版库还没有）");
      return undefined;
    }
    return bind(prop);
  },
  has: function (target, prop) {
    return typeof prop === "string" && (prop in target || NAME_SET.has(prop));
  },
});
