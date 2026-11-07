# 1.10 ITradeMenu · 交易菜单

手写 `UpdateTradePacket` 打开的村民交易界面：交易表完全由本库构造，载荷与 BDS 原生村民交易逐字节一致（`tests/check-trade-packet.bat` 整包对拍）。

- 头文件：`#include "hologramlib/HologramLib.h"`
- 获取方式：`hologramlib::IHologramLib::getInstance().tradeMenus()`
- 载体：打开时在玩家身后 5 格生成一只**隐身、仅该玩家可见**的假村民（`carrierIdentifier` 可换成流浪商人），关闭即删
- 时序：载体是异步送达客户端的，`UpdateTrade` 与经验条元数据在约 4 tick 后发送
- `tier`：1 基，`1` = 新手 … `5` = 大师（`kTradeTierNovice` / `kTradeTierMaster`），超出会被夹紧
- 坐标：载体按玩家**打开时**的位置与朝向摆在其身后 5 格，之后不参与世界逻辑
- 版本：交易域 1.21.0 起；`addOffer` / `setTier` 1.22.0；真结算 1.24.0

!!! info "两条路径（`TradeMenuSpec::usePacketOffers`）"

    - `true`（默认）：只发自建 `UpdateTrade`，服务端**没有**交易表。玩家往付费槽放东西的请求会被 BDS 拒掉、
      物品弹回，界面停在「不可成交」——纯展示。
    - `false`：给载体装**真实交易表**并调 BDS 的 `openTrading`，玩家是真的在交易（物品真的消耗，触屏可走完整流程）；
      这条路上 `settleLocally` 无效。
    - `carrierUniqueIdOverride != 0`：不生成载体，直接以该真实实体为交易对象（装它的原版交易表）。

!!! tip "要稳定成交，用 `usePacketOffers = false`"

    纯协议真结算（`settleLocally = true`）能扣款、能发产物，但客户端要求的「交易槽槽位更正」在本协议版本
    与协议库线格式不一致（照协议库手写会打崩客户端），所以放料可能被客户端撤回。稳定成交请用真实交易表或容器 UI。

## 数据结构

```cpp
struct TradeMenuItem {
    std::string              type;             // 物品 id，如 "minecraft:emerald"
    int                      count{1};         // 数量
    int                      damage{0};        // 数据值 / 变种
    std::string              name;             // 自定义显示名（支持 §）
    std::vector<std::string> lore;             // 说明行
    bool                     isBlock{false};   // true = 按方块物品编码
    int                      blockVersion{18168865}; // 方块调色板版本
};

struct TradeMenuOffer {
    TradeMenuItem buyA;               // 第一付费项（必填）
    TradeMenuItem buyB;               // 第二付费项；buyB.type 为空 = 没有
    TradeMenuItem sell;               // 产出
    int  tier{kTradeTierNovice};      // 1 基解锁栏；高于菜单 tier 即显示为未解锁
    int  maxUses{16};                 // 可交易次数上限
    int  traderExp{0};                // 成交给村民的经验
    bool locked{false};               // true = 无论菜单 tier 多高都显示未解锁
};

struct TradeMenuSpec {
    std::string tradeType{"entity.villager.butcher"}; // 交易类型（自由翻译键或任意串）
    int         tier{kTradeTierNovice};               // 显示栏值，1 基，超出夹紧
    int         experience{0};                        // 经验条当前经验
    bool        usePacketOffers{true};                // 见上：纯协议展示 / 真实交易表
    std::int64_t carrierUniqueIdOverride{0};          // 非 0 = 用该真实实体（不生成载体）
    std::vector<TradeMenuOffer> offers;               // 纯协议路径的交易表
    std::string carrierIdentifier{"minecraft:villager_v2"}; // 或 "minecraft:wandering_trader"
    bool        settleLocally{false};                 // 纯协议路径下由库自己结算
};

struct TradeSettlementEvent {
    std::string playerName;
    int64_t     menuId{-1};
    int         offerIndex{-1};  // 第几条交易（0 基）；定位不到为 -1
    bool        ok{false};       // true = 付费已扣、产物已进背包
    std::string reason;          // ok = false 时的原因
};
```

- `TradeMenuItem.type` : `std::string`<br>物品 id；`buyA` 必填，`buyB` 留空表示没有第二付费项，`sell` 是成交产物
- `TradeMenuItem.count` : `int`<br>数量，一般从 `1` 开始
- `TradeMenuItem.name` : `std::string`<br>自定义显示名，支持 `§`；空串 = 用物品默认名
- `TradeMenuItem.lore` : `std::vector<std::string>`<br>说明行，每项一行
- `TradeMenuItem.isBlock` : `bool`<br>按方块物品编码（带方块 NBT）；普通物品保持 `false`
- `TradeMenuOffer.tier` : `int`<br>这条交易属于第几栏（1 基）；高于菜单 `tier` 时客户端显示为未解锁
- `TradeMenuOffer.maxUses` : `int`<br>可交易次数上限（默认 `16`）
- `TradeMenuOffer.traderExp` : `int`<br>每次成交给村民的经验
- `TradeMenuOffer.locked` : `bool`<br>`true` = 强制显示为未解锁，与菜单 `tier` 无关
- `TradeMenuSpec.tradeType` : `std::string`<br>交易类型；可填任意自定义串，客户端认不出的键会原样显示
- `TradeMenuSpec.tier` : `int`<br>显示栏值，1 基；超出 `1 ~ 5` 会夹紧
- `TradeMenuSpec.experience` : `int`<br>经验条当前经验（不在包内，由载体实体的 `TradeExperience` 元数据驱动）
- `TradeMenuSpec.usePacketOffers` : `bool`<br>`true` 纯协议展示 / `false` 真实交易表（见上）
- `TradeMenuSpec.carrierUniqueIdOverride` : `std::int64_t`<br>非 `0` = 不生成载体，直接以该实体的 uniqueId 为交易对象
- `TradeMenuSpec.offers` : `std::vector<TradeMenuOffer>`<br>交易表；只在纯协议路径生效，真实交易表路径由实体自己的交易表决定
- `TradeMenuSpec.carrierIdentifier` : `std::string`<br>载体实体类型；空串按 `minecraft:villager_v2`
- `TradeMenuSpec.settleLocally` : `bool`<br>纯协议路径下由库自己结算（见「真结算」）

## 打开与关闭

### 打开一个交易界面

`int64_t open(std::string const& playerName, TradeMenuSpec const& spec)`

- 参数：
  - `playerName` : `std::string const&`<br>目标玩家名，必须在线
  - `spec` : `TradeMenuSpec const&`<br>交易类型 / 栏位 / 经验 / 载体 / 交易表，字段见「数据结构」
- 返回值：菜单 id（从 `1` 自增）；失败返回 `-1`（玩家不在线、载体实体生成失败、动态容器 id 用尽）
- 返回值类型：`int64_t`
- 示例：

    ```cpp
    auto& menus = hologramlib::IHologramLib::getInstance().tradeMenus();

    hologramlib::TradeMenuOffer offer;
    offer.buyA.type  = "minecraft:emerald";
    offer.buyA.count = 3;
    offer.sell.type  = "minecraft:diamond";
    offer.sell.count = 1;
    offer.sell.name  = "§b钻石";
    offer.tier       = 1;

    hologramlib::TradeMenuSpec spec;
    spec.tradeType = "entity.villager.butcher";
    spec.tier      = 1;
    spec.offers.push_back(offer);

    int64_t menuId = menus.open("Steve", spec);
    if (menuId < 0) {
        return; // 玩家不在线等
    }
    ```

!!! note "同一玩家只会有一个菜单"

    再次 `open` 会先关掉这个玩家上一个菜单（发 `ContainerClose`、删载体、退还暂扣物品），再开新的。
    原生流程里没有 `ContainerType=Trade` 的 `ContainerOpen` —— 交易界面只靠这个包打开。

### 重新下发整张交易表

`bool update(int64_t menuId, TradeMenuSpec const& spec)`

- 参数：
  - `menuId` : `int64_t`<br>菜单 id
  - `spec` : `TradeMenuSpec const&`<br>**整套新配置**（整个替换，不是增量合并）
- 返回值：成功 `true`；菜单不存在或玩家离线 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    spec.tradeType = "entity.villager.priest";
    spec.tier      = 3;
    menus.update(menuId, spec);
    ```

!!! note "`update` 换不了载体"

    纯协议路径会就地重发 `UpdateTrade`（不重开界面）；真实交易表路径会重装交易表并**重开界面**（`openTrading`）。
    载体（村民 / 流浪商人）在打开时就绑定了，换载体只能关掉重开。已追加的 `offers` 会被 `spec.offers` 整个覆盖。

### 关闭一个交易界面

`bool close(int64_t menuId)`

- 参数：
  - `menuId` : `int64_t`<br>菜单 id
- 返回值：成功 `true`（发 `ContainerClose`、删除载体）；菜单不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    menus.close(menuId);
    ```

!!! note "关闭会退还暂扣的物品"

    真结算模式下，玩家暂存在交易槽里的付费和「点一下拿起」的那份都会退回背包；
    背包放不下或玩家已离线时记为待退还，下次进服再补。

### 关闭全部交易界面

`void closeAll()`

- 参数：无
- 返回值：无
- 返回值类型：`void`
- 示例：

    ```cpp
    menus.closeAll();
    ```

### 查询菜单是否还开着

`bool isOpen(int64_t menuId) const`

- 参数：
  - `menuId` : `int64_t`<br>菜单 id
- 返回值：存在 `true`；已关闭或从来不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    if (menus.isOpen(menuId)) {
        menus.setTier(menuId, 3, 120);
    }
    ```

### 列出全部菜单 id

`std::vector<int64_t> getAllIds() const`

- 参数：无
- 返回值：当前所有打开的菜单 id；没有时是空数组
- 返回值类型：`std::vector<int64_t>`
- 示例：

    ```cpp
    for (int64_t menuId : menus.getAllIds()) {
        menus.close(menuId);
    }
    ```

## 内容（1.22.0）

### 追加一条交易并就地重发

`bool addOffer(int64_t menuId, TradeMenuOffer const& offer)`

- 参数：
  - `menuId` : `int64_t`<br>菜单 id
  - `offer` : `TradeMenuOffer const&`<br>要追加的一条交易
- 返回值：已追加并重发 `true`；菜单不存在或玩家离线 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    offer.buyA.type  = "minecraft:wheat";
    offer.buyA.count = 20;
    offer.sell.type  = "minecraft:bread";
    offer.sell.count = 6;

    menus.addOffer(menuId, offer); // 不重开界面
    ```

### 修改显示栏与经验

`bool setTier(int64_t menuId, int tier, int experience)`

- 参数：
  - `menuId` : `int64_t`<br>菜单 id
  - `tier` : `int`<br>新的显示栏值，1 基（`1` 新手 … `5` 大师），超出夹紧
  - `experience` : `int`<br>新的经验条当前经验
- 返回值：已修改并就地重发 `true`；菜单不存在或玩家离线 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    menus.setTier(menuId, 3, 120); // 抬到 3 级、经验条 120
    ```

## 真结算（1.24.0）

### 开关真结算

`bool setSettleLocally(int64_t menuId, bool on)`

- 参数：
  - `menuId` : `int64_t`<br>菜单 id
  - `on` : `bool`<br>`true` = 库接住付费放置 / 取回 / 成交请求并自己落地；`false` = 回到纯展示
- 返回值：切换成功 `true`；菜单不存在 `false`。只对 `usePacketOffers = true` 且没有 `carrierUniqueIdOverride` 的路径有效
- 返回值类型：`bool`
- 示例：

    ```cpp
    menus.setSettleLocally(menuId, true);
    ```

!!! note "真结算到底做了什么"

    界面仍是自建 `UpdateTrade`（服务端不放交易表），但库：
    从真实背包扣下付费并暂存 → 成交时校验账本、按条目把产物写进背包 → 关界面把没花掉的付费退回；
    「点一下拿起」的光标那份也由库托管。判定逻辑独立在 `src/trade/TradeSettlementLogic.h`，
    离线自检 `tests/check-trade-settlement.bat`（43 项）。

### 查询真结算是否开着

`bool isSettleLocally(int64_t menuId) const`

- 参数：
  - `menuId` : `int64_t`<br>菜单 id
- 返回值：开着 `true`；没开或菜单不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    if (menus.isSettleLocally(menuId)) {
        // 这个菜单会真的扣物品
    }
    ```

### 注册结算监听器

`uint64_t addSettlementListener(std::function<void(TradeSettlementEvent const&)> listener)`

- 参数：
  - `listener` : `std::function<void(TradeSettlementEvent const&)>`<br>每次成交成功 / 失败各回调一次；可注册多个
- 返回值：监听 token（从 `1` 开始）；传空函数返回 `0`
- 返回值类型：`uint64_t`
- 示例：

    ```cpp
    uint64_t token = menus.addSettlementListener([](hologramlib::TradeSettlementEvent const& e) {
        if (e.ok) {
            // e.playerName 成交了 e.offerIndex 这条
        } else {
            // e.reason：payment-missing / inventory-full / offer-out-of-range ...
        }
    });
    ```

### 移除结算监听器

`bool removeSettlementListener(uint64_t token)`

- 参数：
  - `token` : `uint64_t`<br>`addSettlementListener` 返回的 token
- 返回值：移除成功 `true`；token 不存在 `false`
- 返回值类型：`bool`
- 示例：

    ```cpp
    menus.removeSettlementListener(token);
    ```

### 轮询结算队列

`std::vector<std::string> pollSettlements()`

- 参数：无
- 返回值：一行一次结算，格式 `player=<名> menuId=<id> offer=<n> ok=<0|1> reason=<原因>`；队列空时是空数组
- 返回值类型：`std::vector<std::string>`
- 示例：

    ```cpp
    for (auto const& line : menus.pollSettlements()) {
        // "player=Steve menuId=3 offer=0 ok=1 reason="
        logger.info("{}", line);
    }
    ```

!!! note "取走并清空，队列上限 1024"

    只有这个函数会排空队列；只挂监听器、从不轮询时队列会一直增长到 1024 条，然后丢最旧的一条。
    `offer` = 定位到的第几条交易（`0` 基，`-1` = 定位不到），`ok=0` 时 `reason` 给原因，
    常见的有 `payment-missing`、`inventory-full`、`offer-out-of-range`。

## 完整示例

```cpp
#include "hologramlib/HologramLib.h"

int64_t open_diamond_shop(std::string const& playerName) {
    auto& menus = hologramlib::IHologramLib::getInstance().tradeMenus();

    hologramlib::TradeMenuSpec spec;
    spec.tradeType    = "entity.villager.butcher";
    spec.tier         = 1;
    spec.experience   = 0;
    spec.usePacketOffers = true;   // 纯协议展示
    spec.settleLocally   = true;   // 且由库自己结算

    hologramlib::TradeMenuOffer offer;
    offer.buyA.type  = "minecraft:emerald";
    offer.buyA.count = 3;
    offer.sell.type  = "minecraft:diamond";
    offer.sell.count = 1;
    offer.sell.name  = "§b钻石";
    offer.tier       = 1;
    offer.maxUses    = 16;
    spec.offers.push_back(offer);

    int64_t menuId = menus.open(playerName, spec);
    if (menuId < 0) {
        return -1;
    }

    // 结算结果：成功扣款 / 失败原因都能拿到
    menus.addSettlementListener([](hologramlib::TradeSettlementEvent const& e) {
        if (e.ok) {
            logger.info("{} 在菜单 {} 成交了第 {} 条", e.playerName, e.menuId, e.offerIndex);
        } else {
            logger.warn("结算失败: {} ({})", e.reason, e.offerIndex);
        }
    });

    return menuId;
}
```

---

[← 返回 LeviLamina 包装总览](index.md) · [ABI 总览](../index.md)
