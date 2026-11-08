# Endstone API 索引

全部 **442** 个导出，按域列出；点方法名进到对应域页的那一节，
每个方法都有签名、参数、返回类型与对应的 LSE 导出名。

命名规则：方法名 = LSE 导出名去掉域前缀后转 `snake_case`，域前缀对应插件属性（`lib.holograms`、`lib.entities` …）。

## 形状（46）

页面：[形状](shape.md)

- [`attach_entity`](shape.md#attach_entity) — `lib.api.attach_entity(...)` ← `shapeAttachEntity`
- [`clear_attached_entity`](shape.md#clear_attached_entity) — `lib.api.clear_attached_entity(...)` ← `shapeClearAttachedEntity`
- [`clear_background_color`](shape.md#clear_background_color) — `lib.api.clear_background_color(...)` ← `shapeClearBackgroundColor`
- [`clear_rotation`](shape.md#clear_rotation) — `lib.api.clear_rotation(...)` ← `shapeClearRotation`
- [`create_arrow`](shape.md#create_arrow) — `lib.api.create_arrow(...)` ← `shapeCreateArrow`
- [`create_box`](shape.md#create_box) — `lib.api.create_box(...)` ← `shapeCreateBox`
- [`create_circle`](shape.md#create_circle) — `lib.api.create_circle(...)` ← `shapeCreateCircle`
- [`create_cone`](shape.md#create_cone) — `lib.api.create_cone(...)` ← `shapeCreateCone`
- [`create_cylinder`](shape.md#create_cylinder) — `lib.api.create_cylinder(...)` ← `shapeCreateCylinder`
- [`create_ellipsoid`](shape.md#create_ellipsoid) — `lib.api.create_ellipsoid(...)` ← `shapeCreateEllipsoid`
- [`create_line`](shape.md#create_line) — `lib.api.create_line(...)` ← `shapeCreateLine`
- [`create_pyramid`](shape.md#create_pyramid) — `lib.api.create_pyramid(...)` ← `shapeCreatePyramid`
- [`create_sphere`](shape.md#create_sphere) — `lib.api.create_sphere(...)` ← `shapeCreateSphere`
- [`create_text`](shape.md#create_text) — `lib.api.create_text(...)` ← `shapeCreateText`
- [`destroy`](shape.md#destroy) — `lib.api.destroy(...)` ← `shapeDestroy`
- [`destroy_all`](shape.md#destroy_all) — `lib.api.destroy_all(...)` ← `shapeDestroyAll`
- [`destroy_batch`](shape.md#destroy_batch) — `lib.api.destroy_batch(...)` ← `shapeDestroyBatch`
- [`draw`](shape.md#draw) — `lib.api.draw(...)` ← `shapeDraw`
- [`draw_batch`](shape.md#draw_batch) — `lib.api.draw_batch(...)` ← `shapeDrawBatch`
- [`draw_to_dimension`](shape.md#draw_to_dimension) — `lib.api.draw_to_dimension(...)` ← `shapeDrawToDimension`
- [`draw_to_player`](shape.md#draw_to_player) — `lib.api.draw_to_player(...)` ← `shapeDrawToPlayer`
- [`exists`](shape.md#exists) — `lib.api.exists(...)` ← `shapeExists`
- [`find_text_by_location`](shape.md#find_text_by_location) — `lib.api.find_text_by_location(...)` ← `shapeFindTextByLocation`
- [`find_text_by_location_and_content`](shape.md#find_text_by_location_and_content) — `lib.api.find_text_by_location_and_content(...)` ← `shapeFindTextByLocationAndContent`
- [`get_all_shape_ids`](shape.md#get_all_shape_ids) — `lib.api.get_all_shape_ids(...)` ← `shapeGetAllShapeIds`
- [`get_color`](shape.md#get_color) — `lib.api.get_color(...)` ← `shapeGetColor`
- [`get_location`](shape.md#get_location) — `lib.api.get_location(...)` ← `shapeGetLocation`
- [`get_rotation`](shape.md#get_rotation) — `lib.api.get_rotation(...)` ← `shapeGetRotation`
- [`get_shape_type`](shape.md#get_shape_type) — `lib.api.get_shape_type(...)` ← `shapeGetShapeType`
- [`get_text`](shape.md#get_text) — `lib.api.get_text(...)` ← `shapeGetText`
- [`remove`](shape.md#remove) — `lib.api.remove(...)` ← `shapeRemove`
- [`remove_to_dimension`](shape.md#remove_to_dimension) — `lib.api.remove_to_dimension(...)` ← `shapeRemoveToDimension`
- [`remove_to_player`](shape.md#remove_to_player) — `lib.api.remove_to_player(...)` ← `shapeRemoveToPlayer`
- [`set_background_color`](shape.md#set_background_color) — `lib.api.set_background_color(...)` ← `shapeSetBackgroundColor`
- [`set_color`](shape.md#set_color) — `lib.api.set_color(...)` ← `shapeSetColor`
- [`set_depth_test`](shape.md#set_depth_test) — `lib.api.set_depth_test(...)` ← `shapeSetDepthTest`
- [`set_dimension`](shape.md#set_dimension) — `lib.api.set_dimension(...)` ← `shapeSetDimension`
- [`set_duration`](shape.md#set_duration) — `lib.api.set_duration(...)` ← `shapeSetDuration`
- [`set_location`](shape.md#set_location) — `lib.api.set_location(...)` ← `shapeSetLocation`
- [`set_max_render_distance`](shape.md#set_max_render_distance) — `lib.api.set_max_render_distance(...)` ← `shapeSetMaxRenderDistance`
- [`set_rotation`](shape.md#set_rotation) — `lib.api.set_rotation(...)` ← `shapeSetRotation`
- [`set_scale`](shape.md#set_scale) — `lib.api.set_scale(...)` ← `shapeSetScale`
- [`set_text`](shape.md#set_text) — `lib.api.set_text(...)` ← `shapeSetText`
- [`update`](shape.md#update) — `lib.api.update(...)` ← `shapeUpdate`
- [`update_to_dimension`](shape.md#update_to_dimension) — `lib.api.update_to_dimension(...)` ← `shapeUpdateToDimension`
- [`update_to_player`](shape.md#update_to_player) — `lib.api.update_to_player(...)` ← `shapeUpdateToPlayer`

## 悬浮字（28）

页面：[悬浮字](hologram.md)

- [`add_line`](hologram.md#add_line) — `lib.holograms.add_line(...)` ← `holoAddLine`
- `lib.holo_api_version` ← `holoApiVersion`（常量）
- [`clear_background_color`](hologram.md#clear_background_color) — `lib.holograms.clear_background_color(...)` ← `holoClearBackgroundColor`
- [`clear_follow_player`](hologram.md#clear_follow_player) — `lib.holograms.clear_follow_player(...)` ← `holoClearFollowPlayer`
- [`clear_lines`](hologram.md#clear_lines) — `lib.holograms.clear_lines(...)` ← `holoClearLines`
- [`clear_rotation`](hologram.md#clear_rotation) — `lib.holograms.clear_rotation(...)` ← `holoClearRotation`
- [`create`](hologram.md#create) — `lib.holograms.create(...)` ← `holoCreate`
- [`destroy`](hologram.md#destroy) — `lib.holograms.destroy(...)` ← `holoDestroy`
- [`destroy_all`](hologram.md#destroy_all) — `lib.holograms.destroy_all(...)` ← `holoDestroyAll`
- [`draw`](hologram.md#draw) — `lib.holograms.draw(...)` ← `holoDraw`
- [`draw_to_dimension`](hologram.md#draw_to_dimension) — `lib.holograms.draw_to_dimension(...)` ← `holoDrawToDimension`
- [`draw_to_player`](hologram.md#draw_to_player) — `lib.holograms.draw_to_player(...)` ← `holoDrawToPlayer`
- [`get_line_count`](hologram.md#get_line_count) — `lib.holograms.get_line_count(...)` ← `holoGetLineCount`
- [`info`](hologram.md#info) — `lib.holograms.info(...)` ← `holoInfo`
- [`refresh`](hologram.md#refresh) — `lib.holograms.refresh(...)` ← `holoRefresh`
- [`destroy`](hologram.md#destroy) — `lib.holograms.destroy(...)` ← `holoRemove`
- [`remove_line`](hologram.md#remove_line) — `lib.holograms.remove_line(...)` ← `holoRemoveLine`
- [`set_background_color`](hologram.md#set_background_color) — `lib.holograms.set_background_color(...)` ← `holoSetBackgroundColor`
- [`set_color`](hologram.md#set_color) — `lib.holograms.set_color(...)` ← `holoSetColor`
- [`set_depth_test`](hologram.md#set_depth_test) — `lib.holograms.set_depth_test(...)` ← `holoSetDepthTest`
- [`set_dimension`](hologram.md#set_dimension) — `lib.holograms.set_dimension(...)` ← `holoSetDimension`
- [`set_follow_player`](hologram.md#set_follow_player) — `lib.holograms.set_follow_player(...)` ← `holoSetFollowPlayer`
- [`set_line_parse_variables`](hologram.md#set_line_parse_variables) — `lib.holograms.set_line_parse_variables(...)` ← `holoSetLineParseVariables`
- [`set_line_pool`](hologram.md#set_line_pool) — `lib.holograms.set_line_pool(...)` ← `holoSetLinePool`
- [`set_line_text`](hologram.md#set_line_text) — `lib.holograms.set_line_text(...)` ← `holoSetLineText`
- [`set_location`](hologram.md#set_location) — `lib.holograms.set_location(...)` ← `holoSetLocation`
- [`set_rotation`](hologram.md#set_rotation) — `lib.holograms.set_rotation(...)` ← `holoSetRotation`
- [`set_scale`](hologram.md#set_scale) — `lib.holograms.set_scale(...)` ← `holoSetScale`

## 渐变线（11）

页面：[渐变线](gradient.md)

- [`create`](gradient.md#create) — `lib.gradients.create(...)` ← `gradientCreate`
- [`destroy`](gradient.md#destroy) — `lib.gradients.destroy(...)` ← `gradientDestroy`
- [`destroy_all`](gradient.md#destroy_all) — `lib.gradients.destroy_all(...)` ← `gradientDestroyAll`
- [`draw`](gradient.md#draw) — `lib.gradients.draw(...)` ← `gradientDraw`
- [`draw_to_dimension`](gradient.md#draw_to_dimension) — `lib.gradients.draw_to_dimension(...)` ← `gradientDrawToDimension`
- [`remove`](gradient.md#remove) — `lib.gradients.remove(...)` ← `gradientRemove`
- [`set_color`](gradient.md#set_color) — `lib.gradients.set_color(...)` ← `gradientSetColor`
- [`set_endpoints`](gradient.md#set_endpoints) — `lib.gradients.set_endpoints(...)` ← `gradientSetEndpoints`
- [`set_gradient`](gradient.md#set_gradient) — `lib.gradients.set_gradient(...)` ← `gradientSetGradient`
- [`set_rainbow`](gradient.md#set_rainbow) — `lib.gradients.set_rainbow(...)` ← `gradientSetRainbow`
- [`tick`](gradient.md#tick) — `lib.gradients.tick(...)` ← `gradientTick`

## 粒子（27）

页面：[粒子](particles.md)

- [`clear_visible_players`](particles.md#clear_visible_players) — `lib.particles.clear_visible_players(...)` ← `particleClearVisiblePlayers`
- [`create_box`](particles.md#create_box) — `lib.particles.create_box(...)` ← `particleCreateBox`
- [`create_box_faces`](particles.md#create_box_faces) — `lib.particles.create_box_faces(...)` ← `particleCreateBoxFaces`
- [`create_line`](particles.md#create_line) — `lib.particles.create_line(...)` ← `particleCreateLine`
- [`create_plane`](particles.md#create_plane) — `lib.particles.create_plane(...)` ← `particleCreatePlane`
- [`create_point`](particles.md#create_point) — `lib.particles.create_point(...)` ← `particleCreatePoint`
- [`create_poly`](particles.md#create_poly) — `lib.particles.create_poly(...)` ← `particleCreatePoly`
- [`create_rect`](particles.md#create_rect) — `lib.particles.create_rect(...)` ← `particleCreateRect`
- [`destroy`](particles.md#destroy) — `lib.particles.destroy(...)` ← `particleDestroy`
- [`destroy_all`](particles.md#destroy_all) — `lib.particles.destroy_all(...)` ← `particleDestroyAll`
- [`exists`](particles.md#exists) — `lib.particles.exists(...)` ← `particleExists`
- [`follow`](particles.md#follow) — `lib.particles.follow(...)` ← `particleFollow`
- [`get_all_ids`](particles.md#get_all_ids) — `lib.particles.get_all_ids(...)` ← `particleGetAllIds`
- [`get_info`](particles.md#get_info) — `lib.particles.get_info(...)` ← `particleGetInfo`
- [`move_by`](particles.md#move_by) — `lib.particles.move_by(...)` ← `particleMoveBy`
- [`move_to`](particles.md#move_to) — `lib.particles.move_to(...)` ← `particleMoveTo`
- [`set_effect`](particles.md#set_effect) — `lib.particles.set_effect(...)` ← `particleSetEffect`
- [`set_interval`](particles.md#set_interval) — `lib.particles.set_interval(...)` ← `particleSetInterval`
- [`set_lifetime`](particles.md#set_lifetime) — `lib.particles.set_lifetime(...)` ← `particleSetLifetime`
- [`set_pos`](particles.md#set_pos) — `lib.particles.set_pos(...)` ← `particleSetPos`
- [`set_rot`](particles.md#set_rot) — `lib.particles.set_rot(...)` ← `particleSetRot`
- [`set_scale`](particles.md#set_scale) — `lib.particles.set_scale(...)` ← `particleSetScale`
- [`set_view_distance`](particles.md#set_view_distance) — `lib.particles.set_view_distance(...)` ← `particleSetViewDistance`
- [`set_visible`](particles.md#set_visible) — `lib.particles.set_visible(...)` ← `particleSetVisible`
- [`set_visible_players`](particles.md#set_visible_players) — `lib.particles.set_visible_players(...)` ← `particleSetVisiblePlayers`
- [`spin`](particles.md#spin) — `lib.particles.spin(...)` ← `particleSpin`
- [`unfollow`](particles.md#unfollow) — `lib.particles.unfollow(...)` ← `particleUnfollow`

## Boss 条（10）

页面：[Boss 条](bossbar.md)

- [`add`](bossbar.md#add) — `lib.bossbar.add(...)` ← `bossBarAdd`
- [`add_on`](bossbar.md#add_on) — `lib.bossbar.add_on(...)` ← `bossBarAddOn`
- [`count`](bossbar.md#count) — `lib.bossbar.count(...)` ← `bossBarCount`
- [`get_info`](bossbar.md#get_info) — `lib.bossbar.get_info(...)` ← `bossBarGetInfo`
- [`probe`](bossbar.md#probe) — `lib.bossbar.probe(...)` ← `bossBarProbe`
- [`remove`](bossbar.md#remove) — `lib.bossbar.remove(...)` ← `bossBarRemove`
- [`remove_all`](bossbar.md#remove_all) — `lib.bossbar.remove_all(...)` ← `bossBarRemoveAll`
- [`set_health`](bossbar.md#set_health) — `lib.bossbar.set_health(...)` ← `bossBarSetHealth`
- [`set_style`](bossbar.md#set_style) — `lib.bossbar.set_style(...)` ← `bossBarSetStyle`
- [`set_title`](bossbar.md#set_title) — `lib.bossbar.set_title(...)` ← `bossBarSetTitle`

## 侧边栏（7）

页面：[侧边栏](sidebar.md)

- [`is_shown`](sidebar.md#is_shown) — `lib.sidebar.is_shown(...)` ← `sidebarIsShown`
- [`line_count`](sidebar.md#line_count) — `lib.sidebar.line_count(...)` ← `sidebarLineCount`
- [`remove`](sidebar.md#remove) — `lib.sidebar.remove(...)` ← `sidebarRemove`
- [`remove_all`](sidebar.md#remove_all) — `lib.sidebar.remove_all(...)` ← `sidebarRemoveAll`
- [`set`](sidebar.md#set) — `lib.sidebar.set(...)` ← `sidebarSet`
- [`set_line`](sidebar.md#set_line) — `lib.sidebar.set_line(...)` ← `sidebarSetLine`
- [`set_title`](sidebar.md#set_title) — `lib.sidebar.set_title(...)` ← `sidebarSetTitle`

## HUD 显隐（5）

页面：[HUD 显隐](hud.md)

- [`probe`](hud.md#probe) — `lib.hud.probe(...)` ← `hudProbe`
- [`reset_all`](hud.md#reset_all) — `lib.hud.reset_all(...)` ← `hudResetAll`
- [`set_element`](hud.md#set_element) — `lib.hud.set_element(...)` ← `hudSetElement`
- [`set_elements`](hud.md#set_elements) — `lib.hud.set_elements(...)` ← `hudSetElements`
- [`set_raw`](hud.md#set_raw) — `lib.hud.set_raw(...)` ← `hudSetRaw`

## 玩家表现（5）

页面：[玩家表现](player-effect.md)

- [`play_celebrate`](player-effect.md#play_celebrate) — `lib.playereffect.play_celebrate(...)` ← `playerPlayCelebrate`
- [`play_ominous_drink`](player-effect.md#play_ominous_drink) — `lib.playereffect.play_ominous_drink(...)` ← `playerPlayOminousDrink`
- [`show_bad_omen`](player-effect.md#show_bad_omen) — `lib.playereffect.show_bad_omen(...)` ← `playerShowBadOmen`
- [`show_effect`](player-effect.md#show_effect) — `lib.playereffect.show_effect(...)` ← `playerShowEffect`
- [`show_village_hero`](player-effect.md#show_village_hero) — `lib.playereffect.show_village_hero(...)` ← `playerShowVillageHero`

## Actor 表现（25）

页面：[Actor 表现](actor-effect.md)

- [`block_clear`](actor-effect.md#block_clear) — `lib.actoreffect.block_clear(...)` ← `actorBlockClear`
- [`block_event`](actor-effect.md#block_event) — `lib.actoreffect.block_event(...)` ← `actorBlockEvent`
- [`block_event_for`](actor-effect.md#block_event_for) — `lib.actoreffect.block_event_for(...)` ← `actorBlockEventFor`
- [`block_event_for_entity`](actor-effect.md#block_event_for_entity) — `lib.actoreffect.block_event_for_entity(...)` ← `actorBlockEventForEntity`
- [`block_stats`](actor-effect.md#block_stats) — `lib.actoreffect.block_stats(...)` ← `actorBlockStats`
- [`clear_interactions`](actor-effect.md#clear_interactions) — `lib.actoreffect.clear_interactions(...)` ← `actorClearInteractions`
- [`freeze`](actor-effect.md#freeze) — `lib.actoreffect.freeze(...)` ← `actorFreeze`
- [`play_animation`](actor-effect.md#play_animation) — `lib.actoreffect.play_animation(...)` ← `actorPlayAnimation`
- [`play_animation_full`](actor-effect.md#play_animation_full) — `lib.actoreffect.play_animation_full(...)` ← `actorPlayAnimationFull`
- [`play_death`](actor-effect.md#play_death) — `lib.actoreffect.play_death(...)` ← `actorPlayDeath`
- [`play_event`](actor-effect.md#play_event) — `lib.actoreffect.play_event(...)` ← `actorPlayEvent`
- [`play_event_for`](actor-effect.md#play_event_for) — `lib.actoreffect.play_event_for(...)` ← `actorPlayEventFor`
- [`play_hurt`](actor-effect.md#play_hurt) — `lib.actoreffect.play_hurt(...)` ← `actorPlayHurt`
- [`play_instant_death`](actor-effect.md#play_instant_death) — `lib.actoreffect.play_instant_death(...)` ← `actorPlayInstantDeath`
- [`play_totem`](actor-effect.md#play_totem) — `lib.actoreffect.play_totem(...)` ← `actorPlayTotem`
- [`poll_interactions`](actor-effect.md#poll_interactions) — `lib.actoreffect.poll_interactions(...)` ← `actorPollInteractions`
- [`runtime_id_of_entity`](actor-effect.md#runtime_id_of_entity) — `lib.actoreffect.runtime_id_of_entity(...)` ← `actorRuntimeIdOfEntity`
- [`runtime_id_of_player`](actor-effect.md#runtime_id_of_player) — `lib.actoreffect.runtime_id_of_player(...)` ← `actorRuntimeIdOfPlayer`
- [`set_pose`](actor-effect.md#set_pose) — `lib.actoreffect.set_pose(...)` ← `actorSetPose`
- [`unblock_event`](actor-effect.md#unblock_event) — `lib.actoreffect.unblock_event(...)` ← `actorUnblockEvent`
- [`unblock_event_for`](actor-effect.md#unblock_event_for) — `lib.actoreffect.unblock_event_for(...)` ← `actorUnblockEventFor`
- [`unblock_event_for_entity`](actor-effect.md#unblock_event_for_entity) — `lib.actoreffect.unblock_event_for_entity(...)` ← `actorUnblockEventForEntity`
- [`unfreeze`](actor-effect.md#unfreeze) — `lib.actoreffect.unfreeze(...)` ← `actorUnfreeze`
- [`creeper_display`](actor-effect.md#creeper_display) — `lib.actoreffect.creeper_display(...)` ← `easterEggCreeper`
- [`easter_egg`](actor-effect.md#easter_egg) — `lib.itemdisplay.easter_egg(...)` ← `easterEggCreeperDisplay`

## 假方块（5）

页面：[假方块](fake-block.md)

- [`clear`](fake-block.md#clear) — `lib.fakeblock.clear(...)` ← `blockFakeClear`
- [`clear_all`](fake-block.md#clear_all) — `lib.fakeblock.clear_all(...)` ← `blockFakeClearAll`
- [`resolve`](fake-block.md#resolve) — `lib.fakeblock.resolve(...)` ← `blockFakeResolve`
- [`set`](fake-block.md#set) — `lib.fakeblock.set(...)` ← `blockFakeSet`
- [`set_all`](fake-block.md#set_all) — `lib.fakeblock.set_all(...)` ← `blockFakeSetAll`

## 表单（2）

页面：[表单](form.md)

- [`close`](form.md#close) — `lib.forms.close(...)` ← `formClose`
- [`close_all`](form.md#close_all) — `lib.forms.close_all(...)` ← `formCloseAll`

## 物品详情（2）

页面：[物品详情](item-detail.md)

- [`hide`](item-detail.md#hide) — `lib.itemdetail.hide(...)` ← `itemDetailHide`
- [`show`](item-detail.md#show) — `lib.itemdetail.show(...)` ← `itemDetailShow`

## 复活（3）

页面：[复活](respawn.md)

- [`respawn`](respawn.md#respawn) — `lib.playerrespawn.respawn(...)` ← `playerRespawn`
- [`respawn_at`](respawn.md#respawn_at) — `lib.playerrespawn.respawn_at(...)` ← `playerRespawnAt`
- [`respawn_state`](respawn.md#respawn_state) — `lib.playerrespawn.respawn_state(...)` ← `playerRespawnState`

## 掉落物（9）

页面：[掉落物](item-actor.md)

- [`destroy`](item-actor.md#destroy) — `lib.itemactor.destroy(...)` ← `itemActorDestroy`
- [`destroy_all`](item-actor.md#destroy_all) — `lib.itemactor.destroy_all(...)` ← `itemActorDestroyAll`
- [`exists`](item-actor.md#exists) — `lib.itemactor.exists(...)` ← `itemActorExists`
- [`get_ids`](item-actor.md#get_ids) — `lib.itemactor.get_ids(...)` ← `itemActorGetIds`
- [`move_to`](item-actor.md#move_to) — `lib.itemactor.move_to(...)` ← `itemActorMoveTo`
- [`runtime_id`](item-actor.md#runtime_id) — `lib.itemactor.runtime_id(...)` ← `itemActorRuntimeId`
- [`set_motion`](item-actor.md#set_motion) — `lib.itemactor.set_motion(...)` ← `itemActorSetMotion`
- [`spawn`](item-actor.md#spawn) — `lib.itemactor.spawn(...)` ← `itemActorSpawn`
- [`take`](item-actor.md#take) — `lib.itemactor.take(...)` ← `itemActorTake`

## 物品展示（45）

页面：[物品展示](item-display.md)

- [`clear_anim`](item-display.md#clear_anim) — `lib.itemdisplay.clear_anim(...)` ← `itemDisplayClearAnim`
- [`clear_visible_players`](item-display.md#clear_visible_players) — `lib.itemdisplay.clear_visible_players(...)` ← `itemDisplayClearVisiblePlayers`
- [`create`](item-display.md#create) — `lib.itemdisplay.create(...)` ← `itemDisplayCreate`
- [`create_advanced`](item-display.md#create_advanced) — `lib.itemdisplay.create_advanced(...)` ← `itemDisplayCreateAdvanced`
- [`create_full`](item-display.md#create_full) — `lib.itemdisplay.create_full(...)` ← `itemDisplayCreateFull`
- [`create_random`](item-display.md#create_random) — `lib.itemdisplay.create_random(...)` ← `itemDisplayCreateRandom`
- [`create_seamless`](item-display.md#create_seamless) — `lib.itemdisplay.create_seamless(...)` ← `itemDisplayCreateSeamless`
- [`create_with_id`](item-display.md#create_with_id) — `lib.itemdisplay.create_with_id(...)` ← `itemDisplayCreateWithId`
- [`destroy`](item-display.md#destroy) — `lib.itemdisplay.destroy(...)` ← `itemDisplayDestroy`
- [`destroy_all`](item-display.md#destroy_all) — `lib.itemdisplay.destroy_all(...)` ← `itemDisplayDestroyAll`
- [`emit_for_entity`](item-display.md#emit_for_entity) — `lib.itemdisplay.emit_for_entity(...)` ← `itemDisplayEmitForEntity`
- [`exists`](item-display.md#exists) — `lib.itemdisplay.exists(...)` ← `itemDisplayExists`
- [`find_nearest`](item-display.md#find_nearest) — `lib.itemdisplay.find_nearest(...)` ← `itemDisplayFindNearest`
- [`follow`](item-display.md#follow) — `lib.itemdisplay.follow(...)` ← `itemDisplayFollow`
- [`get_all_ids`](item-display.md#get_all_ids) — `lib.itemdisplay.get_all_ids(...)` ← `itemDisplayGetAllIds`
- [`get_info`](item-display.md#get_info) — `lib.itemdisplay.get_info(...)` ← `itemDisplayGetInfo`
- [`is_id_used`](item-display.md#is_id_used) — `lib.itemdisplay.is_id_used(...)` ← `itemDisplayIsIdUsed`
- [`rotate_y`](item-display.md#rotate_y) — `lib.itemdisplay.rotate_y(...)` ← `itemDisplayRotateY`
- [`runtime_id`](item-display.md#runtime_id) — `lib.itemdisplay.runtime_id(...)` ← `itemDisplayRuntimeId`
- [`scale_by`](item-display.md#scale_by) — `lib.itemdisplay.scale_by(...)` ← `itemDisplayScaleBy`
- [`scale_to`](item-display.md#scale_to) — `lib.itemdisplay.scale_to(...)` ← `itemDisplayScaleTo`
- [`set_anim`](item-display.md#set_anim) — `lib.itemdisplay.set_anim(...)` ← `itemDisplaySetAnim`
- [`set_base_offset`](item-display.md#set_base_offset) — `lib.itemdisplay.set_base_offset(...)` ← `itemDisplaySetBaseOffset`
- [`set_enabled`](item-display.md#set_enabled) — `lib.itemdisplay.set_enabled(...)` ← `itemDisplaySetEnabled`
- [`set_extend`](item-display.md#set_extend) — `lib.itemdisplay.set_extend(...)` ← `itemDisplaySetExtend`
- [`set_glint`](item-display.md#set_glint) — `lib.itemdisplay.set_glint(...)` ← `itemDisplaySetGlint`
- [`set_hitbox`](item-display.md#set_hitbox) — `lib.itemdisplay.set_hitbox(...)` ← `itemDisplaySetHitbox`
- [`set_item`](item-display.md#set_item) — `lib.itemdisplay.set_item(...)` ← `itemDisplaySetItem`
- [`set_item_with_nbt`](item-display.md#set_item_with_nbt) — `lib.itemdisplay.set_item_with_nbt(...)` ← `itemDisplaySetItemWithNbt`
- [`set_mode`](item-display.md#set_mode) — `lib.itemdisplay.set_mode(...)` ← `itemDisplaySetMode`
- [`set_offset`](item-display.md#set_offset) — `lib.itemdisplay.set_offset(...)` ← `itemDisplaySetOffset`
- [`set_offset_live`](item-display.md#set_offset_live) — `lib.itemdisplay.set_offset_live(...)` ← `itemDisplaySetOffsetLive`
- [`set_position`](item-display.md#set_position) — `lib.itemdisplay.set_position(...)` ← `itemDisplaySetPosition`
- [`set_preset`](item-display.md#set_preset) — `lib.itemdisplay.set_preset(...)` ← `itemDisplaySetPreset`
- [`set_rotation`](item-display.md#set_rotation) — `lib.itemdisplay.set_rotation(...)` ← `itemDisplaySetRotation`
- [`set_rotation_live`](item-display.md#set_rotation_live) — `lib.itemdisplay.set_rotation_live(...)` ← `itemDisplaySetRotationLive`
- [`set_scale`](item-display.md#set_scale) — `lib.itemdisplay.set_scale(...)` ← `itemDisplaySetScale`
- [`set_scale_live`](item-display.md#set_scale_live) — `lib.itemdisplay.set_scale_live(...)` ← `itemDisplaySetScaleLive`
- [`set_stretch`](item-display.md#set_stretch) — `lib.itemdisplay.set_stretch(...)` ← `itemDisplaySetStretch`
- [`set_stretch_live`](item-display.md#set_stretch_live) — `lib.itemdisplay.set_stretch_live(...)` ← `itemDisplaySetStretchLive`
- [`set_transform_live`](item-display.md#set_transform_live) — `lib.itemdisplay.set_transform_live(...)` ← `itemDisplaySetTransformLive`
- [`set_view_distance`](item-display.md#set_view_distance) — `lib.itemdisplay.set_view_distance(...)` ← `itemDisplaySetViewDistance`
- [`set_visible_player`](item-display.md#set_visible_player) — `lib.itemdisplay.set_visible_player(...)` ← `itemDisplaySetVisiblePlayer`
- [`set_visible_players`](item-display.md#set_visible_players) — `lib.itemdisplay.set_visible_players(...)` ← `itemDisplaySetVisiblePlayers`
- [`unfollow`](item-display.md#unfollow) — `lib.itemdisplay.unfollow(...)` ← `itemDisplayUnfollow`

## 虚拟容器（16）

页面：[虚拟容器](containers.md)

- [`block_event`](containers.md#block_event) — `lib.container.block_event(...)` ← `containerBlockEvent`
- [`clear_clicks`](containers.md#clear_clicks) — `lib.container.clear_clicks(...)` ← `containerClearClicks`
- [`close`](containers.md#close) — `lib.container.close(...)` ← `containerClose`
- [`close_all`](containers.md#close_all) — `lib.container.close_all(...)` ← `containerCloseAll`
- [`dropped_clicks`](containers.md#dropped_clicks) — `lib.container.dropped_clicks(...)` ← `containerDroppedClicks`
- [`get_ids`](containers.md#get_ids) — `lib.container.get_ids(...)` ← `containerGetIds`
- [`is_open`](containers.md#is_open) — `lib.container.is_open(...)` ← `containerIsOpen`
- [`open`](containers.md#open) — `lib.container.open(...)` ← `containerOpen`
- [`poll_clicks`](containers.md#poll_clicks) — `lib.container.poll_clicks(...)` ← `containerPollClicks`
- [`preview`](containers.md#preview) — `lib.container.preview(...)` ← `containerPreview`
- [`preview_titled`](containers.md#preview_titled) — `lib.container.preview_titled(...)` ← `containerPreviewTitled`
- [`set_interactive`](containers.md#set_interactive) — `lib.container.set_interactive(...)` ← `containerSetInteractive`
- [`set_item`](containers.md#set_item) — `lib.container.set_item(...)` ← `containerSetItem`
- [`set_item_enchanted`](containers.md#set_item_enchanted) — `lib.container.set_item_enchanted(...)` ← `containerSetItemEnchanted`
- [`set_title`](containers.md#set_title) — `lib.container.set_title(...)` ← `containerSetTitle`
- [`update`](containers.md#update) — `lib.container.update(...)` ← `containerUpdate`

## 假背包（10）

页面：[假背包](fake-inventory.md)

- [`apply`](fake-inventory.md#apply) — `lib.fakeinv.apply(...)` ← `fakeInvApply`
- [`clear`](fake-inventory.md#clear) — `lib.fakeinv.clear(...)` ← `fakeInvClear`
- [`clear_all`](fake-inventory.md#clear_all) — `lib.fakeinv.clear_all(...)` ← `fakeInvClearAll`
- [`clear_clicks`](fake-inventory.md#clear_clicks) — `lib.fakeinv.clear_clicks(...)` ← `fakeInvClearClicks`
- [`dropped_clicks`](fake-inventory.md#dropped_clicks) — `lib.fakeinv.dropped_clicks(...)` ← `fakeInvDroppedClicks`
- [`get_players`](fake-inventory.md#get_players) — `lib.fakeinv.get_players(...)` ← `fakeInvGetPlayers`
- [`is_active`](fake-inventory.md#is_active) — `lib.fakeinv.is_active(...)` ← `fakeInvIsActive`
- [`poll_clicks`](fake-inventory.md#poll_clicks) — `lib.fakeinv.poll_clicks(...)` ← `fakeInvPollClicks`
- [`refresh`](fake-inventory.md#refresh) — `lib.fakeinv.refresh(...)` ← `fakeInvRefresh`
- [`set_slot`](fake-inventory.md#set_slot) — `lib.fakeinv.set_slot(...)` ← `fakeInvSetSlot`

## 自定义实体（47）

页面：[自定义实体](entities.md)

- [`clear_mob_properties`](entities.md#clear_mob_properties) — `lib.entities.clear_mob_properties(...)` ← `entityClearMobProperties`
- [`clear_player_appearance`](entities.md#clear_player_appearance) — `lib.entities.clear_player_appearance(...)` ← `entityClearPlayerAppearance`
- [`clear_player_rotation`](entities.md#clear_player_rotation) — `lib.entities.clear_player_rotation(...)` ← `entityClearPlayerRotation`
- [`clear_player_rotations`](entities.md#clear_player_rotations) — `lib.entities.clear_player_rotations(...)` ← `entityClearPlayerRotations`
- [`clear_ride`](entities.md#clear_ride) — `lib.entities.clear_ride(...)` ← `entityClearRide`
- [`clear_spawns`](entities.md#clear_spawns) — `lib.entities.clear_spawns(...)` ← `entityClearSpawns`
- [`clear_visible_players`](entities.md#clear_visible_players) — `lib.entities.clear_visible_players(...)` ← `entityClearVisiblePlayers`
- [`create`](entities.md#create) — `lib.entities.create(...)` ← `entityCreate`
- [`create_advanced`](entities.md#create_advanced) — `lib.entities.create_advanced(...)` ← `entityCreateAdvanced`
- [`create_random`](entities.md#create_random) — `lib.entities.create_random(...)` ← `entityCreateRandom`
- [`create_with_id`](entities.md#create_with_id) — `lib.entities.create_with_id(...)` ← `entityCreateWithId`
- [`destroy`](entities.md#destroy) — `lib.entities.destroy(...)` ← `entityDestroy`
- [`destroy_all`](entities.md#destroy_all) — `lib.entities.destroy_all(...)` ← `entityDestroyAll`
- [`exists`](entities.md#exists) — `lib.entities.exists(...)` ← `entityExists`
- [`find_nearest`](entities.md#find_nearest) — `lib.entities.find_nearest(...)` ← `entityFindNearest`
- [`get_all_ids`](entities.md#get_all_ids) — `lib.entities.get_all_ids(...)` ← `entityGetAllIds`
- [`get_info`](entities.md#get_info) — `lib.entities.get_info(...)` ← `entityGetInfo`
- [`is_id_used`](entities.md#is_id_used) — `lib.entities.is_id_used(...)` ← `entityIsIdUsed`
- [`play_animation`](entities.md#play_animation) — `lib.entities.play_animation(...)` ← `entityPlayAnimation`
- [`play_animation_to`](entities.md#play_animation_to) — `lib.entities.play_animation_to(...)` ← `entityPlayAnimationTo`
- [`poll_spawns`](entities.md#poll_spawns) — `lib.entities.poll_spawns(...)` ← `entityPollSpawns`
- [`scale_by`](entities.md#scale_by) — `lib.entities.scale_by(...)` ← `entityScaleBy`
- [`set_color_index`](entities.md#set_color_index) — `lib.entities.set_color_index(...)` ← `entitySetColorIndex`
- [`set_enabled`](entities.md#set_enabled) — `lib.entities.set_enabled(...)` ← `entitySetEnabled`
- [`set_equipment_slot`](entities.md#set_equipment_slot) — `lib.entities.set_equipment_slot(...)` ← `entitySetEquipmentSlot`
- [`set_flags`](entities.md#set_flags) — `lib.entities.set_flags(...)` ← `entitySetFlags`
- [`set_identifier`](entities.md#set_identifier) — `lib.entities.set_identifier(...)` ← `entitySetIdentifier`
- [`set_invisible`](entities.md#set_invisible) — `lib.entities.set_invisible(...)` ← `entitySetInvisible`
- [`set_mark_variant`](entities.md#set_mark_variant) — `lib.entities.set_mark_variant(...)` ← `entitySetMarkVariant`
- [`set_mob_property`](entities.md#set_mob_property) — `lib.entities.set_mob_property(...)` ← `entitySetMobProperty`
- [`set_nametag`](entities.md#set_nametag) — `lib.entities.set_nametag(...)` ← `entitySetNametag`
- [`set_player_equipment_slot`](entities.md#set_player_equipment_slot) — `lib.entities.set_player_equipment_slot(...)` ← `entitySetPlayerEquipmentSlot`
- [`set_player_nametag`](entities.md#set_player_nametag) — `lib.entities.set_player_nametag(...)` ← `entitySetPlayerNametag`
- [`set_player_rotation`](entities.md#set_player_rotation) — `lib.entities.set_player_rotation(...)` ← `entitySetPlayerRotation`
- [`set_player_scale`](entities.md#set_player_scale) — `lib.entities.set_player_scale(...)` ← `entitySetPlayerScale`
- [`set_pose`](entities.md#set_pose) — `lib.entities.set_pose(...)` ← `entitySetPose`
- [`set_position`](entities.md#set_position) — `lib.entities.set_position(...)` ← `entitySetPosition`
- [`set_ride_entity`](entities.md#set_ride_entity) — `lib.entities.set_ride_entity(...)` ← `entitySetRideEntity`
- [`set_ride_player`](entities.md#set_ride_player) — `lib.entities.set_ride_player(...)` ← `entitySetRidePlayer`
- [`set_rotation`](entities.md#set_rotation) — `lib.entities.set_rotation(...)` ← `entitySetRotation`
- [`set_scale`](entities.md#set_scale) — `lib.entities.set_scale(...)` ← `entitySetScale`
- [`set_variant`](entities.md#set_variant) — `lib.entities.set_variant(...)` ← `entitySetVariant`
- [`set_view_distance`](entities.md#set_view_distance) — `lib.entities.set_view_distance(...)` ← `entitySetViewDistance`
- [`set_visible_player`](entities.md#set_visible_player) — `lib.entities.set_visible_player(...)` ← `entitySetVisiblePlayer`
- [`set_visible_players`](entities.md#set_visible_players) — `lib.entities.set_visible_players(...)` ← `entitySetVisiblePlayers`
- [`stop_animation`](entities.md#stop_animation) — `lib.entities.stop_animation(...)` ← `entityStopAnimation`
- [`stop_animation_to`](entities.md#stop_animation_to) — `lib.entities.stop_animation_to(...)` ← `entityStopAnimationTo`

## 交易菜单（12）

页面：[交易菜单](trade.md)

- [`add_offer`](trade.md#add_offer) — `lib.trade.add_offer(...)` ← `tradeAddOffer`
- [`close`](trade.md#close) — `lib.trade.close(...)` ← `tradeClose`
- [`close_all`](trade.md#close_all) — `lib.trade.close_all(...)` ← `tradeCloseAll`
- [`dropped_settlements`](trade.md#dropped_settlements) — `lib.trade.dropped_settlements(...)` ← `tradeDroppedSettlements`
- [`get_ids`](trade.md#get_ids) — `lib.trade.get_ids(...)` ← `tradeGetIds`
- [`is_open`](trade.md#is_open) — `lib.trade.is_open(...)` ← `tradeIsOpen`
- [`is_settle`](trade.md#is_settle) — `lib.trade.is_settle(...)` ← `tradeIsSettle`
- [`open`](trade.md#open) — `lib.trade.open(...)` ← `tradeOpen`
- [`poll_settlements`](trade.md#poll_settlements) — `lib.trade.poll_settlements(...)` ← `tradePollSettlements`
- [`set_settle`](trade.md#set_settle) — `lib.trade.set_settle(...)` ← `tradeSetSettle`
- [`set_tier`](trade.md#set_tier) — `lib.trade.set_tier(...)` ← `tradeSetTier`
- [`update`](trade.md#update) — `lib.trade.update(...)` ← `tradeUpdate`

## NPC 对话（10）

页面：[NPC 对话](npc-dialog.md)

- [`clear_clicks`](npc-dialog.md#clear_clicks) — `lib.npcdialog.clear_clicks(...)` ← `npcDialogClearClicks`
- [`close`](npc-dialog.md#close) — `lib.npcdialog.close(...)` ← `npcDialogClose`
- [`close_all`](npc-dialog.md#close_all) — `lib.npcdialog.close_all(...)` ← `npcDialogCloseAll`
- [`dropped_clicks`](npc-dialog.md#dropped_clicks) — `lib.npcdialog.dropped_clicks(...)` ← `npcDialogDroppedClicks`
- [`get_ids`](npc-dialog.md#get_ids) — `lib.npcdialog.get_ids(...)` ← `npcDialogGetIds`
- [`is_open`](npc-dialog.md#is_open) — `lib.npcdialog.is_open(...)` ← `npcDialogIsOpen`
- [`open`](npc-dialog.md#open) — `lib.npcdialog.open(...)` ← `npcDialogOpen`
- [`poll_clicks`](npc-dialog.md#poll_clicks) — `lib.npcdialog.poll_clicks(...)` ← `npcDialogPollClicks`
- [`set_avatar`](npc-dialog.md#set_avatar) — `lib.npcdialog.set_avatar(...)` ← `npcDialogSetAvatar`
- [`update`](npc-dialog.md#update) — `lib.npcdialog.update(...)` ← `npcDialogUpdate`

## 硫磺方块（10）

页面：[硫磺方块](sulfur.md)

- [`create`](sulfur.md#create) — `lib.sulfur.create(...)` ← `sulfurCreate`
- [`destroy`](sulfur.md#destroy) — `lib.sulfur.destroy(...)` ← `sulfurDestroy`
- [`destroy_all`](sulfur.md#destroy_all) — `lib.sulfur.destroy_all(...)` ← `sulfurDestroyAll`
- [`exists`](sulfur.md#exists) — `lib.sulfur.exists(...)` ← `sulfurExists`
- [`get_entity_id`](sulfur.md#get_entity_id) — `lib.sulfur.get_entity_id(...)` ← `sulfurGetEntityId`
- [`get_ids`](sulfur.md#get_ids) — `lib.sulfur.get_ids(...)` ← `sulfurGetIds`
- [`set_archetype`](sulfur.md#set_archetype) — `lib.sulfur.set_archetype(...)` ← `sulfurSetArchetype`
- [`set_block`](sulfur.md#set_block) — `lib.sulfur.set_block(...)` ← `sulfurSetBlock`
- [`set_invisible`](sulfur.md#set_invisible) — `lib.sulfur.set_invisible(...)` ← `sulfurSetInvisible`
- [`set_scale`](sulfur.md#set_scale) — `lib.sulfur.set_scale(...)` ← `sulfurSetScale`

## 玩家 NPC（55）

页面：[玩家 NPC](player-npc.md)

- [`capture_skin`](player-npc.md#capture_skin) — `lib.playernpc.capture_skin(...)` ← `playerNpcCaptureSkin`
- [`clear_player_rotation`](player-npc.md#clear_player_rotation) — `lib.playernpc.clear_player_rotation(...)` ← `playerNpcClearPlayerRotation`
- [`clear_player_rotations`](player-npc.md#clear_player_rotations) — `lib.playernpc.clear_player_rotations(...)` ← `playerNpcClearPlayerRotations`
- [`clear_ride`](player-npc.md#clear_ride) — `lib.playernpc.clear_ride(...)` ← `playerNpcClearRide`
- [`clear_spawns`](player-npc.md#clear_spawns) — `lib.playernpc.clear_spawns(...)` ← `playerNpcClearSpawns`
- [`clear_visible_players`](player-npc.md#clear_visible_players) — `lib.playernpc.clear_visible_players(...)` ← `playerNpcClearVisiblePlayers`
- [`create`](player-npc.md#create) — `lib.playernpc.create(...)` ← `playerNpcCreate`
- [`create_ex`](player-npc.md#create_ex) — `lib.playernpc.create_ex(...)` ← `playerNpcCreateEx`
- [`create_random`](player-npc.md#create_random) — `lib.playernpc.create_random(...)` ← `playerNpcCreateRandom`
- [`create_with_id`](player-npc.md#create_with_id) — `lib.playernpc.create_with_id(...)` ← `playerNpcCreateWithId`
- [`destroy`](player-npc.md#destroy) — `lib.playernpc.destroy(...)` ← `playerNpcDestroy`
- [`destroy_all`](player-npc.md#destroy_all) — `lib.playernpc.destroy_all(...)` ← `playerNpcDestroyAll`
- [`exists`](player-npc.md#exists) — `lib.playernpc.exists(...)` ← `playerNpcExists`
- [`get_all_ids`](player-npc.md#get_all_ids) — `lib.playernpc.get_all_ids(...)` ← `playerNpcGetAllIds`
- [`get_debug_info`](player-npc.md#get_debug_info) — `lib.playernpc.get_debug_info(...)` ← `playerNpcGetDebugInfo`
- [`get_skin_blob_b64`](player-npc.md#get_skin_blob_b64) — `lib.playernpc.get_skin_blob_b64(...)` ← `playerNpcGetSkinBlobB64`
- [`get_skin_ids`](player-npc.md#get_skin_ids) — `lib.playernpc.get_skin_ids(...)` ← `playerNpcGetSkinIds`
- [`has_skin`](player-npc.md#has_skin) — `lib.playernpc.has_skin(...)` ← `playerNpcHasSkin`
- [`import_skins`](player-npc.md#import_skins) — `lib.playernpc.import_skins(...)` ← `playerNpcImportSkins`
- [`inject_skin`](player-npc.md#inject_skin) — `lib.playernpc.inject_skin(...)` ← `playerNpcInjectSkin`
- [`inject_skin_all`](player-npc.md#inject_skin_all) — `lib.playernpc.inject_skin_all(...)` ← `playerNpcInjectSkinAll`
- [`is_id_used`](player-npc.md#is_id_used) — `lib.playernpc.is_id_used(...)` ← `playerNpcIsIdUsed`
- [`play_animation`](player-npc.md#play_animation) — `lib.playernpc.play_animation(...)` ← `playerNpcPlayAnimation`
- [`play_animation_to`](player-npc.md#play_animation_to) — `lib.playernpc.play_animation_to(...)` ← `playerNpcPlayAnimationTo`
- [`play_emote`](player-npc.md#play_emote) — `lib.playernpc.play_emote(...)` ← `playerNpcPlayEmote`
- [`poll_spawns`](player-npc.md#poll_spawns) — `lib.playernpc.poll_spawns(...)` ← `playerNpcPollSpawns`
- [`register_image_skin`](player-npc.md#register_image_skin) — `lib.playernpc.register_image_skin(...)` ← `playerNpcRegisterImageSkin`
- [`register_image_skin_sized`](player-npc.md#register_image_skin_sized) — `lib.playernpc.register_image_skin_sized(...)` ← `playerNpcRegisterImageSkinSized`
- [`register_skin`](player-npc.md#register_skin) — `lib.playernpc.register_skin(...)` ← `playerNpcRegisterSkin`
- [`register_skin_data`](player-npc.md#register_skin_data) — `lib.playernpc.register_skin_data(...)` ← `playerNpcRegisterSkinData`
- [`register_skin_data_opaque`](player-npc.md#register_skin_data_opaque) — `lib.playernpc.register_skin_data_opaque(...)` ← `playerNpcRegisterSkinDataOpaque`
- [`register_skin_from_blob_b64`](player-npc.md#register_skin_from_blob_b64) — `lib.playernpc.register_skin_from_blob_b64(...)` ← `playerNpcRegisterSkinFromBlobB64`
- [`register_skin_from_blob_file`](player-npc.md#register_skin_from_blob_file) — `lib.playernpc.register_skin_from_blob_file(...)` ← `playerNpcRegisterSkinFromBlobFile`
- [`runtime_id`](player-npc.md#runtime_id) — `lib.playernpc.runtime_id(...)` ← `playerNpcRuntimeId`
- [`set_enabled`](player-npc.md#set_enabled) — `lib.playernpc.set_enabled(...)` ← `playerNpcSetEnabled`
- [`set_equipment`](player-npc.md#set_equipment) — `lib.playernpc.set_equipment(...)` ← `playerNpcSetEquipment`
- [`set_image_size`](player-npc.md#set_image_size) — `lib.playernpc.set_image_size(...)` ← `playerNpcSetImageSize`
- [`set_nametag`](player-npc.md#set_nametag) — `lib.playernpc.set_nametag(...)` ← `playerNpcSetNametag`
- [`set_player_rotation`](player-npc.md#set_player_rotation) — `lib.playernpc.set_player_rotation(...)` ← `playerNpcSetPlayerRotation`
- [`set_pos`](player-npc.md#set_pos) — `lib.playernpc.set_pos(...)` ← `playerNpcSetPos`
- [`set_pos_light`](player-npc.md#set_pos_light) — `lib.playernpc.set_pos_light(...)` ← `playerNpcSetPosLight`
- [`set_pose`](player-npc.md#set_pose) — `lib.playernpc.set_pose(...)` ← `playerNpcSetPose`
- [`set_ride_entity`](player-npc.md#set_ride_entity) — `lib.playernpc.set_ride_entity(...)` ← `playerNpcSetRideEntity`
- [`set_ride_player`](player-npc.md#set_ride_player) — `lib.playernpc.set_ride_player(...)` ← `playerNpcSetRidePlayer`
- [`set_ride_unique_id`](player-npc.md#set_ride_unique_id) — `lib.playernpc.set_ride_unique_id(...)` ← `playerNpcSetRideUniqueId`
- [`set_rot_light`](player-npc.md#set_rot_light) — `lib.playernpc.set_rot_light(...)` ← `playerNpcSetRotLight`
- [`set_rotation`](player-npc.md#set_rotation) — `lib.playernpc.set_rotation(...)` ← `playerNpcSetRotation`
- [`set_scale`](player-npc.md#set_scale) — `lib.playernpc.set_scale(...)` ← `playerNpcSetScale`
- [`set_skin`](player-npc.md#set_skin) — `lib.playernpc.set_skin(...)` ← `playerNpcSetSkin`
- [`set_view_distance`](player-npc.md#set_view_distance) — `lib.playernpc.set_view_distance(...)` ← `playerNpcSetViewDistance`
- [`set_visible_player`](player-npc.md#set_visible_player) — `lib.playernpc.set_visible_player(...)` ← `playerNpcSetVisiblePlayer`
- [`set_visible_players`](player-npc.md#set_visible_players) — `lib.playernpc.set_visible_players(...)` ← `playerNpcSetVisiblePlayers`
- [`stop_animation`](player-npc.md#stop_animation) — `lib.playernpc.stop_animation(...)` ← `playerNpcStopAnimation`
- [`stop_animation_to`](player-npc.md#stop_animation_to) — `lib.playernpc.stop_animation_to(...)` ← `playerNpcStopAnimationTo`
- [`unregister_skin`](player-npc.md#unregister_skin) — `lib.playernpc.unregister_skin(...)` ← `playerNpcUnregisterSkin`

## 视角覆盖（21）

页面：[视角覆盖](view.md)

- `view_align` ← `viewAlign`（未映射）
- [`view_block`](view.md#view_block) — `lib.view.view_block(...)` ← `viewBlock`
- [`view_clear_all`](view.md#view_clear_all) — `lib.view.view_clear_all(...)` ← `viewClearAll`
- [`view_clear_block`](view.md#view_clear_block) — `lib.view.view_clear_block(...)` ← `viewClearBlock`
- [`view_clear_entity`](view.md#view_clear_entity) — `lib.view.view_clear_entity(...)` ← `viewClearEntity`
- [`view_clear_id`](view.md#view_clear_id) — `lib.view.view_clear_id(...)` ← `viewClearId`
- [`view_describe`](view.md#view_describe) — `lib.view.view_describe(...)` ← `viewDescribe`
- [`view_echo`](view.md#view_echo) — `lib.view.view_echo(...)` ← `viewEcho`
- [`view_entity`](view.md#view_entity) — `lib.view.view_entity(...)` ← `viewEntity`
- `view_entity_info` ← `viewEntityInfo`（未映射）
- [`view_item_spawn_hooked`](view.md#view_item_spawn_hooked) — `lib.view.view_item_spawn_hooked(...)` ← `viewItemSpawnHooked`
- [`view_meta_probe`](view.md#view_meta_probe) — `lib.view.view_meta_probe(...)` ← `viewMetaProbe`
- [`view_poll_spawn_events`](view.md#view_poll_spawn_events) — `lib.view.view_poll_spawn_events(...)` ← `viewPollSpawnEvents`
- [`view_self`](view.md#view_self) — `lib.view.view_self(...)` ← `viewSelf`
- [`view_self_clear`](view.md#view_self_clear) — `lib.view.view_self_clear(...)` ← `viewSelfClear`
- [`view_self_hide`](view.md#view_self_hide) — `lib.view.view_self_hide(...)` ← `viewSelfHide`
- [`view_self_name`](view.md#view_self_name) — `lib.view.view_self_name(...)` ← `viewSelfName`
- [`view_self_skin`](view.md#view_self_skin) — `lib.view.view_self_skin(...)` ← `viewSelfSkin`
- [`view_self_type`](view.md#view_self_type) — `lib.view.view_self_type(...)` ← `viewSelfType`
- [`view_target_type`](view.md#view_target_type) — `lib.view.view_target_type(...)` ← `viewTargetType`
- [`view_unique_id_of`](view.md#view_unique_id_of) — `lib.view.view_unique_id_of(...)` ← `viewUniqueIdOf`

## 客户端感知（6）

页面：[客户端感知](sensing.md)

- [`device_code`](sensing.md#device_code) — `lib.sensing.device_code(...)` ← `sensingDeviceCode`
- [`device_of`](sensing.md#device_of) — `lib.sensing.device_of(...)` ← `sensingDeviceOf`
- [`is_touch`](sensing.md#is_touch) — `lib.sensing.is_touch(...)` ← `sensingIsTouch`
- [`count`](sensing.md#count) — `lib.sensing.count(...)` ← `swingCount`
- [`reset`](sensing.md#reset) — `lib.sensing.reset(...)` ← `swingReset`
- [`total`](sensing.md#total) — `lib.sensing.total(...)` ← `swingTotal`

## 可见性（5）

页面：[可见性](visibility.md)

- [`get_mask`](visibility.md#get_mask) — `lib.visibility.get_mask(...)` ← `playerHideGetMask`
- [`is_hidden`](visibility.md#is_hidden) — `lib.visibility.is_hidden(...)` ← `playerHideIsHidden`
- [`restore`](visibility.md#restore) — `lib.visibility.restore(...)` ← `playerHideRestore`
- [`set_hidden`](visibility.md#set_hidden) — `lib.visibility.set_hidden(...)` ← `playerHideSetHidden`
- [`set_mask`](visibility.md#set_mask) — `lib.visibility.set_mask(...)` ← `playerHideSetMask`

## 幽灵交互（2）

页面：[幽灵交互](ghost.md)

- [`clear_interactions`](ghost.md#clear_interactions) — `lib.ghost.clear_interactions(...)` ← `ghostClearInteractions`
- [`poll_interactions`](ghost.md#poll_interactions) — `lib.ghost.poll_interactions(...)` ← `ghostPollInteractions`

## 线路诊断（9）

页面：[线路诊断](wire.md)

- [`clear`](wire.md#clear) — `lib.wire.clear(...)` ← `wireResetStats`
- [`stats`](wire.md#stats) — `lib.wire.stats(...)` ← `wireStats`
- [`is_active`](wire.md#is_active) — `lib.wire.is_active(...)` ← `wireTraceActive`
- [`clear`](wire.md#clear) — `lib.wire.clear(...)` ← `wireTraceClear`
- [`count`](wire.md#count) — `lib.wire.count(...)` ← `wireTraceCount`
- [`dump`](wire.md#dump) — `lib.wire.dump(...)` ← `wireTraceDump`
- [`mark`](wire.md#mark) — `lib.wire.mark(...)` ← `wireTraceMark`
- [`start`](wire.md#start) — `lib.wire.start(...)` ← `wireTraceStart`
- [`stop`](wire.md#stop) — `lib.wire.stop(...)` ← `wireTraceStop`

## 杂项（9）

页面：[杂项](misc.md)

- [`acquire`](misc.md#acquire) — `lib.anchor.acquire(...)` ← `anchorAcquire`
- [`release`](misc.md#release) — `lib.anchor.release(...)` ← `anchorRelease`
- `block_batch_clear` ← `blockBatchClear`（未映射）
- `block_batch_progress` ← `blockBatchProgress`（未映射）
- `block_batch_self_test` ← `blockBatchSelfTest`（未映射）
- `block_batch_set` ← `blockBatchSet`（未映射）
- `block_batch_set_all` ← `blockBatchSetAll`（未映射）
- `block_batch_set_near` ← `blockBatchSetNear`（未映射）
- `block_batch_stats` ← `blockBatchStats`（未映射）
