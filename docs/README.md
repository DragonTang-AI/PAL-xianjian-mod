# 仙剑奇侠传 Mod 版 · SDLPAL 增强改版

一个基于 **SDLPAL** 深度魔改的 **仙剑奇侠传** HTML5 / iOS 版本，在保留原汁原味经典体验的同时，加入了大量提升可玩性的现代功能。

> 🌐 **在线试玩：** _（部署后更新链接）_
>
> 📱 **iOS 版：** 基于 Xcode 构建部署

---

## 🙏 致谢

本项目建立在以下优秀开源项目的基础之上，特此致以诚挚感谢：

| 项目 | 说明 |
|------|------|
| **[SDLPAL](https://github.com/sdlpal/sdlpal)** | 核心跨平台重实现引擎，由 Wei Mingzhi 发起，SDLPAL 团队维护 |
| **[PAL Research Project](https://github.com/palxex/palresearch)** | 仙剑数据格式研究的基石 |
| **Baldur · louyihua** | 早期代码贡献者 |
| **PALxex** | 持续的支持与测试 |

### 使用的开源库

- **SDL** / **SDL_mixer** — 跨平台多媒体层
- **libmad**、**libogg** & **libvorbis**、**libopus** & **opusfile** — 音频解码
- **Adplug** — OPL 播放器
- **DOSBOX**、**MAME**、**Chocolate Doom** — OPL 模拟核心
- **FFmpeg** — AVI 播放器
- **stb** — 图像解码器

### 游戏版权

本程序 **不包含** 原版游戏的任何代码或数据文件。游戏数据（`.mkf` 文件）版权归 **大宇资讯（SoftStar Inc.）** 所有。请通过 [Steam](https://store.steampowered.com/app/1546570/Sword_and_Fairy/) 购买原版游戏获取数据文件。

---

## ✨ 我们与原生 SDLPAL 的不同

### 新增功能一览

#### 🎮 辅助菜单（13 项功能）
按 **Y 键** 打开，集成所有便利功能：

| # | 功能 | 说明 |
|---|------|------|
| 1 | 天降橫財 | 获得 9999 金钱 |
| 2 | 起死回生 | 全队 HP/MP 回满 |
| 3 | 逆天改命 | 全员 Lv99 + 满属性 |
| 4 | 無盡酒神 | 获得 99 个酒 |
| 5 | 全隊升一級 | 队伍全体升级 |
| 6 | 御劍飛行 | 穿墙模式开关 |
| 7 | AI自動戰鬥 | 智能战斗 AI 开关 |
| 8 | 無遇敵模式 | 不遇敌开关 |
| 9 | 地圖全亮 | 全地图探索开关 |
| 10 | 全物品獲取 | 获得全部道具（快照还原） |
| 11 | BOSS連戰 | 12 层连续 BOSS 挑战 |
| 12 | 圖鑑 | 怪物图鉴 + 物品图鉴 |
| 13 | 地圖傳送 | 景点传送 + 定点传送 |

#### ⚔️ 战斗系统增强

| 功能 | 说明 |
|------|------|
| **AI 智能战斗** | 模拟人类决策：回血 > 解状态 > 用道具 > 法术 > 普攻 > 防御 |
| **战斗 HUD** | 顶部敌人血量 / 底部玩家 HP/MP 常驻显示 |
| **伤害数字动画** | 攻击/回血数字飘起 + 淡出效果 |
| **自研物品资料库** | 150+ 件装备/道具的完整中文说明 |

#### 🗺️ 地图与传送

| 功能 | 说明 |
|------|------|
| **全新小地图** | 地形颜色区分（墙壁/地面/特殊）、玩家方向箭头 |
| **双级缩放** | 2:1 概览 / 1:1 细节，按 D 键切换 |
| **景点传送** | 18 个主要场景一键传送 |
| **定点传送** | 8 个自定义定位槽位，随时保存当前坐标 |

#### 📖 图鉴系统

| 功能 | 说明 |
|------|------|
| **怪物图鉴** | 自动记录遇过的敌人，显示 HP/攻/防/击杀数 |
| **物品图鉴** | 全物品列表 + 价格 + 持有数量 |
| **物品详情** | 背包中按 Y 查看物品说明、价格、类型标签 |

#### 💥 BOSS 連戰

| 特性 | 说明 |
|------|------|
| 逐层难度递增 | 每层 +15% HP/攻/防 |
| 通关奖励 | 金币 + 随机道具 |
| 休息站商店 | 第 4/8 层可用金币补给 |
| 战败重试 | 可选择重试或退出 |
| 连战统计 | 用时、层数、难度信息 |

---

## 🏗️ 架构

```
sdlpal-master/
├── *.c / *.h          # 核心引擎 + 所有 Mod 功能代码
├── emscripten/         # H5 网页版构建
│   ├── Makefile        # Emscripten 编译脚本
│   ├── sdlpal.html     # 网页入口
│   └── main.js         # JavaScript 胶水代码
├── ios/SDLPal/         # iOS 项目
│   └── SDLPal/Resources/*.mkf  # 游戏数据文件
├── 3rd/                # 第三方依赖（SDL 等）
├── libmad/ libogg/     # 音频解码库
└── adplug/ timidity/   # MIDI/音乐播放
```

### Mod 功能代码分布

| 文件 | 功能 |
|------|------|
| `uigame.c` | 辅助菜单、商城、BOSS連戰、图鉴、传送 |
| `uibattle.c` | 战斗 HUD、伤害数字动画 |
| `fight.c` | AI 智能战斗决策系统 |
| `play.c` | 小地图（缩放/地形/方向箭头） |
| `itemmenu.c` | 物品详情弹窗、物品说明数据库 |
| `battle.c` | 难度缩放、图鉴追踪（遇敌/击杀） |
| `video.c` | 画面风格滤镜（CRT/平滑/复古——预留） |

---

## 🚀 部署方式

### iOS 版

```bash
# 克隆项目
git clone https://github.com/DragonTang-AI/PAL-xianjian-mod.git

# 使用 Xcode 打开项目
open ios/SDLPal/SDLPal.xcworkspace

# 选择你的设备，Cmd+R 运行
```

**前提：** 需要 macOS + Xcode + Apple Developer 账号（免费也可）。

### H5 网页版

#### 1. 编译

```bash
# 安装 Emscripten SDK
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh

# 编译项目
cd emscripten
make
```

编译产物：`sdlpal.js` + `sdlpal.wasm` + `sdlpal.html` + `main.js` + `jszip.min.js`

#### 2. 准备数据文件

将原版仙剑的 `.mkf` 文件打包为 `data.zip`：
```
data.zip
├── abc.mkf
├── ball.mkf
├── data.mkf
├── f.mkf
├── fbp.mkf
├── fire.mkf
├── gop.mkf
├── map.mkf
├── mgo.mkf
├── midi.mkf
├── mus.mkf
├── pat.mkf
├── rgm.mkf
├── rng.mkf
├── sounds.mkf
├── sss.mkf
└── voc.mkf
```

#### 3. 部署到服务器

```bash
# 将以下文件上传到 Web 服务器
sdlpal.html        # 页面入口
sdlpal.js          # WebAssembly 加载器
sdlpal.wasm        # 编译后的游戏引擎
main.js            # JavaScript 交互层
jszip.min.js       # 解压库
icon.png           # 图标
data.zip           # 游戏数据（用户上传或预置）
```

**Nginx 配置示例：**
```nginx
server {
    listen 80;
    server_name your-domain.com;
    
    root /var/www/pal;
    index sdlpal.html;
    
    location / {
        add_header Cross-Origin-Opener-Policy same-origin;
        add_header Cross-Origin-Embedder-Policy require-corp;
    }
}
```

> ⚠️ 注意：WASM 需要正确的 `Cross-Origin-Opener-Policy` 和 `Cross-Origin-Embedder-Policy` 头才能使用线程等功能。

---

## 🎮 操作说明（默认按键）

### 键盘

| 按键 | 功能 |
|------|------|
| 方向键 | 移动 / 菜单选择 |
| A / Enter | 确认 / 调查 |
| B / Esc | 取消 / 返回 |
| F / Y | 打开商城 |
| D | 小地图缩放 / 切换标签 |
| S | 状态 |
| E | 道具 |
| W | 装备 |

### iOS 触屏

| 按钮区域 | 功能 |
|---------|------|
| 左上触控区 | 商城（辅助菜单） |
| 右上触控区 | 返回 / 取消 |
| 左下触控区 | 物品菜单 |
| 右下触控区 | 确认 / 调查 |

---

## 📝 许可证

本项目基于 **SDLPAL** 修改，延续使用 **GNU General Public License v3** 开源协议。

```
Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>
Copyright (c) 2011-2026, SDLPAL development team
Copyright (c) 2025, DragonTang-AI (modifications)
```

完整协议见 [LICENSE](LICENSE) 文件。
