// HologramLib.d.ts —— 自动生成（tools/gen_lse_shim.py），API 1.32.0
// 放到脚本旁边, 编辑器就能补全 HL.xxx 的参数名与类型。

declare namespace HologramLib {
  interface Api {
    /** 库的 API 版本，如 "1.32.0" */
    readonly apiVersion: string;
    /** 虚表布局戳（脚本侧无需关心，C++ 消费方用） */
    readonly abiStamp: string;
    /** 本版导出的全部函数名 */
    list(): string[];
    /** 有没有这个导出 */
    has(name: string): boolean;
    /** 显式绑定（等价于取属性） */
    bind(name: string): (...args: any[]) => any;

    /** 实现见 src/container/ContainerMenuExporter.cpp */
    containerBlockEvent(menuId: number, b0: number, b1: number): boolean;
    /** 实现见 src/container/ContainerMenuExporter.cpp */
    containerClearClicks(): any;
    /** 实现见 src/container/ContainerMenuExporter.cpp */
    containerClose(id: number): boolean;
    /** 实现见 src/container/ContainerMenuExporter.cpp */
    containerCloseAll(): any;
    /** 实现见 src/container/ContainerMenuExporter.cpp */
    containerGetIds(): number;
    /** 实现见 src/container/ContainerMenuExporter.cpp */
    containerIsOpen(id: number): boolean;
    /** 实现见 src/container/ContainerMenuExporter.cpp */
    containerOpen(playerName: string, title: string, rows: number): number;
    /** 实现见 src/container/ContainerMenuExporter.cpp */
    containerPollClicks(): string;
    /** 实现见 src/container/ContainerMenuExporter.cpp */
    containerSetInteractive(id: number, on: boolean): boolean;
    /** 实现见 src/container/ContainerMenuExporter.cpp */
    containerSetItem(id: number, slot: number, type: string, count: number, damage: number, name: string, loreCsv: string): boolean;
    /** 实现见 src/container/ContainerMenuExporter.cpp */
    containerSetTitle(id: number, title: string): boolean;
    /** 实现见 src/customentity/CustomEntityExporter.cpp */
    entityClearMobProperties(id: number): boolean;
    /** 实现见 src/customentity/CustomEntityExporter.cpp */
    entityClearPlayerAppearance(id: number, playerName: string): boolean;
    /** 实现见 src/customentity/CustomEntityExporter.cpp */
    entityClearPlayerRotation(id: number, playerName: string): boolean;
    /** 实现见 src/customentity/CustomEntityExporter.cpp */
    entityClearPlayerRotations(id: number): boolean;
    /** 实现见 src/customentity/CustomEntityExporter.cpp */
    entityClearRide(id: number): boolean;
    /** 实现见 src/customentity/CustomEntityExporter.cpp */
    entityClearSpawns(): any;
    /** 实现见 src/customentity/CustomEntityExporter.cpp */
    entityClearVisiblePlayers(id: number): boolean;
    /** 实现见 src/customentity/CustomEntityExporter.cpp */
    entityCreate(identifier: string, x: number, y: number, z: number, dim: number): number;
    /** 实现见 src/customentity/CustomEntityExporter.cpp */
    entityCreateAdvanced(identifier: string, x: number, y: number, z: number, dim: number, yaw: number, pitch: number, scale: number, nametag: string): number;
    /** 实现见 src/customentity/CustomEntityExporter.cpp */
    entityCreateRandom(identifier: string, x: number, y: number, z: number, dim: number): number;
    /** 实现见 src/customentity/CustomEntityExporter.cpp */
    entityCreateWithId(identifier: string, x: number, y: number, z: number, dim: number, desiredId: number): number;
    /** 实现见 src/customentity/CustomEntityExporter.cpp */
    entityDestroy(id: number): boolean;
    /** 实现见 src/customentity/CustomEntityExporter.cpp */
    entityDestroyAll(): any;
    /** 实现见 src/customentity/CustomEntityExporter.cpp */
    entityExists(id: number): boolean;
    /** 实现见 src/customentity/CustomEntityExporter.cpp */
    entityFindNearest(x: number, y: number, z: number, dim: number, maxDist: number): number;
    /** 实现见 src/customentity/CustomEntityExporter.cpp */
    entityGetAllIds(): number;
    /** 实现见 src/customentity/CustomEntityExporter.cpp */
    entityGetInfo(id: number): string;
    /** 实现见 src/customentity/CustomEntityExporter.cpp */
    entityIsIdUsed(id: number): boolean;
    /** 实现见 src/customentity/CustomEntityExporter.cpp */
    entityPlayAnimation(id: number, animation: string, stopExpression: string, durationTicks: number): boolean;
    /** 实现见 src/customentity/CustomEntityExporter.cpp */
    entityPlayAnimationTo(id: number, playerName: string, animation: string, stopExpression: string, durationTicks: number): boolean;
    /** 实现见 src/customentity/CustomEntityExporter.cpp */
    entityPollSpawns(): string;
    /** 实现见 src/customentity/CustomEntityExporter.cpp */
    entityScaleBy(id: number, factor: number): boolean;
    /** 实现见 src/customentity/CustomEntityExporter.cpp */
    entitySetColorIndex(id: number, colorIndex: number): boolean;
    /** 实现见 src/customentity/CustomEntityExporter.cpp */
    entitySetEnabled(id: number, enabled: boolean): boolean;
    /** 实现见 src/customentity/CustomEntityExporter.cpp */
    entitySetEquipmentSlot(id: number, slot: number, name: string, aux: number, nbt: string): boolean;
    /** 实现见 src/customentity/CustomEntityExporter.cpp */
    entitySetFlags(id: number, flags: number): boolean;
    /** 实现见 src/customentity/CustomEntityExporter.cpp */
    entitySetIdentifier(id: number, identifier: string): boolean;
    /** 实现见 src/customentity/CustomEntityExporter.cpp */
    entitySetInvisible(id: number, on: boolean): boolean;
    /** 实现见 src/customentity/CustomEntityExporter.cpp */
    entitySetMarkVariant(id: number, markVariant: number): boolean;
    /** 实现见 src/customentity/CustomEntityExporter.cpp */
    entitySetMobProperty(id: number, name: string, value: string): boolean;
    /** 实现见 src/customentity/CustomEntityExporter.cpp */
    entitySetNametag(id: number, text: string): boolean;
    /** 实现见 src/customentity/CustomEntityExporter.cpp */
    entitySetPlayerEquipmentSlot(id: number, playerName: string, slot: number, name: string, aux: number, nbt: string): boolean;
    /** 实现见 src/customentity/CustomEntityExporter.cpp */
    entitySetPlayerNametag(id: number, playerName: string, text: string): boolean;
    /** 实现见 src/customentity/CustomEntityExporter.cpp */
    entitySetPlayerRotation(id: number, playerName: string, yaw: number, pitch: number): boolean;
    /** 实现见 src/customentity/CustomEntityExporter.cpp */
    entitySetPlayerScale(id: number, playerName: string, scale: number): boolean;
    /** 实现见 src/customentity/CustomEntityExporter.cpp */
    entitySetPose(id: number, pose: number): boolean;
    /** 实现见 src/customentity/CustomEntityExporter.cpp */
    entitySetPosition(id: number, x: number, y: number, z: number, dim: number): boolean;
    /** 实现见 src/customentity/CustomEntityExporter.cpp */
    entitySetRideEntity(id: number, vehicleEntityId: number): boolean;
    /** 实现见 src/customentity/CustomEntityExporter.cpp */
    entitySetRidePlayer(id: number, playerName: string): boolean;
    /** 实现见 src/customentity/CustomEntityExporter.cpp */
    entitySetRotation(id: number, yaw: number, pitch: number): boolean;
    /** 实现见 src/customentity/CustomEntityExporter.cpp */
    entitySetScale(id: number, scale: number): boolean;
    /** 实现见 src/customentity/CustomEntityExporter.cpp */
    entitySetVariant(id: number, variant: number): boolean;
    /** 实现见 src/customentity/CustomEntityExporter.cpp */
    entitySetViewDistance(id: number, dist: number): boolean;
    /** 实现见 src/customentity/CustomEntityExporter.cpp */
    entitySetVisiblePlayer(id: number, playerName: string): boolean;
    /** 实现见 src/customentity/CustomEntityExporter.cpp */
    entitySetVisiblePlayers(id: number, playerNames: string): boolean;
    /** 实现见 src/customentity/CustomEntityExporter.cpp */
    entityStopAnimation(id: number): boolean;
    /** 实现见 src/customentity/CustomEntityExporter.cpp */
    entityStopAnimationTo(id: number, playerName: string): boolean;
    /** 实现见 src/fakeinv/FakeInventoryExporter.cpp */
    fakeInvApply(playerName: string, itemsSpec: string, refreshIntervalTicks: number): boolean;
    /** 实现见 src/fakeinv/FakeInventoryExporter.cpp */
    fakeInvClear(playerName: string): boolean;
    /** 实现见 src/fakeinv/FakeInventoryExporter.cpp */
    fakeInvClearAll(): any;
    /** 实现见 src/fakeinv/FakeInventoryExporter.cpp */
    fakeInvClearClicks(): any;
    /** 实现见 src/fakeinv/FakeInventoryExporter.cpp */
    fakeInvGetPlayers(): string;
    /** 实现见 src/fakeinv/FakeInventoryExporter.cpp */
    fakeInvIsActive(playerName: string): boolean;
    /** 实现见 src/fakeinv/FakeInventoryExporter.cpp */
    fakeInvPollClicks(): string;
    /** 实现见 src/fakeinv/FakeInventoryExporter.cpp */
    fakeInvRefresh(playerName: string): boolean;
    /** 实现见 src/fakeinv/FakeInventoryExporter.cpp */
    fakeInvSetSlot(playerName: string, slot: number, type: string, count: number, damage: number, name: string, loreCsv: string): boolean;
    /** 实现见 src/ui/ClientUiExporter.cpp */
    formClose(playerName: string): boolean;
    /** 实现见 src/ui/ClientUiExporter.cpp */
    formCloseAll(): number;
    /** 实现见 src/ghost/GhostInteractRouter.cpp */
    ghostClearInteractions(): any;
    /** 实现见 src/ghost/GhostInteractRouter.cpp */
    ghostPollInteractions(): string;
    /** 实现见 src/GradientLineExporter.cpp */
    gradientCreate(x1: number, y1: number, z1: number, x2: number, y2: number, z2: number, segments: number): number;
    /** 实现见 src/GradientLineExporter.cpp */
    gradientDestroy(id: number): boolean;
    /** 实现见 src/GradientLineExporter.cpp */
    gradientDestroyAll(): any;
    /** 实现见 src/GradientLineExporter.cpp */
    gradientDraw(id: number): boolean;
    /** 实现见 src/GradientLineExporter.cpp */
    gradientDrawToDimension(id: number, dimId: number): boolean;
    /** 实现见 src/GradientLineExporter.cpp */
    gradientRemove(id: number): boolean;
    /** 实现见 src/GradientLineExporter.cpp */
    gradientSetColor(id: number, r: number, g: number, b: number, a: number): boolean;
    /** 实现见 src/GradientLineExporter.cpp */
    gradientSetEndpoints(id: number, x1: number, y1: number, z1: number, x2: number, y2: number, z2: number): boolean;
    /** 实现见 src/GradientLineExporter.cpp */
    gradientSetGradient(id: number, r1: number, g1: number, b1: number, r2: number, g2: number, b2: number): boolean;
    /** 实现见 src/GradientLineExporter.cpp */
    gradientSetRainbow(id: number, speed: number): boolean;
    /** 实现见 src/GradientLineExporter.cpp */
    gradientTick(deltaTime: number): any;
    /** 实现见 src/FloatingTextExporter.cpp */
    holoAddLine(id: number, text: string): boolean;
    /** 实现见 src/InfoExporter.cpp */
    holoApiVersion(): number;
    /** 实现见 src/FloatingTextExporter.cpp */
    holoClearBackgroundColor(id: number): boolean;
    /** 实现见 src/FloatingTextExporter.cpp */
    holoClearFollowPlayer(id: number): boolean;
    /** 实现见 src/FloatingTextExporter.cpp */
    holoClearLines(id: number): boolean;
    /** 实现见 src/FloatingTextExporter.cpp */
    holoClearRotation(id: number): boolean;
    /** 实现见 src/FloatingTextExporter.cpp */
    holoCreate(x: number, y: number, z: number): number;
    /** 实现见 src/FloatingTextExporter.cpp */
    holoDestroy(id: number): boolean;
    /** 实现见 src/FloatingTextExporter.cpp */
    holoDestroyAll(): any;
    /** 实现见 src/FloatingTextExporter.cpp */
    holoDraw(id: number): boolean;
    /** 实现见 src/FloatingTextExporter.cpp */
    holoDrawToDimension(id: number, dimId: number): boolean;
    /** 实现见 src/FloatingTextExporter.cpp */
    holoDrawToPlayer(id: number, playerName: string): boolean;
    /** 实现见 src/FloatingTextExporter.cpp */
    holoGetLineCount(id: number): number;
    /** 实现见 src/InfoExporter.cpp */
    holoInfo(): string;
    /** 实现见 src/FloatingTextExporter.cpp */
    holoRefresh(id: number): boolean;
    /** 实现见 src/FloatingTextExporter.cpp */
    holoRemove(id: number): boolean;
    /** 实现见 src/FloatingTextExporter.cpp */
    holoRemoveLine(id: number, lineIndex: number): boolean;
    /** 实现见 src/FloatingTextExporter.cpp */
    holoSetBackgroundColor(id: number, r: number, g: number, b: number, a: number): boolean;
    /** 实现见 src/FloatingTextExporter.cpp */
    holoSetColor(id: number, r: number, g: number, b: number, a: number): boolean;
    /** 实现见 src/FloatingTextExporter.cpp */
    holoSetDepthTest(id: number, enabled: boolean): boolean;
    /** 实现见 src/FloatingTextExporter.cpp */
    holoSetDimension(id: number, dimId: number): boolean;
    /** 实现见 src/FloatingTextExporter.cpp */
    holoSetFollowPlayer(id: number, playerName: string, offsetY: number): boolean;
    /** 实现见 src/FloatingTextExporter.cpp */
    holoSetLineParseVariables(id: number, lineIndex: number, enabled: boolean): boolean;
    /** 实现见 src/FloatingTextExporter.cpp */
    holoSetLinePool(id: number, lineIndex: number, content: string, intervalMs: number): boolean;
    /** 实现见 src/FloatingTextExporter.cpp */
    holoSetLineText(id: number, lineIndex: number, text: string): boolean;
    /** 实现见 src/FloatingTextExporter.cpp */
    holoSetLocation(id: number, x: number, y: number, z: number): boolean;
    /** 实现见 src/FloatingTextExporter.cpp */
    holoSetRotation(id: number, pitch: number, yaw: number, roll: number): boolean;
    /** 实现见 src/FloatingTextExporter.cpp */
    holoSetScale(id: number, scale: number): boolean;
    /** 实现见 src/itemactor/ItemActorExporter.cpp */
    itemActorDestroy(id: number): boolean;
    /** 实现见 src/itemactor/ItemActorExporter.cpp */
    itemActorDestroyAll(): any;
    /** 实现见 src/itemactor/ItemActorExporter.cpp */
    itemActorExists(id: number): boolean;
    /** 实现见 src/itemactor/ItemActorExporter.cpp */
    itemActorGetIds(): number;
    /** 实现见 src/itemactor/ItemActorExporter.cpp */
    itemActorMoveTo(id: number, x: number, y: number, z: number): boolean;
    /** 实现见 src/itemactor/ItemActorExporter.cpp */
    itemActorSetMotion(id: number, vx: number, vy: number, vz: number): boolean;
    /** 实现见 src/itemactor/ItemActorExporter.cpp */
    itemActorSpawn(itemId: string, aux: number, count: number, x: number, y: number, z: number, dim: number, vx: number, vy: number, vz: number): number;
    /** 实现见 src/itemactor/ItemActorExporter.cpp */
    itemActorTake(id: number, playerName: string): boolean;
    /** 实现见 src/itemdetail/ItemDetailExporter.cpp */
    itemDetailHide(id: number): boolean;
    /** 实现见 src/itemdetail/ItemDetailExporter.cpp */
    itemDetailShow(dimId: number, x: number, y: number, z: number, itemId: string, aux: number, count: number, customText: string): number;
    /** 实现见 src/itemdisplay/ItemDisplayExporter.cpp */
    itemDisplayClearAnim(id: number): boolean;
    /** 实现见 src/itemdisplay/ItemDisplayExporter.cpp */
    itemDisplayClearVisiblePlayers(id: number): boolean;
    /** 实现见 src/itemdisplay/ItemDisplayExporter.cpp */
    itemDisplayCreate(x: number, y: number, z: number, dim: number, item: string, aux: number): number;
    /** 实现见 src/itemdisplay/ItemDisplayExporter.cpp */
    itemDisplayCreateAdvanced(x: number, y: number, z: number, dim: number, item: string, aux: number, offX: string, offY: string, offZ: string, rotX: string, rotY: string, rotZ: string, scale: string): number;
    /** 实现见 src/itemdisplay/ItemDisplayExporter.cpp */
    itemDisplayCreateFull(x: number, y: number, z: number, dim: number, item: string, aux: number, offX: string, offY: string, offZ: string, rotX: string, rotY: string, rotZ: string, scale: string, nbt: string, glint: boolean): number;
    /** 实现见 src/itemdisplay/ItemDisplayExporter.cpp */
    itemDisplayCreateRandom(x: number, y: number, z: number, dim: number, item: string, aux: number): number;
    /** 实现见 src/itemdisplay/ItemDisplayExporter.cpp */
    itemDisplayCreateSeamless(x: number, y: number, z: number, dim: number, item: string, aux: number, offX: string, offY: string, offZ: string, rotX: string, rotY: string, rotZ: string, scale: string, mode: number, viewDistance: number, visiblePlayer: string): number;
    /** 实现见 src/itemdisplay/ItemDisplayExporter.cpp */
    itemDisplayCreateWithId(x: number, y: number, z: number, dim: number, item: string, aux: number, desiredId: number): number;
    /** 实现见 src/itemdisplay/ItemDisplayExporter.cpp */
    itemDisplayDestroy(id: number): boolean;
    /** 实现见 src/itemdisplay/ItemDisplayExporter.cpp */
    itemDisplayDestroyAll(): any;
    /** 实现见 src/itemdisplay/ItemDisplayExporter.cpp */
    itemDisplayEmitForEntity(playerName: string, uniqueId: number, item: string, aux: number, offsetY: string, rotX: string, rotY: string, rotZ: string, scale: string, nbt: string, glint: boolean): number;
    /** 实现见 src/itemdisplay/ItemDisplayExporter.cpp */
    itemDisplayExists(id: number): boolean;
    /** 实现见 src/itemdisplay/ItemDisplayExporter.cpp */
    itemDisplayFindNearest(x: number, y: number, z: number, dim: number, maxDist: number): number;
    /** 实现见 src/itemdisplay/ItemDisplayExporter.cpp */
    itemDisplayFollow(id: number, playerName: string, offX: number, offY: number, offZ: number): boolean;
    /** 实现见 src/itemdisplay/ItemDisplayExporter.cpp */
    itemDisplayGetAllIds(): number;
    /** 实现见 src/itemdisplay/ItemDisplayExporter.cpp */
    itemDisplayGetInfo(id: number): string;
    /** 实现见 src/itemdisplay/ItemDisplayExporter.cpp */
    itemDisplayIsIdUsed(id: number): boolean;
    /** 实现见 src/itemdisplay/ItemDisplayExporter.cpp */
    itemDisplayRotateY(id: number, delta: number): boolean;
    /** 实现见 src/itemdisplay/ItemDisplayExporter.cpp */
    itemDisplayScaleBy(id: number, factor: number): boolean;
    /** 实现见 src/itemdisplay/ItemDisplayExporter.cpp */
    itemDisplayScaleTo(id: number, targetScale: number): boolean;
    /** 实现见 src/itemdisplay/ItemDisplayExporter.cpp */
    itemDisplaySetAnim(id: number, spinSpeed: number, spinAxis: number, bobAmp: number, bobPeriod: number, pulseAmp: number, pulsePeriod: number, swayAmp: number, swayPeriod: number, phase: number, billboard: boolean): boolean;
    /** 实现见 src/itemdisplay/ItemDisplayExporter.cpp */
    itemDisplaySetBaseOffset(id: number, ox: string, oy: string, oz: string): boolean;
    /** 实现见 src/itemdisplay/ItemDisplayExporter.cpp */
    itemDisplaySetEnabled(id: number, enabled: boolean): boolean;
    /** 实现见 src/itemdisplay/ItemDisplayExporter.cpp */
    itemDisplaySetExtend(id: number, scale: string, rx: string, ry: string): boolean;
    /** 实现见 src/itemdisplay/ItemDisplayExporter.cpp */
    itemDisplaySetGlint(id: number, on: boolean): boolean;
    /** 实现见 src/itemdisplay/ItemDisplayExporter.cpp */
    itemDisplaySetHitbox(id: number, width: number, height: number): boolean;
    /** 实现见 src/itemdisplay/ItemDisplayExporter.cpp */
    itemDisplaySetItem(id: number, item: string, aux: number): boolean;
    /** 实现见 src/itemdisplay/ItemDisplayExporter.cpp */
    itemDisplaySetItemWithNbt(id: number, item: string, aux: number, nbt: string): boolean;
    /** 实现见 src/itemdisplay/ItemDisplayExporter.cpp */
    itemDisplaySetMode(id: number, mode: number): boolean;
    /** 实现见 src/itemdisplay/ItemDisplayExporter.cpp */
    itemDisplaySetOffset(id: number, ox: string, oy: string, oz: string): boolean;
    /** 实现见 src/itemdisplay/ItemDisplayExporter.cpp */
    itemDisplaySetOffsetLive(id: number, ox: string, oy: string, oz: string): boolean;
    /** 实现见 src/itemdisplay/ItemDisplayExporter.cpp */
    itemDisplaySetPosition(id: number, x: number, y: number, z: number, dim: number): boolean;
    /** 实现见 src/itemdisplay/ItemDisplayExporter.cpp */
    itemDisplaySetPreset(id: number, preset: string, a: number, b: number, c: number): boolean;
    /** 实现见 src/itemdisplay/ItemDisplayExporter.cpp */
    itemDisplaySetRotation(id: number, rx: string, ry: string, rz: string): boolean;
    /** 实现见 src/itemdisplay/ItemDisplayExporter.cpp */
    itemDisplaySetRotationLive(id: number, rx: string, ry: string, rz: string): boolean;
    /** 实现见 src/itemdisplay/ItemDisplayExporter.cpp */
    itemDisplaySetScale(id: number, scale: string): boolean;
    /** 实现见 src/itemdisplay/ItemDisplayExporter.cpp */
    itemDisplaySetScaleLive(id: number, scale: string): boolean;
    /** 实现见 src/itemdisplay/ItemDisplayExporter.cpp */
    itemDisplaySetStretch(id: number, xz: string, y: string): boolean;
    /** 实现见 src/itemdisplay/ItemDisplayExporter.cpp */
    itemDisplaySetStretchLive(id: number, xz: string, y: string): boolean;
    /** 实现见 src/itemdisplay/ItemDisplayExporter.cpp */
    itemDisplaySetTransformLive(id: number, ox: string, oy: string, oz: string, rx: string, ry: string, rz: string, scale: string): boolean;
    /** 实现见 src/itemdisplay/ItemDisplayExporter.cpp */
    itemDisplaySetViewDistance(id: number, dist: number): boolean;
    /** 实现见 src/itemdisplay/ItemDisplayExporter.cpp */
    itemDisplaySetVisiblePlayer(id: number, playerName: string): boolean;
    /** 实现见 src/itemdisplay/ItemDisplayExporter.cpp */
    itemDisplaySetVisiblePlayers(id: number, playerNames: string): boolean;
    /** 实现见 src/itemdisplay/ItemDisplayExporter.cpp */
    itemDisplayUnfollow(id: number): boolean;
    /** 实现见 src/ui/ClientUiExporter.cpp */
    locatorBarClear(playerName: string): number;
    /** 实现见 src/ui/ClientUiExporter.cpp */
    locatorBarCount(playerName: string): number;
    /** 实现见 src/ui/ClientUiExporter.cpp */
    locatorBarHideFromOthers(playerName: string, hidden: boolean): boolean;
    /** 实现见 src/ui/ClientUiExporter.cpp */
    locatorBarIsHiddenFromOthers(playerName: string): boolean;
    /** 实现见 src/ui/ClientUiExporter.cpp */
    locatorBarRemove(playerName: string, key: string): boolean;
    /** 实现见 src/ui/ClientUiExporter.cpp */
    locatorBarSet(playerName: string, key: string, x: number, y: number, z: number, dim: number, color: number, visible: boolean): boolean;
    /** 实现见 src/npcdialog/NpcDialogueExporter.cpp */
    npcDialogClearClicks(): any;
    /** 实现见 src/npcdialog/NpcDialogueExporter.cpp */
    npcDialogClose(id: number): boolean;
    /** 实现见 src/npcdialog/NpcDialogueExporter.cpp */
    npcDialogCloseAll(): any;
    /** 实现见 src/npcdialog/NpcDialogueExporter.cpp */
    npcDialogGetIds(): number;
    /** 实现见 src/npcdialog/NpcDialogueExporter.cpp */
    npcDialogIsOpen(id: number): boolean;
    /** 实现见 src/npcdialog/NpcDialogueExporter.cpp */
    npcDialogOpen(playerName: string, npcName: string, sceneName: string, dialogue: string, buttonsSpec: string, carrierIdentifier: string): number;
    /** 实现见 src/npcdialog/NpcDialogueExporter.cpp */
    npcDialogPollClicks(): string;
    /** 实现见 src/npcdialog/NpcDialogueExporter.cpp */
    npcDialogSetAvatar(id: number, skinVariant: number, avatarViewSpec: string): boolean;
    /** 实现见 src/npcdialog/NpcDialogueExporter.cpp */
    npcDialogUpdate(id: number, dialogue: string, buttonsSpec: string): boolean;
    /** 实现见 src/particles/ParticleShapeExporter.cpp */
    particleCreateBox(playerName: string, dimId: number, cx: number, cy: number, cz: number, hx: number, hy: number, hz: number, step: number, effect: string, interval: number, lifetime: number): number;
    /** 实现见 src/particles/ParticleShapeExporter.cpp */
    particleCreateBoxFaces(playerName: string, dimId: number, cx: number, cy: number, cz: number, hx: number, hy: number, hz: number, step: number, effect: string, interval: number, lifetime: number): number;
    /** 实现见 src/particles/ParticleShapeExporter.cpp */
    particleCreateLine(playerName: string, dimId: number, x1: number, y1: number, z1: number, x2: number, y2: number, z2: number, step: number, effect: string, interval: number, lifetime: number): number;
    /** 实现见 src/particles/ParticleShapeExporter.cpp */
    particleCreatePlane(playerName: string, dimId: number, cx: number, cy: number, cz: number, w: number, h: number, axis: number, step: number, effect: string, interval: number, lifetime: number): number;
    /** 实现见 src/particles/ParticleShapeExporter.cpp */
    particleCreatePoint(playerName: string, dimId: number, x: number, y: number, z: number, effect: string, interval: number, lifetime: number): number;
    /** 实现见 src/particles/ParticleShapeExporter.cpp */
    particleCreatePoly(playerName: string, dimId: number, verts: string, edges: string, step: number, effect: string, interval: number, lifetime: number): number;
    /** 实现见 src/particles/ParticleShapeExporter.cpp */
    particleCreateRect(playerName: string, dimId: number, cx: number, cy: number, cz: number, w: number, h: number, axis: number, step: number, effect: string, interval: number, lifetime: number): number;
    /** 实现见 src/particles/ParticleShapeExporter.cpp */
    particleDestroy(id: number): boolean;
    /** 实现见 src/particles/ParticleShapeExporter.cpp */
    particleFollow(id: number, uuid: string, offX: number, offY: number, offZ: number): boolean;
    /** 实现见 src/particles/ParticleShapeExporter.cpp */
    particleGetInfo(id: number): string;
    /** 实现见 src/particles/ParticleShapeExporter.cpp */
    particleMoveBy(id: number, dx: number, dy: number, dz: number): boolean;
    /** 实现见 src/particles/ParticleShapeExporter.cpp */
    particleMoveTo(id: number, x: number, y: number, z: number, durationTicks: number): boolean;
    /** 实现见 src/particles/ParticleShapeExporter.cpp */
    particleSetEffect(id: number, effect: string): boolean;
    /** 实现见 src/particles/ParticleShapeExporter.cpp */
    particleSetInterval(id: number, ticks: number): boolean;
    /** 实现见 src/particles/ParticleShapeExporter.cpp */
    particleSetLifetime(id: number, ticks: number): boolean;
    /** 实现见 src/particles/ParticleShapeExporter.cpp */
    particleSetPos(id: number, x: number, y: number, z: number): boolean;
    /** 实现见 src/particles/ParticleShapeExporter.cpp */
    particleSetRot(id: number, rx: number, ry: number, rz: number): boolean;
    /** 实现见 src/particles/ParticleShapeExporter.cpp */
    particleSetScale(id: number, s: number): boolean;
    /** 实现见 src/particles/ParticleShapeExporter.cpp */
    particleSetViewDistance(id: number, blocks: number): boolean;
    /** 实现见 src/particles/ParticleShapeExporter.cpp */
    particleSetVisible(id: number, playersCsv: string): boolean;
    /** 实现见 src/particles/ParticleShapeExporter.cpp */
    particleSpin(id: number, sx: number, sy: number, sz: number): boolean;
    /** 实现见 src/particles/ParticleShapeExporter.cpp */
    particleUnfollow(id: number): boolean;
    /** 实现见 src/ui/ClientUiExporter.cpp */
    playerHideGetMask(playerName: string): string;
    /** 实现见 src/ui/ClientUiExporter.cpp */
    playerHideIsHidden(playerName: string): boolean;
    /** 实现见 src/ui/ClientUiExporter.cpp */
    playerHideRestore(playerName: string): boolean;
    /** 实现见 src/ui/ClientUiExporter.cpp */
    playerHideSetHidden(playerName: string, hidden: boolean): boolean;
    /** 实现见 src/ui/ClientUiExporter.cpp */
    playerHideSetMask(playerName: string, spawn: boolean, tab: boolean, motion: boolean, state: boolean, skin: boolean, alsoSelf: boolean): boolean;
    /** 实现见 src/playernpc/PlayerNpcExporter.cpp */
    playerNpcCaptureSkin(skinId: string, playerName: string): boolean;
    /** 实现见 src/playernpc/PlayerNpcExporter.cpp */
    playerNpcClearPlayerRotation(id: number, playerName: string): boolean;
    /** 实现见 src/playernpc/PlayerNpcExporter.cpp */
    playerNpcClearPlayerRotations(id: number): boolean;
    /** 实现见 src/playernpc/PlayerNpcExporter.cpp */
    playerNpcClearRide(id: number): boolean;
    /** 实现见 src/playernpc/PlayerNpcExporter.cpp */
    playerNpcClearSpawns(): any;
    /** 实现见 src/playernpc/PlayerNpcExporter.cpp */
    playerNpcClearVisiblePlayers(id: number): boolean;
    /** 实现见 src/playernpc/PlayerNpcExporter.cpp */
    playerNpcCreate(x: number, y: number, z: number, dim: number, name: string, skinId: string): number;
    /** 实现见 src/playernpc/PlayerNpcExporter.cpp */
    playerNpcCreateEx(x: number, y: number, z: number, dim: number, name: string, skinId: string, yaw: number, scale: number): number;
    /** 实现见 src/playernpc/PlayerNpcExporter.cpp */
    playerNpcCreateRandom(x: number, y: number, z: number, dim: number, name: string, skinId: string): number;
    /** 实现见 src/playernpc/PlayerNpcExporter.cpp */
    playerNpcCreateWithId(x: number, y: number, z: number, dim: number, name: string, skinId: string, desiredId: number): number;
    /** 实现见 src/playernpc/PlayerNpcExporter.cpp */
    playerNpcDestroy(id: number): boolean;
    /** 实现见 src/playernpc/PlayerNpcExporter.cpp */
    playerNpcDestroyAll(): any;
    /** 实现见 src/playernpc/PlayerNpcExporter.cpp */
    playerNpcExists(id: number): boolean;
    /** 实现见 src/playernpc/PlayerNpcExporter.cpp */
    playerNpcGetAllIds(): number;
    /** 实现见 src/playernpc/PlayerNpcExporter.cpp */
    playerNpcGetDebugInfo(id: number): string;
    /** 实现见 src/playernpc/PlayerNpcExporter.cpp */
    playerNpcGetSkinBlobB64(skinId: string): string;
    /** 实现见 src/playernpc/PlayerNpcExporter.cpp */
    playerNpcGetSkinIds(): string;
    /** 实现见 src/playernpc/PlayerNpcExporter.cpp */
    playerNpcHasSkin(skinId: string): boolean;
    /** 实现见 src/playernpc/PlayerNpcExporter.cpp */
    playerNpcImportSkins(dirPath: string): number;
    /** 实现见 src/playernpc/PlayerNpcExporter.cpp */
    playerNpcInjectSkin(viewerName: string, targetName: string, skinId: string): boolean;
    /** 实现见 src/playernpc/PlayerNpcExporter.cpp */
    playerNpcInjectSkinAll(targetName: string, skinId: string): boolean;
    /** 实现见 src/playernpc/PlayerNpcExporter.cpp */
    playerNpcIsIdUsed(id: number): boolean;
    /** 实现见 src/playernpc/PlayerNpcExporter.cpp */
    playerNpcPlayAnimation(id: number, animation: string, stopExpression: string, durationTicks: number): boolean;
    /** 实现见 src/playernpc/PlayerNpcExporter.cpp */
    playerNpcPlayAnimationTo(id: number, playerName: string, animation: string, stopExpression: string, durationTicks: number): boolean;
    /** 实现见 src/playernpc/PlayerNpcExporter.cpp */
    playerNpcPlayEmote(id: number, pieceId: string, lengthTicks: number): boolean;
    /** 实现见 src/playernpc/PlayerNpcExporter.cpp */
    playerNpcPollSpawns(): string;
    /** 实现见 src/playernpc/PlayerNpcExporter.cpp */
    playerNpcRegisterImageSkin(pngPath: string, skinId: string, sizeBlocks: number, transparent: boolean): boolean;
    /** 实现见 src/playernpc/PlayerNpcExporter.cpp */
    playerNpcRegisterSkin(pngPath: string, skinId: string, geometry: string, armSize: string): boolean;
    /** 实现见 src/playernpc/PlayerNpcExporter.cpp */
    playerNpcRegisterSkinData(pngPath: string, skinId: string, armSize: string, geometryData: string): boolean;
    /** 实现见 src/playernpc/PlayerNpcExporter.cpp */
    playerNpcRegisterSkinDataOpaque(pngPath: string, skinId: string, armSize: string, geometryData: string): boolean;
    /** 实现见 src/playernpc/PlayerNpcExporter.cpp */
    playerNpcRegisterSkinFromBlobB64(b64: string): boolean;
    /** 实现见 src/playernpc/PlayerNpcExporter.cpp */
    playerNpcRegisterSkinFromBlobFile(path: string): boolean;
    /** 实现见 src/playernpc/PlayerNpcExporter.cpp */
    playerNpcSetEnabled(id: number, enabled: boolean): boolean;
    /** 实现见 src/playernpc/PlayerNpcExporter.cpp */
    playerNpcSetEquipment(id: number, slot: number, name: string, aux: number, nbt: string): boolean;
    /** 实现见 src/playernpc/PlayerNpcExporter.cpp */
    playerNpcSetNametag(id: number, text: string): boolean;
    /** 实现见 src/playernpc/PlayerNpcExporter.cpp */
    playerNpcSetPlayerRotation(id: number, playerName: string, yaw: number): boolean;
    /** 实现见 src/playernpc/PlayerNpcExporter.cpp */
    playerNpcSetPos(id: number, x: number, y: number, z: number, dim: number): boolean;
    /** 实现见 src/playernpc/PlayerNpcExporter.cpp */
    playerNpcSetPosLight(id: number, x: number, y: number, z: number, dim: number): boolean;
    /** 实现见 src/playernpc/PlayerNpcExporter.cpp */
    playerNpcSetPose(id: number, pose: number): boolean;
    /** 实现见 src/playernpc/PlayerNpcExporter.cpp */
    playerNpcSetRideEntity(id: number, vehicleEntityId: number): boolean;
    /** 实现见 src/playernpc/PlayerNpcExporter.cpp */
    playerNpcSetRidePlayer(id: number, playerName: string): boolean;
    /** 实现见 src/playernpc/PlayerNpcExporter.cpp */
    playerNpcSetRideUniqueId(id: number, vehicleUniqueId: number): boolean;
    /** 实现见 src/playernpc/PlayerNpcExporter.cpp */
    playerNpcSetRotLight(id: number, yaw: number): boolean;
    /** 实现见 src/playernpc/PlayerNpcExporter.cpp */
    playerNpcSetRotation(id: number, yaw: number): boolean;
    /** 实现见 src/playernpc/PlayerNpcExporter.cpp */
    playerNpcSetScale(id: number, scale: number): boolean;
    /** 实现见 src/playernpc/PlayerNpcExporter.cpp */
    playerNpcSetSkin(id: number, skinId: string): boolean;
    /** 实现见 src/playernpc/PlayerNpcExporter.cpp */
    playerNpcSetViewDistance(id: number, dist: number): boolean;
    /** 实现见 src/playernpc/PlayerNpcExporter.cpp */
    playerNpcSetVisiblePlayer(id: number, playerName: string): boolean;
    /** 实现见 src/playernpc/PlayerNpcExporter.cpp */
    playerNpcSetVisiblePlayers(id: number, playerNames: string): boolean;
    /** 实现见 src/playernpc/PlayerNpcExporter.cpp */
    playerNpcStopAnimation(id: number): boolean;
    /** 实现见 src/playernpc/PlayerNpcExporter.cpp */
    playerNpcStopAnimationTo(id: number, playerName: string): boolean;
    /** 实现见 src/playernpc/PlayerNpcExporter.cpp */
    playerNpcUnregisterSkin(skinId: string): boolean;
    /** 实现见 src/sensing/PlayerSensingExporter.cpp */
    sensingDeviceCode(playerName: string): number;
    /** 实现见 src/sensing/PlayerSensingExporter.cpp */
    sensingDeviceOf(playerName: string): string;
    /** 实现见 src/sensing/PlayerSensingExporter.cpp */
    sensingIsTouch(playerName: string): boolean;
    /** 实现见 src/RemoteCallExporter.cpp */
    shapeClearBackgroundColor(id: number): boolean;
    /** 实现见 src/RemoteCallExporter.cpp */
    shapeClearRotation(id: number): boolean;
    /** 实现见 src/RemoteCallExporter.cpp */
    shapeCreateArrow(x1: number, y1: number, z1: number, x2: number, y2: number, z2: number): number;
    /** 实现见 src/RemoteCallExporter.cpp */
    shapeCreateBox(x1: number, y1: number, z1: number, x2: number, y2: number, z2: number): number;
    /** 实现见 src/RemoteCallExporter.cpp */
    shapeCreateCircle(x: number, y: number, z: number, scale: number): number;
    /** 实现见 src/RemoteCallExporter.cpp */
    shapeCreateLine(x1: number, y1: number, z1: number, x2: number, y2: number, z2: number): number;
    /** 实现见 src/RemoteCallExporter.cpp */
    shapeCreateSphere(x: number, y: number, z: number, scale: number): number;
    /** 实现见 src/RemoteCallExporter.cpp */
    shapeCreateText(x: number, y: number, z: number, text: string): number;
    /** 实现见 src/RemoteCallExporter.cpp */
    shapeDestroy(id: number): boolean;
    /** 实现见 src/RemoteCallExporter.cpp */
    shapeDestroyAll(): any;
    /** 实现见 src/RemoteCallExporter.cpp */
    shapeDestroyBatch(ids: number): boolean;
    /** 实现见 src/RemoteCallExporter.cpp */
    shapeDraw(id: number): boolean;
    /** 实现见 src/RemoteCallExporter.cpp */
    shapeDrawBatch(ids: number): boolean;
    /** 实现见 src/RemoteCallExporter.cpp */
    shapeDrawToDimension(id: number, dimId: number): boolean;
    /** 实现见 src/RemoteCallExporter.cpp */
    shapeDrawToPlayer(id: number, playerName: string): boolean;
    /** 实现见 src/RemoteCallExporter.cpp */
    shapeExists(id: number): boolean;
    /** 实现见 src/RemoteCallExporter.cpp */
    shapeFindTextByLocation(x: number, y: number, z: number, radius: number): number;
    /** 实现见 src/RemoteCallExporter.cpp */
    shapeFindTextByLocationAndContent(x: number, y: number, z: number, radius: number, text: string): number;
    /** 实现见 src/RemoteCallExporter.cpp */
    shapeGetAllShapeIds(): number;
    /** 实现见 src/RemoteCallExporter.cpp */
    shapeGetColor(id: number): number;
    /** 实现见 src/RemoteCallExporter.cpp */
    shapeGetLocation(id: number): number;
    /** 实现见 src/RemoteCallExporter.cpp */
    shapeGetRotation(id: number): number;
    /** 实现见 src/RemoteCallExporter.cpp */
    shapeGetShapeType(id: number): number;
    /** 实现见 src/RemoteCallExporter.cpp */
    shapeGetText(id: number): string;
    /** 实现见 src/RemoteCallExporter.cpp */
    shapeRemove(id: number): boolean;
    /** 实现见 src/RemoteCallExporter.cpp */
    shapeRemoveToDimension(id: number, dimId: number): boolean;
    /** 实现见 src/RemoteCallExporter.cpp */
    shapeRemoveToPlayer(id: number, playerName: string): boolean;
    /** 实现见 src/RemoteCallExporter.cpp */
    shapeSetBackgroundColor(id: number, r: number, g: number, b: number, a: number): boolean;
    /** 实现见 src/RemoteCallExporter.cpp */
    shapeSetColor(id: number, r: number, g: number, b: number, a: number): boolean;
    /** 实现见 src/RemoteCallExporter.cpp */
    shapeSetDepthTest(id: number, enabled: boolean): boolean;
    /** 实现见 src/RemoteCallExporter.cpp */
    shapeSetDimension(id: number, dimId: number): boolean;
    /** 实现见 src/RemoteCallExporter.cpp */
    shapeSetDuration(id: number, seconds: number): boolean;
    /** 实现见 src/RemoteCallExporter.cpp */
    shapeSetLocation(id: number, x: number, y: number, z: number): boolean;
    /** 实现见 src/RemoteCallExporter.cpp */
    shapeSetRotation(id: number, pitch: number, yaw: number, roll: number): boolean;
    /** 实现见 src/RemoteCallExporter.cpp */
    shapeSetScale(id: number, scale: number): boolean;
    /** 实现见 src/RemoteCallExporter.cpp */
    shapeSetText(id: number, text: string): boolean;
    /** 实现见 src/RemoteCallExporter.cpp */
    shapeUpdate(id: number): boolean;
    /** 实现见 src/RemoteCallExporter.cpp */
    shapeUpdateToDimension(id: number, dimId: number): boolean;
    /** 实现见 src/RemoteCallExporter.cpp */
    shapeUpdateToPlayer(id: number, playerName: string): boolean;
    /** 实现见 src/sulfur/SulfurDisplayExporter.cpp */
    sulfurCreate(x: number, y: number, z: number, dim: number, blockType: string, blockCount: number, blockDamage: number, blockName: string, archetype: string, invisible: boolean): number;
    /** 实现见 src/sulfur/SulfurDisplayExporter.cpp */
    sulfurDestroy(id: number): boolean;
    /** 实现见 src/sulfur/SulfurDisplayExporter.cpp */
    sulfurDestroyAll(): any;
    /** 实现见 src/sulfur/SulfurDisplayExporter.cpp */
    sulfurExists(id: number): boolean;
    /** 实现见 src/sulfur/SulfurDisplayExporter.cpp */
    sulfurGetEntityId(id: number): number;
    /** 实现见 src/sulfur/SulfurDisplayExporter.cpp */
    sulfurGetIds(): number;
    /** 实现见 src/sulfur/SulfurDisplayExporter.cpp */
    sulfurSetArchetype(id: number, archetype: string): boolean;
    /** 实现见 src/sulfur/SulfurDisplayExporter.cpp */
    sulfurSetBlock(id: number, type: string, count: number, damage: number, name: string): boolean;
    /** 实现见 src/sulfur/SulfurDisplayExporter.cpp */
    sulfurSetInvisible(id: number, on: boolean): boolean;
    /** 实现见 src/sulfur/SulfurDisplayExporter.cpp */
    sulfurSetScale(id: number, scale: number): boolean;
    /** 实现见 src/trade/TradeMenuExporter.cpp */
    tradeAddOffer(id: number, buyAType: string, buyACount: number, buyBType: string, buyBCount: number, sellType: string, sellCount: number, sellName: string, tier: number, locked: boolean): boolean;
    /** 实现见 src/trade/TradeMenuExporter.cpp */
    tradeClose(id: number): boolean;
    /** 实现见 src/trade/TradeMenuExporter.cpp */
    tradeCloseAll(): any;
    /** 实现见 src/trade/TradeMenuExporter.cpp */
    tradeGetIds(): number;
    /** 实现见 src/trade/TradeMenuExporter.cpp */
    tradeIsOpen(id: number): boolean;
    /** 实现见 src/trade/TradeMenuExporter.cpp */
    tradeIsSettle(id: number): boolean;
    /** 实现见 src/trade/TradeMenuExporter.cpp */
    tradeOpen(playerName: string, tradeType: string, tier: number, experience: number, realTrades: boolean, carrierIdentifier: string): number;
    /** 实现见 src/trade/TradeMenuExporter.cpp */
    tradePollSettlements(): string;
    /** 实现见 src/trade/TradeMenuExporter.cpp */
    tradeSetSettle(id: number, on: boolean): boolean;
    /** 实现见 src/trade/TradeMenuExporter.cpp */
    tradeSetTier(id: number, tier: number, experience: number): boolean;
    /** 实现见 src/view/ViewOverrideExporter.cpp */
    viewBlock(playerName: string, x: number, y: number, z: number, type: string): boolean;
    /** 实现见 src/view/ViewOverrideExporter.cpp */
    viewClearAll(playerName: string): any;
    /** 实现见 src/view/ViewOverrideExporter.cpp */
    viewClearBlock(playerName: string, x: number, y: number, z: number): boolean;
    /** 实现见 src/view/ViewOverrideExporter.cpp */
    viewClearEntity(playerName: string, uniqueId: number): boolean;
    /** 实现见 src/view/ViewOverrideExporter.cpp */
    viewClearId(uniqueId: number): boolean;
    /** 实现见 src/view/ViewOverrideExporter.cpp */
    viewDescribe(playerName: string): string;
    /** 实现见 src/view/ViewOverrideExporter.cpp */
    viewEcho(text: string): string;
    /** 实现见 src/view/ViewOverrideExporter.cpp */
    viewEntity(playerName: string, uniqueId: number, spec: string): boolean;
    /** 实现见 src/view/ViewOverrideExporter.cpp */
    viewItemSpawnHooked(): boolean;
    /** 实现见 src/view/ViewOverrideExporter.cpp */
    viewMetaProbe(spec: string): string;
    /** 实现见 src/view/ViewOverrideExporter.cpp */
    viewPollSpawnEvents(): string;
    /** 实现见 src/view/ViewOverrideExporter.cpp */
    viewSelf(playerName: string, spec: string): boolean;
    /** 实现见 src/view/ViewOverrideExporter.cpp */
    viewSelfClear(playerName: string): boolean;
    /** 实现见 src/view/ViewOverrideExporter.cpp */
    viewSelfHide(playerName: string, hidden: boolean): boolean;
    /** 实现见 src/view/ViewOverrideExporter.cpp */
    viewSelfName(playerName: string, nametag: string, alwaysShow: boolean): boolean;
    /** 实现见 src/view/ViewOverrideExporter.cpp */
    viewSelfSkin(playerName: string, skinOfPlayer: string): boolean;
    /** 实现见 src/view/ViewOverrideExporter.cpp */
    viewSelfType(playerName: string, type: string): boolean;
    /** 实现见 src/view/ViewOverrideExporter.cpp */
    viewTargetType(uniqueId: number, type: string): boolean;
    /** 实现见 src/view/ViewOverrideExporter.cpp */
    viewUniqueIdOf(playerName: string): number;
  }

  // ── 按类分组（同 GMLIB-LegacyRemoteCallApi 的写法）──
  interface Container {
    blockEvent(menuId: number, b0: number, b1: number): boolean;
    clearClicks(): any;
    close(id: number): boolean;
    closeAll(): any;
    getIds(): number;
    isOpen(id: number): boolean;
    open(playerName: string, title: string, rows: number): number;
    pollClicks(): string;
    setInteractive(id: number, on: boolean): boolean;
    setItem(id: number, slot: number, type: string, count: number, damage: number, name: string, loreCsv: string): boolean;
    setTitle(id: number, title: string): boolean;
  }

  interface Entity {
    clearMobProperties(id: number): boolean;
    clearPlayerAppearance(id: number, playerName: string): boolean;
    clearPlayerRotation(id: number, playerName: string): boolean;
    clearPlayerRotations(id: number): boolean;
    clearRide(id: number): boolean;
    clearSpawns(): any;
    clearVisiblePlayers(id: number): boolean;
    create(identifier: string, x: number, y: number, z: number, dim: number): number;
    createAdvanced(identifier: string, x: number, y: number, z: number, dim: number, yaw: number, pitch: number, scale: number, nametag: string): number;
    createRandom(identifier: string, x: number, y: number, z: number, dim: number): number;
    createWithId(identifier: string, x: number, y: number, z: number, dim: number, desiredId: number): number;
    destroy(id: number): boolean;
    destroyAll(): any;
    exists(id: number): boolean;
    findNearest(x: number, y: number, z: number, dim: number, maxDist: number): number;
    getAllIds(): number;
    getInfo(id: number): string;
    isIdUsed(id: number): boolean;
    playAnimation(id: number, animation: string, stopExpression: string, durationTicks: number): boolean;
    playAnimationTo(id: number, playerName: string, animation: string, stopExpression: string, durationTicks: number): boolean;
    pollSpawns(): string;
    scaleBy(id: number, factor: number): boolean;
    setColorIndex(id: number, colorIndex: number): boolean;
    setEnabled(id: number, enabled: boolean): boolean;
    setEquipmentSlot(id: number, slot: number, name: string, aux: number, nbt: string): boolean;
    setFlags(id: number, flags: number): boolean;
    setIdentifier(id: number, identifier: string): boolean;
    setInvisible(id: number, on: boolean): boolean;
    setMarkVariant(id: number, markVariant: number): boolean;
    setMobProperty(id: number, name: string, value: string): boolean;
    setNametag(id: number, text: string): boolean;
    setPlayerEquipmentSlot(id: number, playerName: string, slot: number, name: string, aux: number, nbt: string): boolean;
    setPlayerNametag(id: number, playerName: string, text: string): boolean;
    setPlayerRotation(id: number, playerName: string, yaw: number, pitch: number): boolean;
    setPlayerScale(id: number, playerName: string, scale: number): boolean;
    setPose(id: number, pose: number): boolean;
    setPosition(id: number, x: number, y: number, z: number, dim: number): boolean;
    setRideEntity(id: number, vehicleEntityId: number): boolean;
    setRidePlayer(id: number, playerName: string): boolean;
    setRotation(id: number, yaw: number, pitch: number): boolean;
    setScale(id: number, scale: number): boolean;
    setVariant(id: number, variant: number): boolean;
    setViewDistance(id: number, dist: number): boolean;
    setVisiblePlayer(id: number, playerName: string): boolean;
    setVisiblePlayers(id: number, playerNames: string): boolean;
    stopAnimation(id: number): boolean;
    stopAnimationTo(id: number, playerName: string): boolean;
  }

  interface FakeInventory {
    apply(playerName: string, itemsSpec: string, refreshIntervalTicks: number): boolean;
    clear(playerName: string): boolean;
    clearAll(): any;
    clearClicks(): any;
    getPlayers(): string;
    isActive(playerName: string): boolean;
    pollClicks(): string;
    refresh(playerName: string): boolean;
    setSlot(playerName: string, slot: number, type: string, count: number, damage: number, name: string, loreCsv: string): boolean;
  }

  interface Form {
    close(playerName: string): boolean;
    closeAll(): number;
  }

  interface Ghost {
    clearInteractions(): any;
    pollInteractions(): string;
  }

  interface GradientLine {
    create(x1: number, y1: number, z1: number, x2: number, y2: number, z2: number, segments: number): number;
    destroy(id: number): boolean;
    destroyAll(): any;
    draw(id: number): boolean;
    drawToDimension(id: number, dimId: number): boolean;
    remove(id: number): boolean;
    setColor(id: number, r: number, g: number, b: number, a: number): boolean;
    setEndpoints(id: number, x1: number, y1: number, z1: number, x2: number, y2: number, z2: number): boolean;
    setGradient(id: number, r1: number, g1: number, b1: number, r2: number, g2: number, b2: number): boolean;
    setRainbow(id: number, speed: number): boolean;
    tick(deltaTime: number): any;
  }

  interface Hologram {
    addLine(id: number, text: string): boolean;
    apiVersion(): number;
    clearBackgroundColor(id: number): boolean;
    clearFollowPlayer(id: number): boolean;
    clearLines(id: number): boolean;
    clearRotation(id: number): boolean;
    create(x: number, y: number, z: number): number;
    destroy(id: number): boolean;
    destroyAll(): any;
    draw(id: number): boolean;
    drawToDimension(id: number, dimId: number): boolean;
    drawToPlayer(id: number, playerName: string): boolean;
    getLineCount(id: number): number;
    info(): string;
    refresh(id: number): boolean;
    remove(id: number): boolean;
    removeLine(id: number, lineIndex: number): boolean;
    setBackgroundColor(id: number, r: number, g: number, b: number, a: number): boolean;
    setColor(id: number, r: number, g: number, b: number, a: number): boolean;
    setDepthTest(id: number, enabled: boolean): boolean;
    setDimension(id: number, dimId: number): boolean;
    setFollowPlayer(id: number, playerName: string, offsetY: number): boolean;
    setLineParseVariables(id: number, lineIndex: number, enabled: boolean): boolean;
    setLinePool(id: number, lineIndex: number, content: string, intervalMs: number): boolean;
    setLineText(id: number, lineIndex: number, text: string): boolean;
    setLocation(id: number, x: number, y: number, z: number): boolean;
    setRotation(id: number, pitch: number, yaw: number, roll: number): boolean;
    setScale(id: number, scale: number): boolean;
  }

  interface ItemActor {
    destroy(id: number): boolean;
    destroyAll(): any;
    exists(id: number): boolean;
    getIds(): number;
    moveTo(id: number, x: number, y: number, z: number): boolean;
    setMotion(id: number, vx: number, vy: number, vz: number): boolean;
    spawn(itemId: string, aux: number, count: number, x: number, y: number, z: number, dim: number, vx: number, vy: number, vz: number): number;
    take(id: number, playerName: string): boolean;
  }

  interface ItemDetail {
    hide(id: number): boolean;
    show(dimId: number, x: number, y: number, z: number, itemId: string, aux: number, count: number, customText: string): number;
  }

  interface ItemDisplay {
    clearAnim(id: number): boolean;
    clearVisiblePlayers(id: number): boolean;
    create(x: number, y: number, z: number, dim: number, item: string, aux: number): number;
    createAdvanced(x: number, y: number, z: number, dim: number, item: string, aux: number, offX: string, offY: string, offZ: string, rotX: string, rotY: string, rotZ: string, scale: string): number;
    createFull(x: number, y: number, z: number, dim: number, item: string, aux: number, offX: string, offY: string, offZ: string, rotX: string, rotY: string, rotZ: string, scale: string, nbt: string, glint: boolean): number;
    createRandom(x: number, y: number, z: number, dim: number, item: string, aux: number): number;
    createSeamless(x: number, y: number, z: number, dim: number, item: string, aux: number, offX: string, offY: string, offZ: string, rotX: string, rotY: string, rotZ: string, scale: string, mode: number, viewDistance: number, visiblePlayer: string): number;
    createWithId(x: number, y: number, z: number, dim: number, item: string, aux: number, desiredId: number): number;
    destroy(id: number): boolean;
    destroyAll(): any;
    emitForEntity(playerName: string, uniqueId: number, item: string, aux: number, offsetY: string, rotX: string, rotY: string, rotZ: string, scale: string, nbt: string, glint: boolean): number;
    exists(id: number): boolean;
    findNearest(x: number, y: number, z: number, dim: number, maxDist: number): number;
    follow(id: number, playerName: string, offX: number, offY: number, offZ: number): boolean;
    getAllIds(): number;
    getInfo(id: number): string;
    isIdUsed(id: number): boolean;
    rotateY(id: number, delta: number): boolean;
    scaleBy(id: number, factor: number): boolean;
    scaleTo(id: number, targetScale: number): boolean;
    setAnim(id: number, spinSpeed: number, spinAxis: number, bobAmp: number, bobPeriod: number, pulseAmp: number, pulsePeriod: number, swayAmp: number, swayPeriod: number, phase: number, billboard: boolean): boolean;
    setBaseOffset(id: number, ox: string, oy: string, oz: string): boolean;
    setEnabled(id: number, enabled: boolean): boolean;
    setExtend(id: number, scale: string, rx: string, ry: string): boolean;
    setGlint(id: number, on: boolean): boolean;
    setHitbox(id: number, width: number, height: number): boolean;
    setItem(id: number, item: string, aux: number): boolean;
    setItemWithNbt(id: number, item: string, aux: number, nbt: string): boolean;
    setMode(id: number, mode: number): boolean;
    setOffset(id: number, ox: string, oy: string, oz: string): boolean;
    setOffsetLive(id: number, ox: string, oy: string, oz: string): boolean;
    setPosition(id: number, x: number, y: number, z: number, dim: number): boolean;
    setPreset(id: number, preset: string, a: number, b: number, c: number): boolean;
    setRotation(id: number, rx: string, ry: string, rz: string): boolean;
    setRotationLive(id: number, rx: string, ry: string, rz: string): boolean;
    setScale(id: number, scale: string): boolean;
    setScaleLive(id: number, scale: string): boolean;
    setStretch(id: number, xz: string, y: string): boolean;
    setStretchLive(id: number, xz: string, y: string): boolean;
    setTransformLive(id: number, ox: string, oy: string, oz: string, rx: string, ry: string, rz: string, scale: string): boolean;
    setViewDistance(id: number, dist: number): boolean;
    setVisiblePlayer(id: number, playerName: string): boolean;
    setVisiblePlayers(id: number, playerNames: string): boolean;
    unfollow(id: number): boolean;
  }

  interface LocatorBar {
    clear(playerName: string): number;
    count(playerName: string): number;
    hideFromOthers(playerName: string, hidden: boolean): boolean;
    isHiddenFromOthers(playerName: string): boolean;
    remove(playerName: string, key: string): boolean;
    set(playerName: string, key: string, x: number, y: number, z: number, dim: number, color: number, visible: boolean): boolean;
  }

  interface NpcDialog {
    clearClicks(): any;
    close(id: number): boolean;
    closeAll(): any;
    getIds(): number;
    isOpen(id: number): boolean;
    open(playerName: string, npcName: string, sceneName: string, dialogue: string, buttonsSpec: string, carrierIdentifier: string): number;
    pollClicks(): string;
    setAvatar(id: number, skinVariant: number, avatarViewSpec: string): boolean;
    update(id: number, dialogue: string, buttonsSpec: string): boolean;
  }

  interface Particle {
    createBox(playerName: string, dimId: number, cx: number, cy: number, cz: number, hx: number, hy: number, hz: number, step: number, effect: string, interval: number, lifetime: number): number;
    createBoxFaces(playerName: string, dimId: number, cx: number, cy: number, cz: number, hx: number, hy: number, hz: number, step: number, effect: string, interval: number, lifetime: number): number;
    createLine(playerName: string, dimId: number, x1: number, y1: number, z1: number, x2: number, y2: number, z2: number, step: number, effect: string, interval: number, lifetime: number): number;
    createPlane(playerName: string, dimId: number, cx: number, cy: number, cz: number, w: number, h: number, axis: number, step: number, effect: string, interval: number, lifetime: number): number;
    createPoint(playerName: string, dimId: number, x: number, y: number, z: number, effect: string, interval: number, lifetime: number): number;
    createPoly(playerName: string, dimId: number, verts: string, edges: string, step: number, effect: string, interval: number, lifetime: number): number;
    createRect(playerName: string, dimId: number, cx: number, cy: number, cz: number, w: number, h: number, axis: number, step: number, effect: string, interval: number, lifetime: number): number;
    destroy(id: number): boolean;
    follow(id: number, uuid: string, offX: number, offY: number, offZ: number): boolean;
    getInfo(id: number): string;
    moveBy(id: number, dx: number, dy: number, dz: number): boolean;
    moveTo(id: number, x: number, y: number, z: number, durationTicks: number): boolean;
    setEffect(id: number, effect: string): boolean;
    setInterval(id: number, ticks: number): boolean;
    setLifetime(id: number, ticks: number): boolean;
    setPos(id: number, x: number, y: number, z: number): boolean;
    setRot(id: number, rx: number, ry: number, rz: number): boolean;
    setScale(id: number, s: number): boolean;
    setViewDistance(id: number, blocks: number): boolean;
    setVisible(id: number, playersCsv: string): boolean;
    spin(id: number, sx: number, sy: number, sz: number): boolean;
    unfollow(id: number): boolean;
  }

  interface PlayerNpc {
    captureSkin(skinId: string, playerName: string): boolean;
    clearPlayerRotation(id: number, playerName: string): boolean;
    clearPlayerRotations(id: number): boolean;
    clearRide(id: number): boolean;
    clearSpawns(): any;
    clearVisiblePlayers(id: number): boolean;
    create(x: number, y: number, z: number, dim: number, name: string, skinId: string): number;
    createEx(x: number, y: number, z: number, dim: number, name: string, skinId: string, yaw: number, scale: number): number;
    createRandom(x: number, y: number, z: number, dim: number, name: string, skinId: string): number;
    createWithId(x: number, y: number, z: number, dim: number, name: string, skinId: string, desiredId: number): number;
    destroy(id: number): boolean;
    destroyAll(): any;
    exists(id: number): boolean;
    getAllIds(): number;
    getDebugInfo(id: number): string;
    getSkinBlobB64(skinId: string): string;
    getSkinIds(): string;
    hasSkin(skinId: string): boolean;
    importSkins(dirPath: string): number;
    injectSkin(viewerName: string, targetName: string, skinId: string): boolean;
    injectSkinAll(targetName: string, skinId: string): boolean;
    isIdUsed(id: number): boolean;
    playAnimation(id: number, animation: string, stopExpression: string, durationTicks: number): boolean;
    playAnimationTo(id: number, playerName: string, animation: string, stopExpression: string, durationTicks: number): boolean;
    playEmote(id: number, pieceId: string, lengthTicks: number): boolean;
    pollSpawns(): string;
    registerImageSkin(pngPath: string, skinId: string, sizeBlocks: number, transparent: boolean): boolean;
    registerSkin(pngPath: string, skinId: string, geometry: string, armSize: string): boolean;
    registerSkinData(pngPath: string, skinId: string, armSize: string, geometryData: string): boolean;
    registerSkinDataOpaque(pngPath: string, skinId: string, armSize: string, geometryData: string): boolean;
    registerSkinFromBlobB64(b64: string): boolean;
    registerSkinFromBlobFile(path: string): boolean;
    setEnabled(id: number, enabled: boolean): boolean;
    setEquipment(id: number, slot: number, name: string, aux: number, nbt: string): boolean;
    setNametag(id: number, text: string): boolean;
    setPlayerRotation(id: number, playerName: string, yaw: number): boolean;
    setPos(id: number, x: number, y: number, z: number, dim: number): boolean;
    setPosLight(id: number, x: number, y: number, z: number, dim: number): boolean;
    setPose(id: number, pose: number): boolean;
    setRideEntity(id: number, vehicleEntityId: number): boolean;
    setRidePlayer(id: number, playerName: string): boolean;
    setRideUniqueId(id: number, vehicleUniqueId: number): boolean;
    setRotLight(id: number, yaw: number): boolean;
    setRotation(id: number, yaw: number): boolean;
    setScale(id: number, scale: number): boolean;
    setSkin(id: number, skinId: string): boolean;
    setViewDistance(id: number, dist: number): boolean;
    setVisiblePlayer(id: number, playerName: string): boolean;
    setVisiblePlayers(id: number, playerNames: string): boolean;
    stopAnimation(id: number): boolean;
    stopAnimationTo(id: number, playerName: string): boolean;
    unregisterSkin(skinId: string): boolean;
  }

  interface Sensing {
    deviceCode(playerName: string): number;
    deviceOf(playerName: string): string;
    isTouch(playerName: string): boolean;
  }

  interface Shape {
    clearBackgroundColor(id: number): boolean;
    clearRotation(id: number): boolean;
    createArrow(x1: number, y1: number, z1: number, x2: number, y2: number, z2: number): number;
    createBox(x1: number, y1: number, z1: number, x2: number, y2: number, z2: number): number;
    createCircle(x: number, y: number, z: number, scale: number): number;
    createLine(x1: number, y1: number, z1: number, x2: number, y2: number, z2: number): number;
    createSphere(x: number, y: number, z: number, scale: number): number;
    createText(x: number, y: number, z: number, text: string): number;
    destroy(id: number): boolean;
    destroyAll(): any;
    destroyBatch(ids: number): boolean;
    draw(id: number): boolean;
    drawBatch(ids: number): boolean;
    drawToDimension(id: number, dimId: number): boolean;
    drawToPlayer(id: number, playerName: string): boolean;
    exists(id: number): boolean;
    findTextByLocation(x: number, y: number, z: number, radius: number): number;
    findTextByLocationAndContent(x: number, y: number, z: number, radius: number, text: string): number;
    getAllShapeIds(): number;
    getColor(id: number): number;
    getLocation(id: number): number;
    getRotation(id: number): number;
    getShapeType(id: number): number;
    getText(id: number): string;
    remove(id: number): boolean;
    removeToDimension(id: number, dimId: number): boolean;
    removeToPlayer(id: number, playerName: string): boolean;
    setBackgroundColor(id: number, r: number, g: number, b: number, a: number): boolean;
    setColor(id: number, r: number, g: number, b: number, a: number): boolean;
    setDepthTest(id: number, enabled: boolean): boolean;
    setDimension(id: number, dimId: number): boolean;
    setDuration(id: number, seconds: number): boolean;
    setLocation(id: number, x: number, y: number, z: number): boolean;
    setRotation(id: number, pitch: number, yaw: number, roll: number): boolean;
    setScale(id: number, scale: number): boolean;
    setText(id: number, text: string): boolean;
    update(id: number): boolean;
    updateToDimension(id: number, dimId: number): boolean;
    updateToPlayer(id: number, playerName: string): boolean;
  }

  interface Sulfur {
    create(x: number, y: number, z: number, dim: number, blockType: string, blockCount: number, blockDamage: number, blockName: string, archetype: string, invisible: boolean): number;
    destroy(id: number): boolean;
    destroyAll(): any;
    exists(id: number): boolean;
    getEntityId(id: number): number;
    getIds(): number;
    setArchetype(id: number, archetype: string): boolean;
    setBlock(id: number, type: string, count: number, damage: number, name: string): boolean;
    setInvisible(id: number, on: boolean): boolean;
    setScale(id: number, scale: number): boolean;
  }

  interface Trade {
    addOffer(id: number, buyAType: string, buyACount: number, buyBType: string, buyBCount: number, sellType: string, sellCount: number, sellName: string, tier: number, locked: boolean): boolean;
    close(id: number): boolean;
    closeAll(): any;
    getIds(): number;
    isOpen(id: number): boolean;
    isSettle(id: number): boolean;
    open(playerName: string, tradeType: string, tier: number, experience: number, realTrades: boolean, carrierIdentifier: string): number;
    pollSettlements(): string;
    setSettle(id: number, on: boolean): boolean;
    setTier(id: number, tier: number, experience: number): boolean;
  }

  interface View {
    block(playerName: string, x: number, y: number, z: number, type: string): boolean;
    clearAll(playerName: string): any;
    clearBlock(playerName: string, x: number, y: number, z: number): boolean;
    clearEntity(playerName: string, uniqueId: number): boolean;
    clearId(uniqueId: number): boolean;
    describe(playerName: string): string;
    echo(text: string): string;
    entity(playerName: string, uniqueId: number, spec: string): boolean;
    itemSpawnHooked(): boolean;
    metaProbe(spec: string): string;
    pollSpawnEvents(): string;
    self(playerName: string, spec: string): boolean;
    selfClear(playerName: string): boolean;
    selfHide(playerName: string, hidden: boolean): boolean;
    selfName(playerName: string, nametag: string, alwaysShow: boolean): boolean;
    selfSkin(playerName: string, skinOfPlayer: string): boolean;
    selfType(playerName: string, type: string): boolean;
    targetType(uniqueId: number, type: string): boolean;
    uniqueIdOf(playerName: string): number;
  }

  interface Visibility {
    getMask(playerName: string): string;
    isHidden(playerName: string): boolean;
    restore(playerName: string): boolean;
    setHidden(playerName: string, hidden: boolean): boolean;
    setMask(playerName: string, spawn: boolean, tab: boolean, motion: boolean, state: boolean, skin: boolean, alsoSelf: boolean): boolean;
  }

  interface Groups {
    Container: Container;
    Entity: Entity;
    FakeInventory: FakeInventory;
    Form: Form;
    Ghost: Ghost;
    GradientLine: GradientLine;
    Hologram: Hologram;
    ItemActor: ItemActor;
    ItemDetail: ItemDetail;
    ItemDisplay: ItemDisplay;
    LocatorBar: LocatorBar;
    NpcDialog: NpcDialog;
    Particle: Particle;
    PlayerNpc: PlayerNpc;
    Sensing: Sensing;
    Shape: Shape;
    Sulfur: Sulfur;
    Trade: Trade;
    View: View;
    Visibility: Visibility;
  }

  const groups: Groups;
}

declare const HL: HologramLib.Api & HologramLib.Groups;
export = HL;
