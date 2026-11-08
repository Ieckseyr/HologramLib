// HologramLib.js —— 脚本导入模块（由 tools/gen_lse_shim.py 生成）
//
// 版本: API 1.37.0 / ABI 戳 2
//
// 用法一：按分组调用
//
//   const { ItemActor, Visibility, LocatorBar } = require("mods/HologramLib/lib/HologramLib.js");
//   ItemActor.spawn("minecraft:diamond", 0, 1, 100.5, 65, 100.5, 0, 0, 0.4, 0);
//   Visibility.setHidden("Steve", true);
//
// 用法二：按函数原名调用
//
//   const HL = require("mods/HologramLib/lib/HologramLib.js");
//   HL.itemActorSpawn("minecraft:diamond", 0, 1, 100.5, 65, 100.5, 0, 0, 0.4, 0);
//
// 说明:
//   · 函数在首次使用时才与库绑定，之后复用。
//   · 缺少 lrca 或函数不存在时，调用被忽略并记录一条警告，脚本不会中断。
//   · HL.list() 返回全部函数名，HL.has(name) 判断是否存在。

'use strict';

const NAMESPACE = "HologramLib";
const API_VERSION = "1.37.0";
const ABI_STAMP = "2";

const NAMES = [
  "actorBlockClear",
  "actorBlockEvent",
  "actorBlockEventFor",
  "actorBlockEventForEntity",
  "actorBlockStats",
  "actorClearInteractions",
  "actorFreeze",
  "actorPlayAnimation",
  "actorPlayAnimationFull",
  "actorPlayDeath",
  "actorPlayEvent",
  "actorPlayEventFor",
  "actorPlayHurt",
  "actorPlayInstantDeath",
  "actorPlayTotem",
  "actorPollInteractions",
  "actorRuntimeIdOfEntity",
  "actorRuntimeIdOfPlayer",
  "actorSetPose",
  "actorUnblockEvent",
  "actorUnblockEventFor",
  "actorUnblockEventForEntity",
  "actorUnfreeze",
  "anchorAcquire",
  "anchorRelease",
  "blockBatchClear",
  "blockBatchProgress",
  "blockBatchSelfTest",
  "blockBatchSet",
  "blockBatchSetAll",
  "blockBatchSetNear",
  "blockBatchStats",
  "blockFakeClear",
  "blockFakeClearAll",
  "blockFakeResolve",
  "blockFakeSet",
  "blockFakeSetAll",
  "bossBarAdd",
  "bossBarAddOn",
  "bossBarCount",
  "bossBarGetInfo",
  "bossBarProbe",
  "bossBarRemove",
  "bossBarRemoveAll",
  "bossBarSetHealth",
  "bossBarSetStyle",
  "bossBarSetTitle",
  "containerBlockEvent",
  "containerClearClicks",
  "containerClose",
  "containerCloseAll",
  "containerDroppedClicks",
  "containerGetIds",
  "containerIsOpen",
  "containerOpen",
  "containerPollClicks",
  "containerPreview",
  "containerPreviewTitled",
  "containerSetInteractive",
  "containerSetItem",
  "containerSetItemEnchanted",
  "containerSetTitle",
  "containerUpdate",
  "easterEggCreeper",
  "easterEggCreeperDisplay",
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
  "fakeInvDroppedClicks",
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
  "hudProbe",
  "hudResetAll",
  "hudSetElement",
  "hudSetElements",
  "hudSetRaw",
  "itemActorDestroy",
  "itemActorDestroyAll",
  "itemActorExists",
  "itemActorGetIds",
  "itemActorMoveTo",
  "itemActorRuntimeId",
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
  "itemDisplayRuntimeId",
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
  "npcDialogClearClicks",
  "npcDialogClose",
  "npcDialogCloseAll",
  "npcDialogDroppedClicks",
  "npcDialogGetIds",
  "npcDialogIsOpen",
  "npcDialogOpen",
  "npcDialogPollClicks",
  "npcDialogSetAvatar",
  "npcDialogUpdate",
  "particleClearVisiblePlayers",
  "particleCreateBox",
  "particleCreateBoxFaces",
  "particleCreateLine",
  "particleCreatePlane",
  "particleCreatePoint",
  "particleCreatePoly",
  "particleCreateRect",
  "particleDestroy",
  "particleDestroyAll",
  "particleExists",
  "particleFollow",
  "particleGetAllIds",
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
  "particleSetVisiblePlayers",
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
  "playerNpcRegisterImageSkinSized",
  "playerNpcRegisterSkin",
  "playerNpcRegisterSkinData",
  "playerNpcRegisterSkinDataOpaque",
  "playerNpcRegisterSkinFromBlobB64",
  "playerNpcRegisterSkinFromBlobFile",
  "playerNpcRuntimeId",
  "playerNpcSetEnabled",
  "playerNpcSetEquipment",
  "playerNpcSetImageSize",
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
  "playerPlayCelebrate",
  "playerPlayOminousDrink",
  "playerRespawn",
  "playerRespawnAt",
  "playerRespawnState",
  "playerShowBadOmen",
  "playerShowEffect",
  "playerShowVillageHero",
  "sensingDeviceCode",
  "sensingDeviceOf",
  "sensingIsTouch",
  "shapeAttachEntity",
  "shapeClearAttachedEntity",
  "shapeClearBackgroundColor",
  "shapeClearRotation",
  "shapeCreateArrow",
  "shapeCreateBox",
  "shapeCreateCircle",
  "shapeCreateCone",
  "shapeCreateCylinder",
  "shapeCreateEllipsoid",
  "shapeCreateLine",
  "shapeCreatePyramid",
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
  "shapeSetMaxRenderDistance",
  "shapeSetRotation",
  "shapeSetScale",
  "shapeSetText",
  "shapeUpdate",
  "shapeUpdateToDimension",
  "shapeUpdateToPlayer",
  "sidebarIsShown",
  "sidebarLineCount",
  "sidebarRemove",
  "sidebarRemoveAll",
  "sidebarSet",
  "sidebarSetLine",
  "sidebarSetTitle",
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
  "swingClearEvents",
  "swingCount",
  "swingDropped",
  "swingLastBlock",
  "swingLastEvent",
  "swingLastTarget",
  "swingPollEvents",
  "swingReset",
  "swingTotal",
  "tradeAddOffer",
  "tradeClose",
  "tradeCloseAll",
  "tradeDroppedSettlements",
  "tradeGetIds",
  "tradeIsOpen",
  "tradeIsSettle",
  "tradeOpen",
  "tradePollSettlements",
  "tradeSetSettle",
  "tradeSetTier",
  "tradeUpdate",
  "viewAlign",
  "viewBlock",
  "viewClearAll",
  "viewClearBlock",
  "viewClearEntity",
  "viewClearId",
  "viewDescribe",
  "viewEcho",
  "viewEntity",
  "viewEntityInfo",
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
  "viewUniqueIdOf",
  "wireResetStats",
  "wireStats",
  "wireTraceActive",
  "wireTraceClear",
  "wireTraceCount",
  "wireTraceDump",
  "wireTraceMark",
  "wireTraceStart",
  "wireTraceStop"
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
  ActorEffect: {blockClear:"actorBlockClear",blockEvent:"actorBlockEvent",blockEventFor:"actorBlockEventFor",blockEventForEntity:"actorBlockEventForEntity",blockStats:"actorBlockStats",clearInteractions:"actorClearInteractions",freeze:"actorFreeze",playAnimation:"actorPlayAnimation",playAnimationFull:"actorPlayAnimationFull",playDeath:"actorPlayDeath",playEvent:"actorPlayEvent",playEventFor:"actorPlayEventFor",playHurt:"actorPlayHurt",playInstantDeath:"actorPlayInstantDeath",playTotem:"actorPlayTotem",pollInteractions:"actorPollInteractions",runtimeIdOfEntity:"actorRuntimeIdOfEntity",runtimeIdOfPlayer:"actorRuntimeIdOfPlayer",setPose:"actorSetPose",unblockEvent:"actorUnblockEvent",unblockEventFor:"actorUnblockEventFor",unblockEventForEntity:"actorUnblockEventForEntity",unfreeze:"actorUnfreeze"},
  Container: {blockEvent:"containerBlockEvent",clearClicks:"containerClearClicks",close:"containerClose",closeAll:"containerCloseAll",droppedClicks:"containerDroppedClicks",getIds:"containerGetIds",isOpen:"containerIsOpen",open:"containerOpen",pollClicks:"containerPollClicks",preview:"containerPreview",previewTitled:"containerPreviewTitled",setInteractive:"containerSetInteractive",setItem:"containerSetItem",setItemEnchanted:"containerSetItemEnchanted",setTitle:"containerSetTitle",update:"containerUpdate"},
  Entity: {clearMobProperties:"entityClearMobProperties",clearPlayerAppearance:"entityClearPlayerAppearance",clearPlayerRotation:"entityClearPlayerRotation",clearPlayerRotations:"entityClearPlayerRotations",clearRide:"entityClearRide",clearSpawns:"entityClearSpawns",clearVisiblePlayers:"entityClearVisiblePlayers",create:"entityCreate",createAdvanced:"entityCreateAdvanced",createRandom:"entityCreateRandom",createWithId:"entityCreateWithId",destroy:"entityDestroy",destroyAll:"entityDestroyAll",exists:"entityExists",findNearest:"entityFindNearest",getAllIds:"entityGetAllIds",getInfo:"entityGetInfo",isIdUsed:"entityIsIdUsed",playAnimation:"entityPlayAnimation",playAnimationTo:"entityPlayAnimationTo",pollSpawns:"entityPollSpawns",scaleBy:"entityScaleBy",setColorIndex:"entitySetColorIndex",setEnabled:"entitySetEnabled",setEquipmentSlot:"entitySetEquipmentSlot",setFlags:"entitySetFlags",setIdentifier:"entitySetIdentifier",setInvisible:"entitySetInvisible",setMarkVariant:"entitySetMarkVariant",setMobProperty:"entitySetMobProperty",setNametag:"entitySetNametag",setPlayerEquipmentSlot:"entitySetPlayerEquipmentSlot",setPlayerNametag:"entitySetPlayerNametag",setPlayerRotation:"entitySetPlayerRotation",setPlayerScale:"entitySetPlayerScale",setPose:"entitySetPose",setPosition:"entitySetPosition",setRideEntity:"entitySetRideEntity",setRidePlayer:"entitySetRidePlayer",setRotation:"entitySetRotation",setScale:"entitySetScale",setVariant:"entitySetVariant",setViewDistance:"entitySetViewDistance",setVisiblePlayer:"entitySetVisiblePlayer",setVisiblePlayers:"entitySetVisiblePlayers",stopAnimation:"entityStopAnimation",stopAnimationTo:"entityStopAnimationTo"},
  FakeInventory: {apply:"fakeInvApply",clear:"fakeInvClear",clearAll:"fakeInvClearAll",clearClicks:"fakeInvClearClicks",droppedClicks:"fakeInvDroppedClicks",getPlayers:"fakeInvGetPlayers",isActive:"fakeInvIsActive",pollClicks:"fakeInvPollClicks",refresh:"fakeInvRefresh",setSlot:"fakeInvSetSlot"},
  Form: {close:"formClose",closeAll:"formCloseAll"},
  Ghost: {clearInteractions:"ghostClearInteractions",pollInteractions:"ghostPollInteractions"},
  GradientLine: {create:"gradientCreate",destroy:"gradientDestroy",destroyAll:"gradientDestroyAll",draw:"gradientDraw",drawToDimension:"gradientDrawToDimension",remove:"gradientRemove",setColor:"gradientSetColor",setEndpoints:"gradientSetEndpoints",setGradient:"gradientSetGradient",setRainbow:"gradientSetRainbow",tick:"gradientTick"},
  Hologram: {addLine:"holoAddLine",apiVersion:"holoApiVersion",clearBackgroundColor:"holoClearBackgroundColor",clearFollowPlayer:"holoClearFollowPlayer",clearLines:"holoClearLines",clearRotation:"holoClearRotation",create:"holoCreate",destroy:"holoDestroy",destroyAll:"holoDestroyAll",draw:"holoDraw",drawToDimension:"holoDrawToDimension",drawToPlayer:"holoDrawToPlayer",getLineCount:"holoGetLineCount",info:"holoInfo",refresh:"holoRefresh",remove:"holoRemove",removeLine:"holoRemoveLine",setBackgroundColor:"holoSetBackgroundColor",setColor:"holoSetColor",setDepthTest:"holoSetDepthTest",setDimension:"holoSetDimension",setFollowPlayer:"holoSetFollowPlayer",setLineParseVariables:"holoSetLineParseVariables",setLinePool:"holoSetLinePool",setLineText:"holoSetLineText",setLocation:"holoSetLocation",setRotation:"holoSetRotation",setScale:"holoSetScale"},
  ItemActor: {destroy:"itemActorDestroy",destroyAll:"itemActorDestroyAll",exists:"itemActorExists",getIds:"itemActorGetIds",moveTo:"itemActorMoveTo",runtimeId:"itemActorRuntimeId",setMotion:"itemActorSetMotion",spawn:"itemActorSpawn",take:"itemActorTake"},
  ItemDetail: {hide:"itemDetailHide",show:"itemDetailShow"},
  ItemDisplay: {clearAnim:"itemDisplayClearAnim",clearVisiblePlayers:"itemDisplayClearVisiblePlayers",create:"itemDisplayCreate",createAdvanced:"itemDisplayCreateAdvanced",createFull:"itemDisplayCreateFull",createRandom:"itemDisplayCreateRandom",createSeamless:"itemDisplayCreateSeamless",createWithId:"itemDisplayCreateWithId",destroy:"itemDisplayDestroy",destroyAll:"itemDisplayDestroyAll",emitForEntity:"itemDisplayEmitForEntity",exists:"itemDisplayExists",findNearest:"itemDisplayFindNearest",follow:"itemDisplayFollow",getAllIds:"itemDisplayGetAllIds",getInfo:"itemDisplayGetInfo",isIdUsed:"itemDisplayIsIdUsed",rotateY:"itemDisplayRotateY",runtimeId:"itemDisplayRuntimeId",scaleBy:"itemDisplayScaleBy",scaleTo:"itemDisplayScaleTo",setAnim:"itemDisplaySetAnim",setBaseOffset:"itemDisplaySetBaseOffset",setEnabled:"itemDisplaySetEnabled",setExtend:"itemDisplaySetExtend",setGlint:"itemDisplaySetGlint",setHitbox:"itemDisplaySetHitbox",setItem:"itemDisplaySetItem",setItemWithNbt:"itemDisplaySetItemWithNbt",setMode:"itemDisplaySetMode",setOffset:"itemDisplaySetOffset",setOffsetLive:"itemDisplaySetOffsetLive",setPosition:"itemDisplaySetPosition",setPreset:"itemDisplaySetPreset",setRotation:"itemDisplaySetRotation",setRotationLive:"itemDisplaySetRotationLive",setScale:"itemDisplaySetScale",setScaleLive:"itemDisplaySetScaleLive",setStretch:"itemDisplaySetStretch",setStretchLive:"itemDisplaySetStretchLive",setTransformLive:"itemDisplaySetTransformLive",setViewDistance:"itemDisplaySetViewDistance",setVisiblePlayer:"itemDisplaySetVisiblePlayer",setVisiblePlayers:"itemDisplaySetVisiblePlayers",unfollow:"itemDisplayUnfollow"},
  Misc: {anchorAcquire:"anchorAcquire",anchorRelease:"anchorRelease",blockBatchClear:"blockBatchClear",blockBatchProgress:"blockBatchProgress",blockBatchSelfTest:"blockBatchSelfTest",blockBatchSet:"blockBatchSet",blockBatchSetAll:"blockBatchSetAll",blockBatchSetNear:"blockBatchSetNear",blockBatchStats:"blockBatchStats",blockFakeClear:"blockFakeClear",blockFakeClearAll:"blockFakeClearAll",blockFakeResolve:"blockFakeResolve",blockFakeSet:"blockFakeSet",blockFakeSetAll:"blockFakeSetAll",bossBarAdd:"bossBarAdd",bossBarAddOn:"bossBarAddOn",bossBarCount:"bossBarCount",bossBarGetInfo:"bossBarGetInfo",bossBarProbe:"bossBarProbe",bossBarRemove:"bossBarRemove",bossBarRemoveAll:"bossBarRemoveAll",bossBarSetHealth:"bossBarSetHealth",bossBarSetStyle:"bossBarSetStyle",bossBarSetTitle:"bossBarSetTitle",easterEggCreeper:"easterEggCreeper",easterEggCreeperDisplay:"easterEggCreeperDisplay",hudProbe:"hudProbe",hudResetAll:"hudResetAll",hudSetElement:"hudSetElement",hudSetElements:"hudSetElements",hudSetRaw:"hudSetRaw",playerPlayCelebrate:"playerPlayCelebrate",playerPlayOminousDrink:"playerPlayOminousDrink",playerShowBadOmen:"playerShowBadOmen",playerShowEffect:"playerShowEffect",playerShowVillageHero:"playerShowVillageHero",sidebarIsShown:"sidebarIsShown",sidebarLineCount:"sidebarLineCount",sidebarRemove:"sidebarRemove",sidebarRemoveAll:"sidebarRemoveAll",sidebarSet:"sidebarSet",sidebarSetLine:"sidebarSetLine",sidebarSetTitle:"sidebarSetTitle",wireResetStats:"wireResetStats",wireStats:"wireStats",wireTraceActive:"wireTraceActive",wireTraceClear:"wireTraceClear",wireTraceCount:"wireTraceCount",wireTraceDump:"wireTraceDump",wireTraceMark:"wireTraceMark",wireTraceStart:"wireTraceStart",wireTraceStop:"wireTraceStop"},
  NpcDialog: {clearClicks:"npcDialogClearClicks",close:"npcDialogClose",closeAll:"npcDialogCloseAll",droppedClicks:"npcDialogDroppedClicks",getIds:"npcDialogGetIds",isOpen:"npcDialogIsOpen",open:"npcDialogOpen",pollClicks:"npcDialogPollClicks",setAvatar:"npcDialogSetAvatar",update:"npcDialogUpdate"},
  Particle: {clearVisiblePlayers:"particleClearVisiblePlayers",createBox:"particleCreateBox",createBoxFaces:"particleCreateBoxFaces",createLine:"particleCreateLine",createPlane:"particleCreatePlane",createPoint:"particleCreatePoint",createPoly:"particleCreatePoly",createRect:"particleCreateRect",destroy:"particleDestroy",destroyAll:"particleDestroyAll",exists:"particleExists",follow:"particleFollow",getAllIds:"particleGetAllIds",getInfo:"particleGetInfo",moveBy:"particleMoveBy",moveTo:"particleMoveTo",setEffect:"particleSetEffect",setInterval:"particleSetInterval",setLifetime:"particleSetLifetime",setPos:"particleSetPos",setRot:"particleSetRot",setScale:"particleSetScale",setViewDistance:"particleSetViewDistance",setVisible:"particleSetVisible",setVisiblePlayers:"particleSetVisiblePlayers",spin:"particleSpin",unfollow:"particleUnfollow"},
  PlayerNpc: {captureSkin:"playerNpcCaptureSkin",clearPlayerRotation:"playerNpcClearPlayerRotation",clearPlayerRotations:"playerNpcClearPlayerRotations",clearRide:"playerNpcClearRide",clearSpawns:"playerNpcClearSpawns",clearVisiblePlayers:"playerNpcClearVisiblePlayers",create:"playerNpcCreate",createEx:"playerNpcCreateEx",createRandom:"playerNpcCreateRandom",createWithId:"playerNpcCreateWithId",destroy:"playerNpcDestroy",destroyAll:"playerNpcDestroyAll",exists:"playerNpcExists",getAllIds:"playerNpcGetAllIds",getDebugInfo:"playerNpcGetDebugInfo",getSkinBlobB64:"playerNpcGetSkinBlobB64",getSkinIds:"playerNpcGetSkinIds",hasSkin:"playerNpcHasSkin",importSkins:"playerNpcImportSkins",injectSkin:"playerNpcInjectSkin",injectSkinAll:"playerNpcInjectSkinAll",isIdUsed:"playerNpcIsIdUsed",playAnimation:"playerNpcPlayAnimation",playAnimationTo:"playerNpcPlayAnimationTo",playEmote:"playerNpcPlayEmote",pollSpawns:"playerNpcPollSpawns",registerImageSkin:"playerNpcRegisterImageSkin",registerImageSkinSized:"playerNpcRegisterImageSkinSized",registerSkin:"playerNpcRegisterSkin",registerSkinData:"playerNpcRegisterSkinData",registerSkinDataOpaque:"playerNpcRegisterSkinDataOpaque",registerSkinFromBlobB64:"playerNpcRegisterSkinFromBlobB64",registerSkinFromBlobFile:"playerNpcRegisterSkinFromBlobFile",runtimeId:"playerNpcRuntimeId",setEnabled:"playerNpcSetEnabled",setEquipment:"playerNpcSetEquipment",setImageSize:"playerNpcSetImageSize",setNametag:"playerNpcSetNametag",setPlayerRotation:"playerNpcSetPlayerRotation",setPos:"playerNpcSetPos",setPosLight:"playerNpcSetPosLight",setPose:"playerNpcSetPose",setRideEntity:"playerNpcSetRideEntity",setRidePlayer:"playerNpcSetRidePlayer",setRideUniqueId:"playerNpcSetRideUniqueId",setRotLight:"playerNpcSetRotLight",setRotation:"playerNpcSetRotation",setScale:"playerNpcSetScale",setSkin:"playerNpcSetSkin",setViewDistance:"playerNpcSetViewDistance",setVisiblePlayer:"playerNpcSetVisiblePlayer",setVisiblePlayers:"playerNpcSetVisiblePlayers",stopAnimation:"playerNpcStopAnimation",stopAnimationTo:"playerNpcStopAnimationTo",unregisterSkin:"playerNpcUnregisterSkin"},
  PlayerRespawn: {at:"playerRespawnAt",respawn:"playerRespawn",state:"playerRespawnState"},
  Sensing: {clearEvents:"swingClearEvents",count:"swingCount",deviceCode:"sensingDeviceCode",deviceOf:"sensingDeviceOf",dropped:"swingDropped",isTouch:"sensingIsTouch",lastBlock:"swingLastBlock",lastEvent:"swingLastEvent",lastTarget:"swingLastTarget",pollEvents:"swingPollEvents",reset:"swingReset",total:"swingTotal"},
  Shape: {attachEntity:"shapeAttachEntity",clearAttachedEntity:"shapeClearAttachedEntity",clearBackgroundColor:"shapeClearBackgroundColor",clearRotation:"shapeClearRotation",createArrow:"shapeCreateArrow",createBox:"shapeCreateBox",createCircle:"shapeCreateCircle",createCone:"shapeCreateCone",createCylinder:"shapeCreateCylinder",createEllipsoid:"shapeCreateEllipsoid",createLine:"shapeCreateLine",createPyramid:"shapeCreatePyramid",createSphere:"shapeCreateSphere",createText:"shapeCreateText",destroy:"shapeDestroy",destroyAll:"shapeDestroyAll",destroyBatch:"shapeDestroyBatch",draw:"shapeDraw",drawBatch:"shapeDrawBatch",drawToDimension:"shapeDrawToDimension",drawToPlayer:"shapeDrawToPlayer",exists:"shapeExists",findTextByLocation:"shapeFindTextByLocation",findTextByLocationAndContent:"shapeFindTextByLocationAndContent",getAllShapeIds:"shapeGetAllShapeIds",getColor:"shapeGetColor",getLocation:"shapeGetLocation",getRotation:"shapeGetRotation",getShapeType:"shapeGetShapeType",getText:"shapeGetText",remove:"shapeRemove",removeToDimension:"shapeRemoveToDimension",removeToPlayer:"shapeRemoveToPlayer",setBackgroundColor:"shapeSetBackgroundColor",setColor:"shapeSetColor",setDepthTest:"shapeSetDepthTest",setDimension:"shapeSetDimension",setDuration:"shapeSetDuration",setLocation:"shapeSetLocation",setMaxRenderDistance:"shapeSetMaxRenderDistance",setRotation:"shapeSetRotation",setScale:"shapeSetScale",setText:"shapeSetText",update:"shapeUpdate",updateToDimension:"shapeUpdateToDimension",updateToPlayer:"shapeUpdateToPlayer"},
  Sulfur: {create:"sulfurCreate",destroy:"sulfurDestroy",destroyAll:"sulfurDestroyAll",exists:"sulfurExists",getEntityId:"sulfurGetEntityId",getIds:"sulfurGetIds",setArchetype:"sulfurSetArchetype",setBlock:"sulfurSetBlock",setInvisible:"sulfurSetInvisible",setScale:"sulfurSetScale"},
  Trade: {addOffer:"tradeAddOffer",close:"tradeClose",closeAll:"tradeCloseAll",droppedSettlements:"tradeDroppedSettlements",getIds:"tradeGetIds",isOpen:"tradeIsOpen",isSettle:"tradeIsSettle",open:"tradeOpen",pollSettlements:"tradePollSettlements",setSettle:"tradeSetSettle",setTier:"tradeSetTier",update:"tradeUpdate"},
  View: {align:"viewAlign",block:"viewBlock",clearAll:"viewClearAll",clearBlock:"viewClearBlock",clearEntity:"viewClearEntity",clearId:"viewClearId",describe:"viewDescribe",echo:"viewEcho",entity:"viewEntity",entityInfo:"viewEntityInfo",itemSpawnHooked:"viewItemSpawnHooked",metaProbe:"viewMetaProbe",pollSpawnEvents:"viewPollSpawnEvents",self:"viewSelf",selfClear:"viewSelfClear",selfHide:"viewSelfHide",selfName:"viewSelfName",selfSkin:"viewSelfSkin",selfType:"viewSelfType",targetType:"viewTargetType",uniqueIdOf:"viewUniqueIdOf"},
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
