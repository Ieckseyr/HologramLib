# ABI 槽位表（0x012500 / ABI 戳 1）

> 本文件由 `python tools/abi_tool.py extract --md-out abi/slots.md` 生成，不要手改。
> 虚函数声明顺序 = 虚表槽位号。**只能在尾部追加**。

## IShapeDrawer

| # | 种类 | 签名 |
|---|---|---|
| 0 | default | `~IShapeDrawer()` |
| 1 | pure | `int64_t createText(float, float, float, std::string const&)` |
| 2 | pure | `int64_t createLine(float, float, float, float, float, float)` |
| 3 | pure | `int64_t createBox(float, float, float, float, float, float)` |
| 4 | pure | `int64_t createCircle(float, float, float, float)` |
| 5 | pure | `int64_t createSphere(float, float, float, float)` |
| 6 | pure | `int64_t createArrow(float, float, float, float, float, float)` |
| 7 | pure | `bool setColor(int64_t, float, float, float, float)` |
| 8 | pure | `bool setScale(int64_t, float)` |
| 9 | pure | `bool setDuration(int64_t, float)` |
| 10 | pure | `bool setDimension(int64_t, int)` |
| 11 | pure | `bool setLocation(int64_t, float, float, float)` |
| 12 | pure | `bool setText(int64_t, std::string const&)` |
| 13 | pure | `bool setRotation(int64_t, float, float, float)` |
| 14 | pure | `bool clearRotation(int64_t)` |
| 15 | pure | `bool draw(int64_t)` |
| 16 | pure | `bool drawToPlayer(int64_t, std::string const&)` |
| 17 | pure | `bool drawToDimension(int64_t, int)` |
| 18 | pure | `bool remove(int64_t)` |
| 19 | pure | `bool update(int64_t)` |
| 20 | pure | `bool destroy(int64_t)` |
| 21 | pure | `void destroyAll()` |
| 22 | pure | `bool exists(int64_t)` |
| 23 | pure | `ShapeType type(int64_t)` |
| 24 | pure | `bool setBackgroundColor(int64_t, float, float, float, float)` |
| 25 | pure | `bool clearBackgroundColor(int64_t)` |
| 26 | pure | `bool setDepthTest(int64_t, bool)` |

## IHologramText

| # | 种类 | 签名 |
|---|---|---|
| 0 | default | `~IHologramText()` |
| 1 | pure | `int64_t create(float, float, float)` |
| 2 | pure | `bool destroy(int64_t)` |
| 3 | pure | `void destroyAll()` |
| 4 | pure | `bool addLine(int64_t, std::string const&)` |
| 5 | pure | `bool setLineText(int64_t, int, std::string const&)` |
| 6 | body | `bool setLineScale(int64_t, int, float)` |
| 7 | pure | `bool removeLine(int64_t, int)` |
| 8 | pure | `bool clearLines(int64_t)` |
| 9 | pure | `int getLineCount(int64_t)` |
| 10 | pure | `bool setColor(int64_t, float, float, float, float)` |
| 11 | body | `bool setLineColor(int64_t, int, float, float, float, float)` |
| 12 | body | `bool setLineGradient(int64_t, int, float, float, float, float, float, float)` |
| 13 | body | `bool setLineRainbow(int64_t, int, float)` |
| 14 | body | `bool setLineScroll(int64_t, int, int, float)` |
| 15 | body | `bool setVerticalAnimation(int64_t, int, float, float)` |
| 16 | body | `bool setLineSpacing(int64_t, float)` |
| 17 | pure | `bool setLocation(int64_t, float, float, float)` |
| 18 | pure | `bool setFollowPlayer(int64_t, std::string const&, float)` |
| 19 | pure | `bool clearFollowPlayer(int64_t)` |
| 20 | pure | `bool draw(int64_t)` |
| 21 | pure | `bool drawToDimension(int64_t, int)` |
| 22 | pure | `bool drawToPlayer(int64_t, std::string const&)` |
| 23 | pure | `bool remove(int64_t)` |
| 24 | pure | `bool refresh(int64_t)` |
| 25 | body | `void tick(float)` |
| 26 | pure | `bool setDimension(int64_t, int)` |
| 27 | pure | `bool setScale(int64_t, float)` |
| 28 | pure | `bool setBackgroundColor(int64_t, float, float, float, float)` |
| 29 | pure | `bool clearBackgroundColor(int64_t)` |
| 30 | pure | `bool setDepthTest(int64_t, bool)` |
| 31 | pure | `bool setRotation(int64_t, float, float, float)` |
| 32 | pure | `bool clearRotation(int64_t)` |
| 33 | pure | `bool setLinePool(int64_t, int, std::vector<std::string> const&, int)` |
| 34 | pure | `bool setLineParseVariables(int64_t, int, bool)` |

## IItemDetail

| # | 种类 | 签名 |
|---|---|---|
| 0 | default | `~IItemDetail()` |
| 1 | pure | `int64_t show(int, float, float, float, std::string const&, int, int, std::string const& customText = "")` |
| 2 | pure | `bool hide(int64_t)` |

## IItemDisplay

| # | 种类 | 签名 |
|---|---|---|
| 0 | default | `~IItemDisplay()` |
| 1 | pure | `int64_t create(ItemDisplayConfig const&)` |
| 2 | pure | `bool destroy(int64_t)` |
| 3 | pure | `void destroyAll()` |
| 4 | pure | `bool exists(int64_t)` |
| 5 | pure | `bool get(int64_t, ItemDisplayConfig&)` |
| 6 | pure | `bool setItem(int64_t, std::string const&, int)` |
| 7 | pure | `bool setPosition(int64_t, float, float, float, int)` |
| 8 | pure | `bool setOffset(int64_t, std::string const&, std::string const&, std::string const&)` |
| 9 | pure | `bool setBaseOffset(int64_t, std::string const&, std::string const&, std::string const&)` |
| 10 | pure | `bool setRotation(int64_t, std::string const&, std::string const&, std::string const&)` |
| 11 | pure | `bool setScale(int64_t, std::string const&)` |
| 12 | pure | `bool setExtend(int64_t, std::string const&, std::string const&, std::string const&)` |
| 13 | pure | `bool setMode(int64_t, int)` |
| 14 | pure | `bool setEnabled(int64_t, bool)` |
| 15 | pure | `bool setViewDistance(int64_t, double)` |
| 16 | pure | `bool rotateY(int64_t, float)` |
| 17 | pure | `std::vector<int64_t> getAllIds()` |
| 18 | pure | `int64_t createRandom(ItemDisplayConfig const&)` |
| 19 | pure | `int64_t createWithId(ItemDisplayConfig const&, int64_t)` |
| 20 | pure | `bool isIdUsed(int64_t)` |
| 21 | pure | `bool scaleBy(int64_t, double)` |
| 22 | pure | `bool setItemWithNbt(int64_t, std::string const&, int, std::string const&)` |
| 23 | pure | `bool setGlint(int64_t, bool)` |
| 24 | pure | `bool follow(int64_t, std::string const&, float, float, float)` |
| 25 | pure | `bool unfollow(int64_t)` |
| 26 | pure | `bool setHitbox(int64_t, float, float)` |
| 27 | pure | `bool setStretch(int64_t, std::string const&, std::string const&)` |
| 28 | pure | `bool setStretchLive(int64_t, std::string const&, std::string const&)` |
| 29 | pure | `bool setRotationLive(int64_t, std::string const&, std::string const&, std::string const&)` |
| 30 | pure | `bool setOffsetLive(int64_t, std::string const&, std::string const&, std::string const&)` |
| 31 | pure | `bool setScaleLive(int64_t, std::string const&)` |
| 32 | pure | `bool setTransformLive(int64_t, std::string const&, std::string const&, std::string const&, std::string const&, std::string const&, std::string const&, std::string const&)` |
| 33 | pure | `bool setAnim(int64_t, ItemDisplayAnim const&)` |
| 34 | pure | `bool setAnimPreset(int64_t, std::string const&, float, float, int)` |
| 35 | pure | `bool clearAnim(int64_t)` |

## ICustomEntity

| # | 种类 | 签名 |
|---|---|---|
| 0 | default | `~ICustomEntity()` |
| 1 | pure | `int64_t create(CustomEntityConfig const&)` |
| 2 | pure | `int64_t createRandom(CustomEntityConfig const&)` |
| 3 | pure | `int64_t createWithId(CustomEntityConfig const&, int64_t)` |
| 4 | pure | `bool destroy(int64_t)` |
| 5 | pure | `void destroyAll()` |
| 6 | pure | `bool exists(int64_t)` |
| 7 | pure | `bool get(int64_t, CustomEntityConfig&)` |
| 8 | pure | `bool isIdUsed(int64_t)` |
| 9 | pure | `std::vector<int64_t> getAllIds()` |
| 10 | pure | `bool setIdentifier(int64_t, std::string const&)` |
| 11 | pure | `bool setPosition(int64_t, float, float, float, int)` |
| 12 | pure | `bool setRotation(int64_t, float, float)` |
| 13 | pure | `bool setNametag(int64_t, std::string const&)` |
| 14 | pure | `bool setScale(int64_t, float)` |
| 15 | pure | `bool setVariant(int64_t, int)` |
| 16 | pure | `bool setMarkVariant(int64_t, int)` |
| 17 | pure | `bool setColorIndex(int64_t, int)` |
| 18 | pure | `bool setFlags(int64_t, std::int64_t)` |
| 19 | pure | `bool setInvisible(int64_t, bool)` |
| 20 | pure | `bool setEnabled(int64_t, bool)` |
| 21 | pure | `bool setViewDistance(int64_t, double)` |
| 22 | pure | `bool setPose(int64_t, int)` |
| 23 | pure | `bool setEquipmentSlot(int64_t, int, std::string const&, int, std::string const&)` |
| 24 | pure | `bool setMobProperty(int64_t, std::string const&, std::string const&)` |
| 25 | pure | `bool clearMobProperties(int64_t)` |
| 26 | pure | `int64_t findNearest(float, float, float, int, double)` |
| 27 | pure | `bool scaleBy(int64_t, double)` |
| 28 | pure | `bool setVisiblePlayers(int64_t, std::vector<std::string> const&)` |
| 29 | pure | `bool clearVisiblePlayers(int64_t)` |
| 30 | pure | `bool setVisiblePlayer(int64_t, std::string const&)` |
| 31 | pure | `std::string getDebugInfo(int64_t)` |
| 32 | pure | `bool setRidePlayer(int64_t, std::string const&)` |
| 33 | pure | `bool setRideEntity(int64_t, int64_t)` |
| 34 | pure | `bool clearRide(int64_t)` |
| 35 | pure | `bool playAnimation(int64_t, std::string const&, std::string const&, int)` |
| 36 | pure | `bool playAnimationTo(int64_t, std::string const&, std::string const&, std::string const&, int)` |
| 37 | pure | `void setEntitySpawnCallback(EntitySpawnCallback)` |
| 38 | pure | `bool setPlayerRotation(int64_t, std::string const&, float, float)` |
| 39 | pure | `bool clearPlayerRotation(int64_t, std::string const&)` |
| 40 | pure | `bool clearPlayerRotations(int64_t)` |
| 41 | pure | `bool setPlayerNametag(int64_t, std::string const&, std::string const&)` |
| 42 | pure | `bool setPlayerScale(int64_t, std::string const&, float)` |
| 43 | pure | `bool setPlayerEquipmentSlot(int64_t, std::string const&, int, std::string const&, int, std::string const&)` |
| 44 | pure | `bool clearPlayerAppearance(int64_t, std::string const&)` |
| 45 | pure | `bool stopAnimation(int64_t)` |
| 46 | pure | `bool stopAnimationTo(int64_t, std::string const&)` |

## IParticleShape

| # | 种类 | 签名 |
|---|---|---|
| 0 | default | `~IParticleShape()` |
| 1 | pure | `int64_t createPoint(std::string const&, int, float, float, float, std::string const&, int, int)` |
| 2 | pure | `int64_t createLine(std::string const&, int, float, float, float, float, float, float, float, std::string const&, int, int)` |
| 3 | pure | `int64_t createRect(std::string const&, int, float, float, float, float, float, int, float, std::string const&, int, int)` |
| 4 | pure | `int64_t createPlane(std::string const&, int, float, float, float, float, float, int, float, std::string const&, int, int)` |
| 5 | pure | `int64_t createBox(std::string const&, int, float, float, float, float, float, float, float, std::string const&, int, int)` |
| 6 | pure | `int64_t createBoxFaces(std::string const&, int, float, float, float, float, float, float, float, std::string const&, int, int)` |
| 7 | pure | `int64_t createPoly(std::string const&, int, std::vector<float> const&, std::vector<std::int32_t> const&, float, std::string const&, int, int)` |
| 8 | pure | `bool setPos(int64_t, float, float, float)` |
| 9 | pure | `bool moveBy(int64_t, float, float, float)` |
| 10 | pure | `bool setRot(int64_t, float, float, float)` |
| 11 | pure | `bool spin(int64_t, float, float, float)` |
| 12 | pure | `bool setScale(int64_t, float)` |
| 13 | pure | `bool follow(int64_t, std::string const&, float, float, float)` |
| 14 | pure | `bool unfollow(int64_t)` |
| 15 | pure | `bool setEffect(int64_t, std::string const&)` |
| 16 | pure | `bool setVisiblePlayers(int64_t, std::vector<std::string> const&)` |
| 17 | pure | `bool clearVisiblePlayers(int64_t)` |
| 18 | pure | `bool setInterval(int64_t, int)` |
| 19 | pure | `bool setViewDistance(int64_t, int)` |
| 20 | pure | `bool setLifetime(int64_t, int)` |
| 21 | pure | `bool destroy(int64_t)` |
| 22 | pure | `void destroyAll()` |
| 23 | pure | `bool exists(int64_t)` |
| 24 | pure | `std::vector<int64_t> getAllIds()` |
| 25 | pure | `std::string getDebugInfo(int64_t)` |
| 26 | pure | `bool moveTo(int64_t, float, float, float, int)` |

## IPlayerNpc

| # | 种类 | 签名 |
|---|---|---|
| 0 | default | `~IPlayerNpc()` |
| 1 | pure | `bool registerSkin(PlayerNpcSkin const&)` |
| 2 | pure | `bool captureSkin(std::string const&, std::string const&)` |
| 3 | pure | `bool hasSkin(std::string const&)` |
| 4 | pure | `bool unregisterSkin(std::string const&)` |
| 5 | pure | `std::vector<std::string> getSkinIds()` |
| 6 | pure | `int64_t create(PlayerNpcConfig const&)` |
| 7 | pure | `int64_t createRandom(PlayerNpcConfig const&)` |
| 8 | pure | `int64_t createWithId(PlayerNpcConfig const&, int64_t)` |
| 9 | pure | `bool destroy(int64_t)` |
| 10 | pure | `void destroyAll()` |
| 11 | pure | `bool exists(int64_t)` |
| 12 | pure | `bool get(int64_t, PlayerNpcConfig&)` |
| 13 | pure | `bool isIdUsed(int64_t)` |
| 14 | pure | `std::vector<int64_t> getAllIds()` |
| 15 | pure | `bool setPosition(int64_t, float, float, float, int)` |
| 16 | pure | `bool setRotation(int64_t, float)` |
| 17 | pure | `bool setNametag(int64_t, std::string const&)` |
| 18 | pure | `bool setSkin(int64_t, std::string const&)` |
| 19 | pure | `bool setViewDistance(int64_t, double)` |
| 20 | pure | `bool setScale(int64_t, float)` |
| 21 | pure | `bool setEnabled(int64_t, bool)` |
| 22 | pure | `bool setVisiblePlayers(int64_t, std::vector<std::string> const&)` |
| 23 | pure | `bool clearVisiblePlayers(int64_t)` |
| 24 | pure | `bool setVisiblePlayer(int64_t, std::string const&)` |
| 25 | pure | `std::string getDebugInfo(int64_t)` |
| 26 | pure | `int importSkins(std::string const&)` |
| 27 | pure | `bool getSkinBlob(std::string const&, std::string&)` |
| 28 | pure | `bool registerSkinFromBlob(std::string const&)` |
| 29 | pure | `bool setRotationLight(int64_t, float)` |
| 30 | pure | `bool setPlayerRotation(int64_t, std::string const&, float)` |
| 31 | pure | `bool clearPlayerRotation(int64_t, std::string const&)` |
| 32 | pure | `bool clearPlayerRotations(int64_t)` |
| 33 | pure | `bool setPositionLight(int64_t, float, float, float, int)` |
| 34 | pure | `bool injectSkin(std::string const&, std::string const&, std::string const&)` |
| 35 | pure | `bool injectSkinAll(std::string const&, std::string const&)` |
| 36 | pure | `bool playAnimation(int64_t, std::string const&, std::string const&, int)` |
| 37 | pure | `bool playAnimationTo(int64_t, std::string const&, std::string const&, std::string const&, int)` |
| 38 | pure | `void setEntitySpawnCallback(EntitySpawnCallback)` |
| 39 | pure | `bool stopAnimation(int64_t)` |
| 40 | pure | `bool stopAnimationTo(int64_t, std::string const&)` |
| 41 | pure | `bool setPose(int64_t, int)` |
| 42 | pure | `bool setEquipment(int64_t, int, std::string const&, int, std::string const&)` |
| 43 | pure | `bool setRideUniqueId(int64_t, std::int64_t)` |
| 44 | pure | `bool setRidePlayer(int64_t, std::string const&)` |
| 45 | pure | `bool setRideEntity(int64_t, int64_t)` |
| 46 | pure | `bool clearRide(int64_t)` |

## ITradeMenu

| # | 种类 | 签名 |
|---|---|---|
| 0 | default | `~ITradeMenu()` |
| 1 | pure | `int64_t open(std::string const&, TradeMenuSpec const&)` |
| 2 | pure | `bool update(int64_t, TradeMenuSpec const&)` |
| 3 | pure | `bool close(int64_t)` |
| 4 | pure | `void closeAll()` |
| 5 | pure | `bool isOpen(int64_t)` |
| 6 | pure | `std::vector<int64_t> getAllIds()` |
| 7 | pure | `bool addOffer(int64_t, TradeMenuOffer const&)` |
| 8 | pure | `bool setTier(int64_t, int, int)` |
| 9 | pure | `bool setSettleLocally(int64_t, bool)` |
| 10 | pure | `bool isSettleLocally(int64_t)` |
| 11 | pure | `uint64_t addSettlementListener(std::function<void(TradeSettlementEvent const&)>)` |
| 12 | pure | `bool removeSettlementListener(uint64_t)` |
| 13 | pure | `std::vector<std::string> pollSettlements()` |

## IContainerMenu

| # | 种类 | 签名 |
|---|---|---|
| 0 | default | `~IContainerMenu()` |
| 1 | pure | `int64_t open(std::string const&, ContainerMenuSpec const&)` |
| 2 | pure | `bool close(int64_t)` |
| 3 | pure | `void closeAll()` |
| 4 | pure | `bool isOpen(int64_t)` |
| 5 | pure | `std::vector<int64_t> getAllIds()` |
| 6 | pure | `uint64_t addClickListener(std::function<void(ContainerClickEvent const&)>)` |
| 7 | pure | `bool removeClickListener(uint64_t)` |
| 8 | pure | `bool update(int64_t, ContainerMenuSpec const&)` |
| 9 | pure | `bool setItem(int64_t, int, ContainerMenuItem const&)` |
| 10 | pure | `bool setInteractive(int64_t, bool)` |

## INpcDialogue

| # | 种类 | 签名 |
|---|---|---|
| 0 | default | `~INpcDialogue()` |
| 1 | pure | `int64_t open(std::string const&, NpcDialogSpec const&)` |
| 2 | pure | `bool close(int64_t)` |
| 3 | pure | `void closeAll()` |
| 4 | pure | `bool isOpen(int64_t)` |
| 5 | pure | `std::vector<int64_t> getAllIds()` |
| 6 | pure | `uint64_t addClickListener(std::function<void(NpcDialogClickEvent const&)>)` |
| 7 | pure | `bool removeClickListener(uint64_t)` |
| 8 | pure | `bool update(int64_t, NpcDialogSpec const&)` |

## IPlayerSensing

| # | 种类 | 签名 |
|---|---|---|
| 0 | default | `~IPlayerSensing()` |
| 1 | pure | `ClientInputDevice inputDeviceOf(std::string const&)` |
| 2 | pure | `bool isTouch(std::string const&)` |

## IFakeInventory

| # | 种类 | 签名 |
|---|---|---|
| 0 | default | `~IFakeInventory()` |
| 1 | pure | `bool apply(std::string const&, FakeInventorySpec const&)` |
| 2 | pure | `bool setSlot(std::string const&, int, ContainerMenuItem const&)` |
| 3 | pure | `bool refresh(std::string const&)` |
| 4 | pure | `bool clear(std::string const&)` |
| 5 | pure | `void clearAll()` |
| 6 | pure | `bool isActive(std::string const&)` |
| 7 | pure | `std::vector<std::string> getActivePlayers()` |
| 8 | pure | `uint64_t addClickListener(std::function<void(FakeInventoryClickEvent const&)>)` |
| 9 | pure | `bool removeClickListener(uint64_t)` |

## ISulfurDisplay

| # | 种类 | 签名 |
|---|---|---|
| 0 | default | `~ISulfurDisplay()` |
| 1 | pure | `int64_t create(SulfurDisplaySpec const&)` |
| 2 | pure | `bool setBlock(int64_t, ContainerMenuItem const&)` |
| 3 | pure | `bool setArchetype(int64_t, std::string const&)` |
| 4 | pure | `bool setInvisible(int64_t, bool)` |
| 5 | pure | `bool setScale(int64_t, float)` |
| 6 | pure | `bool destroy(int64_t)` |
| 7 | pure | `void destroyAll()` |
| 8 | pure | `bool exists(int64_t)` |
| 9 | pure | `std::vector<int64_t> getAllIds()` |
| 10 | pure | `int64_t entityIdOf(int64_t)` |

## IViewOverride

| # | 种类 | 签名 |
|---|---|---|
| 0 | default | `~IViewOverride()` |
| 1 | pure | `bool overrideEntity(std::string const&, std::int64_t, EntityView const&)` |
| 2 | pure | `bool overrideBlock(std::string const&, int, int, int, BlockView const&)` |
| 3 | pure | `bool clearEntity(std::string const&, std::int64_t)` |
| 4 | pure | `bool clearBlock(std::string const&, int, int, int)` |
| 5 | pure | `void clearAll(std::string const&)` |
| 6 | pure | `std::string describeFor(std::string const&)` |

## IHologramLib

| # | 种类 | 签名 |
|---|---|---|
| 0 | default | `~IHologramLib()` |
| 1 | pure | `IShapeDrawer& shapes()` |
| 2 | pure | `IHologramText& holograms()` |
| 3 | pure | `IItemDetail& itemDetails()` |
| 4 | pure | `bool isLseAvailable()` |
| 5 | pure | `uint32_t version()` |
| 6 | pure | `IItemDisplay& itemDisplays()` |
| 7 | pure | `int64_t findNearestItemDisplay(float, float, float, int, double)` |
| 8 | pure | `ICustomEntity& customEntities()` |
| 9 | pure | `void setGhostInteractListener(std::function<void(GhostInteractEvent const&)>)` |
| 10 | pure | `void clearGhostInteractListener()` |
| 11 | pure | `std::vector<std::string> pollGhostInteractions()` |
| 12 | pure | `IParticleShape& particleShapes()` |
| 13 | pure | `IPlayerNpc& playerNpcs()` |
| 14 | pure | `uint64_t addGhostInteractListener(std::function<void(GhostInteractEvent const&)>)` |
| 15 | pure | `bool removeGhostInteractListener(uint64_t)` |
| 16 | pure | `ITradeMenu& tradeMenus()` |
| 17 | pure | `INpcDialogue& npcDialogs()` |
| 18 | pure | `IContainerMenu& containerMenus()` |
| 19 | pure | `IPlayerSensing& playerSensing()` |
| 20 | pure | `IFakeInventory& fakeInventories()` |
| 21 | pure | `ISulfurDisplay& sulfurDisplays()` |
| 22 | pure | `IViewOverride& viewOverrides()` |
| 23 | pure | `uint64_t addActorInteractListener(std::function<void(ActorInteractEvent const&)>)` |
| 24 | pure | `bool removeActorInteractListener(uint64_t)` |

## 结构体布局

| 结构体 | 字段（按最终布局顺序） |
|---|---|
| PerPlayerRotation | `float yaw` `float pitch` |
| ItemDisplayConfig | `std::string item` `int itemAux` `float x, y, z` `int dimension` `std::string offsetX` `std::string offsetY` `std::string offsetZ` `std::string baseOffsetX` `std::string baseOffsetY` `std::string baseOffsetZ` `std::string rotX` `std::string rotY` `std::string rotZ` `std::string scale` `std::string extendScale` `std::string extendRotX` `std::string extendRotY` `int mode` `double viewDistance` `bool enabled` `std::string itemNbt` `bool itemGlint` `float hitboxWidth` `float hitboxHeight` |
| ItemDisplayAnim | `float spinSpeed` `int spinAxis` `float bobAmplitude` `float bobPeriod` `float pulseAmplitude` `float pulsePeriod` `float swayAmplitude` `float swayPeriod` `float phase` `bool billboard` `float orbitRadius` `float orbitPeriod` `int orbitPlane` `float tumbleSpeed` `float wobbleAmplitude` `float wobbleSpeed` `float heartbeatAmplitude` `float heartbeatPeriod` `float pendulumAmplitude` `float pendulumPeriod` `bool billboardPitch` `float lodNear` `float lodFar` `float lodMinScale` `bool hideInFirstPerson` |
| CustomEntityEquipment | `std::string name` `int aux` `std::string nbt` |
| EntityMobProperty | `std::string name` `std::string value` |
| CustomEntityConfig | `std::string identifier` `float x, y, z` `int dimension` `float yaw, pitch` `std::string nametag` `bool nametagAlwaysShow` `float scale` `int variant` `int markVariant` `int colorIndex` `std::int64_t flags` `bool invisible` `double viewDistance` `bool enabled` `int pose` `CustomEntityEquipment equipment[6]` `std::string ridePlayerName` `int64_t rideEntityId` `std::vector<EntityMobProperty> mobProperties` |
| ActorInteractEvent | `std::string playerName` `std::string targetType` `int action` `std::uint64_t runtimeId` `std::int64_t uniqueId` `bool hasPos` `float x, y, z` |
| GhostInteractEvent | `std::string playerName` `int action` `std::string domain` `int64_t id` `bool hasPos` `float x, y, z` |
| PlayerNpcSkin | `std::string pngPath` `std::string skinId` `std::string geometry` `std::string armSize` `std::string geometryData` |
| PlayerNpcConfig | `std::string name` `std::string skinId` `float x, y, z` `int dimension` `float yaw` `double viewDistance` `float scale` `bool enabled` |
| TradeMenuItem | `std::string type` `int count` `int damage` `std::string name` `std::vector<std::string> lore` `bool isBlock` `int blockVersion` |
| TradeMenuOffer | `TradeMenuItem buyA` `TradeMenuItem buyB` `TradeMenuItem sell` `int tier` `int maxUses` `int traderExp` `bool locked` |
| TradeMenuSpec | `std::string tradeType` `int tier` `int experience` `bool usePacketOffers` `std::int64_t carrierUniqueIdOverride` `std::vector<TradeMenuOffer> offers` `std::string carrierIdentifier` `bool settleLocally` |
| TradeSettlementEvent | `std::string playerName` `int64_t menuId` `int offerIndex` `bool ok` `std::string reason` |
| NpcDialogButton | `std::string label` `std::vector<std::string> commands` `std::string actionId` `int mode` |
| NpcDialogSpec | `std::string carrierIdentifier` `std::string npcName` `std::string sceneName` `std::string dialogue` `std::int64_t npcUniqueIdOverride` `std::vector<NpcDialogButton> buttons` `std::string rawActionJson` `int avatarSkinVariant` `std::string avatarViewSpec` |
| NpcDialogClickEvent | `std::string playerName` `int64_t dialogId` `std::string sceneName` `int buttonIndex` `std::string actionId` `bool closed` `std::vector<std::string> commands` |
| ContainerMenuItem | `std::string type` `int count` `int damage` `std::string name` `std::vector<std::string> lore` |
| ContainerMenuSpec | `int rows` `std::vector<ContainerMenuItem> items` `bool useMinecart` `std::string title` `int openDelayTicks` `bool interactive` |
| ContainerClickEvent | `std::string playerName` `int64_t menuId` `int slot` `bool closed` |
| FakeInventorySpec | `std::vector<ContainerMenuItem> items` `int refreshIntervalTicks` |
| FakeInventoryClickEvent | `std::string playerName` `int slot` |
| SulfurDisplaySpec | `float x` `float y` `float z` `int dim` `ContainerMenuItem block` `std::string archetype` `int variant` `bool invisible` `float scale` `double viewDistance` `std::vector<std::string> visiblePlayers` |
| EntityView | `std::string identifier` `std::string asPlayer` `std::string skinId` `float scale` `float yOffset` `bool hidden` `bool hasNametag` `std::string nametag` `bool nametagAlwaysShow` `int pose` `bool hasPose` `float metaScale` `bool hasMetaScale` `int variant` `bool hasVariant` `int markVariant` `bool hasMarkVariant` `int colorIndex` `bool hasColorIndex` `int color2` `bool hasColor2` |
| BlockView | `std::string type` |
