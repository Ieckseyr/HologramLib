"""HologramLib —— endstone 版。"""

from __future__ import annotations

from .actoreffect import ActorEffectApi
from .container import ContainerApi
from .bossbar import BossBarApi
from .entities import EntityApi
from .fakeblock import FakeBlockApi
from .fakeinv import FakeInventoryApi
from .anchor import AnchorApi
from .forms import FormApi
from .gradients import GradientApi
from .itemactor import ItemActorApi
from .itemdetail import ItemDetailApi
from .itemdisplay import ItemDisplayApi
from .playerrespawn import PlayerRespawnApi
from .ghost import GhostApi
from .hud import HudApi
from .holograms import HologramApi
from .native import NativeCore, NativeCoreError, ShapeDesc
from .particles import ParticleApi
from .playereffect import PlayerEffectApi
from .npcdialog import NpcDialogApi
from .sidebar import SidebarApi
from .playernpc import PlayerNpcApi
from .skins import SkinRegistry
from .sensing import SensingApi
from .sulfur import SulfurApi
from .trade import TradeApi
from .view import ViewApi
from .visibility import VisibilityApi
from .wire import WireApi
from .plugin import HologramLibPlugin
from .shape_api import ShapeApi
from .shapes import ShapeScene

# 版本号分三类，与 LSE 侧同口径，各自独立变化
__abi_version__ = "1.37.0"        # C++ 头文件与虚表布局
__lse_version__ = "1.0.0"         # 脚本侧导出面
__api_version__ = "1.0.0"         # endstone Python API

__all__ = [
    "HologramLibPlugin",
    "ShapeApi",
    "HologramApi",
    "GradientApi",
    "ParticleApi",
    "BossBarApi",
    "SidebarApi",
    "HudApi",
    "PlayerEffectApi",
    "FakeBlockApi",
    "ActorEffectApi",
    "LocatorBarApi",
    "PlayerRespawnApi",
    "ItemActorApi",
    "ItemDisplayApi",
    "ContainerApi",
    "FakeInventoryApi",
    "EntityApi",
    "TradeApi",
    "NpcDialogApi",
    "SulfurApi",
    "WireApi",
    "SkinRegistry",
    "GhostApi",
    "SensingApi",
    "VisibilityApi",
    "AnchorApi",
    "ViewApi",
    "PlayerNpcApi",
    "FormApi",
    "ItemDetailApi",
    "ShapeScene",
    "ShapeDesc",
    "NativeCore",
    "NativeCoreError",
]
