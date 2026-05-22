/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2026, SDLPAL development team.
// All rights reserved.
//
// This file is part of SDLPAL.
//
// SDLPAL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License, version 3
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "main.h"

static int     g_iNumInventory = 0;
static WORD    g_wItemFlags = 0;
static BOOL    g_fNoDesc = FALSE;

//
// Custom item description database (from 仙剑奇侠传1 data)
//
typedef struct { LPCWSTR name; LPCWSTR desc; } _ITEMDESC;

static const _ITEMDESC _itemDescTable[] = {
   { L"短刀", L"一尺半长的钝刀，可以用来劈砍木材，武术+6，身法-5" },
   { L"護肩", L"披于肩臂上的铠甲，又称\"掩膊\"，防御+6" },
   { L"披風", L"无领的对襟、无袖的披衣，俗称斗篷，防御+2" },
   { L"鐵履", L"鞋底加缝铁片，较普通布靴重，防御+6" },
   { L"護腕", L"粗布缝制的腕部护套，防御+2" },
   { L"木劍", L"用木材雕刻的剑，小孩玩具，武术+2 身法+3" },
   { L"草鞋", L"以蔺草编织而成，十分便宜，穿起来很轻便，适宜行走防御+1" },
   { L"布靴", L"粗布缝制的长统靴防御+2 身法+2" },
   { L"藤甲", L"以荆藤编制的护甲防御+7" },
   { L"鐵劍", L"一般铁匠大量生产的剑，打造的颇为粗劣，武术+10 防御+3" },
   { L"香袋", L"填充木屑、香粉的小布包，常用来装饰兼避邪的物品，灵力+8 运气+9 避毒率+20%" },
   { L"仙女劍", L"一尺长的双手剑，适合女子使用，攻击×2，武术+8 防御+5" },
   { L"絲衣", L"以蚕丝纺织而成，轻柔透气防御+3 身法+4" },
   { L"桂花酒", L"掺了水的酒" },
   { L"雄黃", L"天然产的矿物，块状，色黄，可解赤毒" },
   { L"醃肉", L"用盐巴腌渍的猪肉，HP+85" },
   { L"大蒜", L"具有除秽、祛病、护身等功效，可以入药，战斗中服食避毒率+30%" },
   { L"糯米糕", L"糯米加麦芽、甜豆所煮的米糕，可解尸毒，HP+25" },
   { L"還魂香", L"点燃后会散发奇异的香气，能牵引离体的魂魄回归身体，HP恢复50%" },
   { L"止血草", L"嚼碎后敷在伤口上可迅速止血HP+50" },
   { L"淨衣符", L"具有祛病、驱邪的法力，可解赤毒、尸毒、瘴毒" },
   { L"梅花鏢", L"形如梅花的暗器，敌人HP-90" },
   { L"蜂王蜜", L"蜜蜂所酿最好的蜜HPMP+150" },
   { L"還神丹", L"宁神醒脑的药丸MP+50" },
   { L"靈山仙芝", L"寄生于枯木上的菌类，俗称瑞草，具有养气培元之神效MP+260" },
   { L"鼠兒果", L"产于山间野地，多为鼠类所食，经人发现移种平地MP+36" },
   { L"觀音符", L"以观音圣水书写的灵符HP+150" },
   { L"行軍丹", L"活血顺气的药丸HP+100" },
   { L"金創藥", L"上等刀伤药，去腐生肌HP+200" },
   { L"紫金丹", L"水月宫最珍贵的仙丹灵药" },
   { L"酒", L"以米+酒糟酿制而成，可解赤毒HPMP+15" },
   { L"越女劍", L"剑身宽仅两指，专为女子打造，武术+22 身法+8" },
   { L"銀釵", L"纯银的发钗防御+5" },
   { L"翠玉金釵", L"镶有绿翡翠的黄金发钗防御+9" },
   { L"銅鏡", L"青铜铸造的照容用具防御+6" },
   { L"珍珠", L"蚌类所生的球状物，是珍贵的装饰品运气+20" },
   { L"鐵鎖衣", L"以铁环和锁制成的锁甲防御+13 身法-10" },
   { L"大刀", L"刀身宽而长，刃部锋利，背部厚重，武术+16 防御+1" },
   { L"長劍", L"一般铁匠接受订造的剑，比铁剑精致锋利，武术+25" },
   { L"武士披風", L"将帅所穿有护肩软甲的战帔防御+12" },
   { L"芙蓉刀", L"百花门派独门兵器双手弯刀，攻击×2，武术+16 身法+8" },
   { L"護心鏡", L"防御胸前受到伤害的披甲，形如铜镜防御+20" },
   { L"糖葫蘆", L"以竹签串李子裹上麦芽糖，形如葫芦HPMP+22" },
   { L"八仙石", L"八仙石洞所采集之丹矿 防御上限+2" },
   { L"銀針", L"用银针刺肉，以痛楚唤醒神智，可解定身、昏睡、疯魔" },
   { L"袖裡劍", L"暗藏在衣袖中的飞剑，敌人HP-170" },
   { L"金剛符", L"使用后如有金钟铁罩护身，增加防御七回合" },
   { L"蝮蛇涎", L"蝮蛇的毒涎" },
   { L"火蠶蠱", L"以麒麟炎洞内所产生火蚕所培养的蛊虫，可作为攻击道具" },
   { L"贖魂燈", L"以莲灯作法与鬼差交涉，赎回死者魂魄，HP恢复30%" },
   { L"孟婆湯", L"消除死者罪孽业障，使死者复活，HP恢复50%" },
   { L"金蠶王", L"蛊中之王，月夜散发金色磷光，服食后可提升修行" },
   { L"隱蠱", L"形如带刺甲虫，将其身体捏破，散发之烟雾可使我方隐匿形迹，全体隐形3回合" },
   { L"柳月刀", L"细长铁质双刀，形如柳叶新月，攻击×2，武术+28" },
   { L"武僧靴", L"罗汉僧练武时所穿的布靴，防御+8 身法+6" },
   { L"紅纓刀", L"精钢打造，背厚刃薄，刀柄饰以红色长穗，武术+38" },
   { L"天師符", L"茅山道士用来对付妖怪的符咒" },
   { L"試煉果", L"药王神农氏尝百草时最早发现的珍药，灵力上限+3" },
   { L"九陰散", L"服食前若已中毒可补满体力，但无法解毒，服食前若没中毒即刻毙命" },
   { L"天香續命露", L"以大量珍贵秘药精炼而成，有肉白骨之奇效，HP恢复100%" },
   { L"九節菖蒲", L"一种水草，叶子狭长如剑，可解赤毒、尸毒、瘴毒、毒丝" },
   { L"神仙茶", L"神仙广成子养生延寿之秘方HPMP+440" },
   { L"六神丹", L"韩家药铺的祖传妇女良药" },
   { L"念珠", L"佛教徒记数念经咒或佛号次数的计算珠，灵力+5 防御+5" },
   { L"羅漢袍", L"修行得道的和尚所穿的衣袍，防御+10 运气+10 灵力+10" },
   { L"玉佛珠", L"佛法无边，灵力+88 防御+18 避毒率+30%，可与杨戬合体法术" },
   { L"銀杏子", L"银杏树所结的白色核果，HP上限+3" },
   { L"舍利子", L"得道高僧火化后结成的如念珠的东西，真气上限+3" },
   { L"青銅甲", L"青铜制的兽面纹胸护甲，防御+22 身法-13" },
   { L"土靈珠", L"女娲降服山神后禁制山神于内的宝珠，避土率+50%，可用于脱离洞窟" },
   { L"吸星鎖", L"铁制钢抓，尾端系以灵蛊蚕丝，可吸取敌人HP180" },
   { L"透骨釘", L"精铁打造三寸长的铁钉，是很锋利的武器，敌人HP-250" },
   { L"傀儡蟲", L"湘西云贵巫师用以控制尸体，可使死者继续攻击九回合" },
   { L"血玲瓏", L"红色铁球，四周装有锋利刀片，敌全体HP-300" },
   { L"戒刀", L"佛门中人练武所用之刀，严禁伤生染血，武术+55 防御+5 灵力+10" },
   { L"孔雀膽", L"七大毒蛊，中毒后每回合损血致死。解药：金蚕蛊，致命药引：鹤顶红" },
   { L"九節鞭", L"以铁节铁环组成的九节软鞭，武术+66 身法+33" },
   { L"玄鐵劍", L"以珍贵黑色铁矿打造，坚韧锋利但极笨重，武术+75 身法-20 灵力-15" },
   { L"青鋒劍", L"名家精心打造的剑，轻薄锋利。武术+75 身法+15" },
   { L"鐵鱗甲", L"以鱼鳞形甲片编缀而成的铠甲，防御+28 身法-4" },
   { L"鹿皮靴", L"以鹿皮毛缝制，行动可如鹿般迅捷，防御+11 身法+9" },
   { L"夜行衣", L"暗黑的紧身衣，便于隐匿夜色中，防御+18 身法+12" },
   { L"極風靴", L"以博如云雾的蚕纱织成，穿着疾行如风，防御+14 身法+17" },
   { L"霓紅羽衣", L"东海霓红鸟的羽毛织成的披肩，防御+18 身法+18 运气+18" },
   { L"菩提袈裟", L"高等僧衣，又名无垢衣，多为高僧与长者所穿，防御+31 灵力+16" },
   { L"雄黃酒", L"一点点的雄黄，撒在酒中，可解赤毒、瘴毒" },
   { L"紫菁玉蓉膏", L"依宫廷秘方采珍贵药材炼制，疗伤药的极品，HP+1000" },
   { L"天仙玉露", L"观音菩萨净瓶甘露水，人间难求的仙界圣药，MP+700" },
   { L"無影神針", L"细如牛毛，伤人于无形，敌HP-400" },
   { L"五毒珠", L"成精蟾怪的内丹，配戴后百毒不侵" },
   { L"斷腸草", L"七大毒蛊，中毒后每回合损血。解药：三尸蛊，致命药引：金蚕蛊" },
   { L"靈蠱", L"以稀有药材豢养的雌蛊，全体MP+250" },
   { L"雪蛤蟆", L"生长于天山极寒之地，仅铜钱般大小，武术上限+2 防御上限+2" },
   { L"金童劍", L"鸳鸯双剑中的雄剑，与玉女剑为一对，武术+100 运气+30" },
   { L"玉女劍", L"鸳鸯双剑中的雌剑，与金童剑为一对，武术+100 灵力+15" },
   { L"龍泉劍", L"龙泉水质极佳，所造龙泉剑武术+88 身法+20 运气+22" },
   { L"金蛇鞭", L"以蛇皮绞以金丝编织成的9尺软鞭，武术+99 身法+60" },
   { L"精鋼戰甲", L"以椭圆形的精铁片编缀而成，又称光明铠，防御+40 身法-7" },
   { L"雪蓮子", L"白玉雪莲之莲子，服食者真气充盈经脉通畅，MP+400" },
   { L"靈心符", L"有定神、驱邪的灵效，可解疯魔、定身、昏睡、封咒" },
   { L"雷靈珠", L"女娲降服雷神后禁制雷神于内的法珠。合体法术：狂雷" },
   { L"靈葫仙丹", L"修道隐士所炼丹药HPMP+250" },
   { L"七星劍", L"剑身镶嵌七颗金黄宝石，可吸取北斗七星之精气，武术+120 灵力+50" },
   { L"天師道袍", L"天师道祖修行时所穿的法衣，防御+33 灵力+28" },
   { L"鳳鳴刀", L"出鞘之声有如凤鸣，故称凤鸣刀，武术+124 防御+9" },
   { L"雙龍劍", L"双手剑，攻击×2，武术+62 防御+9 身法+9" },
   { L"女媧石", L"女神娲皇炼石补天所遗之五色石，防御上限+3" },
   { L"蟠果", L"西王母蟠桃园的遗种，汁液香甜，HP+450" },
   { L"玉菩提", L"墨玉菩提树的树籽，真气上限+5" },
   { L"毒龍膽", L"千年毒蛟的胆，以毒攻毒可解天下所有的毒，若未中毒吃会毙命" },
   { L"鳳凰羽毛", L"金翅凤凰腹部的银色羽毛，防御+7 身法+24 运气+9" },
   { L"風靈珠", L"女娲降服风神后禁制风神于内的宝珠" },
   { L"虎紋披風", L"以整张千年白额虎虎皮制成，防御+40" },
   { L"磐龍劍", L"铸剑宗师欧冶子所炼宝剑，剑身柱有青龙盘柱，武术+134 灵力+37" },
   { L"苗刀", L"苗族战士所惯用的佩刀，武术+70 身法+32" },
   { L"乾坤鏡", L"铜镜背面铸有太极乾坤图，灵气+14 防御+14" },
   { L"青蛇杖", L"雕刻双蛇缠绕的绿玉杖，武术+50 灵力+62 防御+6" },
   { L"壽葫蘆", L"战斗中发出真气充持有者，HPMP每回合+20" },
   { L"金罡珠", L"大罗金仙修炼千年的内丹，防御+90" },
   { L"沖天冠", L"天兵神将遗留的护头金盔，防御+28 法力+3" },
   { L"鬼針胄", L"长满倒刺的铜制盔甲，防御+55 武术+9" },
   { L"鳳紋披風", L"相传为织女编制的披风，绣凤织锦，防御+52" },
   { L"鬼頭杖", L"苗族巫师役鬼炼蛊之法器，武术+70 灵力+88 防御+11" },
   { L"天蠶寶衣", L"以珍贵的天蚕丝编织而成，轻薄柔韧防御+66" },
   { L"火靈珠", L"女娲降服火神之后禁锢火神于内，合体法术：炼狱真火" },
   { L"步雲靴", L"云中子羽化登天后的神靴，防御+28 身法+20" },
   { L"天蛇杖", L"女神娲皇炼化五色石时所用的法杖，武术+100 灵力+150" },
   { L"水靈珠", L"女娲降服水神之后禁锢水神于内，合体法术：冰天雪地" },
   { L"魅影神靴", L"妖魔附体，身如鬼魅，防御+32 身法+26" },
   { L"玄冥寶刀", L"可连续攻击敌全体两次，传说是魔族的邪异兵器" },
   { L"冥蛇杖", L"来自冥界之魔杖，武术+88 灵力+120 防御+22" },
   { L"無塵劍", L"上古神剑，指天天崩，划地低裂，武术+200 防御+20" },
   { L"鶴頂紅", L"七大毒蛊，中毒后每回合损血。解药：血海棠，药引：孔雀胆" },
   { L"金蠶蠱", L"七大毒蛊，中毒后每回合损血。解药：鹤顶红，药引：断肠草" },
   { L"三屍蠱", L"七大毒蛊，潜伏后毒性猛烈。解药：鹤顶红，药引：血海棠" },
   { L"聖靈珠", L"女娲末族祖传宝物，历代圣魂归依之所。合体法术：武神" },
   { L"聖靈披風", L"巫后的遗物，潜藏神圣的力量，防御+66 灵力+30" },
   { L"朱雀戰衣", L"以南方火鸟的羽毛编制而成，防御+80" },
   { L"青龍寶甲", L"龙鳞编制而成，世间绝顶战甲，防御+90" },
};
#define _ITEMDESC_COUNT (sizeof(_itemDescTable) / sizeof(_itemDescTable[0]))

static LPCWSTR PAL_GetCustomItemDesc(WORD wObjectID)
{
   if (wObjectID == 0) return NULL;
   LPCWSTR itemName = PAL_GetWord(wObjectID);
   if (!itemName || !itemName[0]) return NULL;

   // Direct match
   for (int i = 0; i < _ITEMDESC_COUNT; i++) {
      if (wcscmp(itemName, _itemDescTable[i].name) == 0)
         return _itemDescTable[i].desc;
   }

   return NULL;
}

//
// Item detail popup
//
static void PAL_ItemDetailPopup(WORD wObject)
{
   if (wObject == 0) return;
   SDL_Surface *saved = SDL_CreateRGBSurface(SDL_SWSURFACE, 320, 200, 8, 0, 0, 0, 0);
   SDL_BlitSurface(gpScreen, NULL, saved, NULL);
   SDL_Rect bg; bg.x=30; bg.y=15; bg.w=260; bg.h=170;
   SDL_FillRect(gpScreen, &bg, 0x11);
   bg.x=31; bg.y=16; bg.w=258; bg.h=168;
   SDL_FillRect(gpScreen, &bg, 0x00);
   bg.x=30; bg.y=15; bg.w=260; bg.h=14;
   SDL_FillRect(gpScreen, &bg, 0x2C);
   PAL_DrawText(PAL_GetWord(wObject), PAL_XY(75, 34), 0x4F, TRUE, FALSE, FALSE);
   WCHAR _w[48];
   PAL_swprintf(_w, 48, L"\u4EF7\u683C: %d", gpGlobals->g.rgObject[wObject].item.wPrice);
   PAL_DrawText(_w, PAL_XY(75, 50), 0x1B, TRUE, FALSE, FALSE);
   int amt = PAL_GetItemAmount(wObject);
   PAL_swprintf(_w, 48, L"\u6301\u6709: %d", amt);
   PAL_DrawText(_w, PAL_XY(75, 64), 0x1C, TRUE, FALSE, FALSE);
   WORD fl = gpGlobals->g.rgObject[wObject].item.wFlags;
   int y2 = 80;
   if (fl & kItemFlagEquipable) { PAL_DrawText(L"[\u88C5\u5907]", PAL_XY(75, y2), 0x2C, TRUE, FALSE, FALSE); y2 += 12; }
   if (fl & kItemFlagUsable)    { PAL_DrawText(L"[\u4F7F\u7528]", PAL_XY(75, y2), 0x2C, TRUE, FALSE, FALSE); y2 += 12; }
   if (fl & kItemFlagConsuming) { PAL_DrawText(L"[\u6D88\u8017]", PAL_XY(75, y2), 0x2C, TRUE, FALSE, FALSE); y2 += 12; }
   if (fl & kItemFlagThrowable) { PAL_DrawText(L"[\u6295\u63B7]", PAL_XY(75, y2), 0x2C, TRUE, FALSE, FALSE); }
   PAL_DrawText(L"\u2014\u2014 \u8BF4\u660E \u2014\u2014", PAL_XY(110, 112), 0x1B, TRUE, FALSE, FALSE);
   const WCHAR *_desc = NULL;

   // Try game description first (only if non-empty)
   if (gpGlobals->lpObjectDesc != NULL) {
      _desc = PAL_GetObjectDesc(gpGlobals->lpObjectDesc, wObject);
      if (_desc != NULL && _desc[0] == L'\0') _desc = NULL; // skip empty descs
   }
   // Fallback to our custom knowledge base
   if (_desc == NULL) {
      _desc = PAL_GetCustomItemDesc(wObject);
   }

   if (_desc != NULL) {
      WCHAR sz[256]; wcscpy(sz, _desc);
      int ky = 128; WCHAR *p = sz;
      while (1) {
         WCHAR *nx = wcschr(p, '*');
         if (nx) *nx++ = '\0';
         PAL_DrawText(p, PAL_XY(35, ky), 0x1C, TRUE, FALSE, FALSE);
         ky += 14;
         if (!nx) break; p = nx;
      }
   }
   PAL_DrawText(L"[A/B] \u8FD4\u56DE", PAL_XY(130, 172), 0x11, TRUE, FALSE, TRUE);
   VIDEO_UpdateScreen(NULL);
   g_InputState.dir = kDirUnknown; PAL_ClearKeyState();
   while (1) { PAL_ClearKeyState(); UTIL_Delay(50); if (g_InputState.dwKeyPress & (kKeyMenu|kKeySearch|kKeyForce)) break; }
   PAL_ClearKeyState();
   SDL_BlitSurface(saved, NULL, gpScreen, NULL);
   SDL_FreeSurface(saved);
}

WORD
PAL_ItemSelectMenuUpdate(
   VOID
)
/*++
  Purpose:

    Initialize the item selection menu.

  Parameters:

    None.

  Return value:

    The object ID of the selected item. 0 if cancelled, 0xFFFF if not confirmed.

--*/
{
   int                i, j, k, line, item_delta;
   WORD               wObject, wScript;
   BYTE               bColor;
   static BYTE        bufImage[2048];
   const int          iItemsPerLine = 32 / gConfig.dwWordLength;
   const int          iItemTextWidth = 8 * gConfig.dwWordLength + 20;
   const int          iLinesPerPage = 7 - gConfig.ScreenLayout.ExtraItemDescLines;
   const int          iCursorXOffset = gConfig.dwWordLength * 5 / 2;
   const int          iAmountXOffset = gConfig.dwWordLength * 8 + 1;
   const int          iPageLineOffset = (iLinesPerPage + 1) / 2;
   const int          iPictureYOffset = (gConfig.ScreenLayout.ExtraItemDescLines > 1) ? (gConfig.ScreenLayout.ExtraItemDescLines - 1) * 16 : 0;
   PAL_POS            cursorPos = PAL_XY(15 + iCursorXOffset, 22);;

   //
   // Process input
   //
   if (g_InputState.dwKeyPress & kKeyUp)
   {
      item_delta = -iItemsPerLine;
   }
   else if (g_InputState.dwKeyPress & kKeyDown)
   {
      item_delta = iItemsPerLine;
   }
   else if (g_InputState.dwKeyPress & kKeyLeft)
   {
      item_delta = -1;
   }
   else if (g_InputState.dwKeyPress & kKeyRight)
   {
      item_delta = 1;
   }
   else if (g_InputState.dwKeyPress & kKeyPgUp)
   {
      item_delta = -(iItemsPerLine * iLinesPerPage);
   }
   else if (g_InputState.dwKeyPress & kKeyPgDn)
   {
      item_delta = iItemsPerLine * iLinesPerPage;
   }
   else if (g_InputState.dwKeyPress & kKeyHome)
   {
      item_delta = -gpGlobals->iCurInvMenuItem;
   }
   else if (g_InputState.dwKeyPress & kKeyEnd)
   {
      item_delta = g_iNumInventory - gpGlobals->iCurInvMenuItem - 1;
   }
   else if (g_InputState.dwKeyPress & kKeyMenu)
   {
      return 0;
   }
   else if (g_InputState.dwKeyPress & kKeyForce)
   {
      //
      // Show item detail popup (Y key / touch button 1)
      //
      wObject = gpGlobals->rgInventory[gpGlobals->iCurInvMenuItem].wItem;
      if (wObject != 0)
      {
         PAL_ItemDetailPopup(wObject);
         UTIL_Delay(150);
      }
   }
   else
   {
      item_delta = 0;
   }

   //
   // Make sure the current menu item index is in bound
   //
   if (gpGlobals->iCurInvMenuItem + item_delta < 0)
      gpGlobals->iCurInvMenuItem = 0;
   else if (gpGlobals->iCurInvMenuItem + item_delta >= g_iNumInventory)
      gpGlobals->iCurInvMenuItem = g_iNumInventory-1;
   else
      gpGlobals->iCurInvMenuItem += item_delta;

   //
   // Redraw the box
   //
   PAL_CreateBoxWithShadow(PAL_XY(2, 0), iLinesPerPage - 1, 17, 1, FALSE, 0);

   //
   // Draw the texts in the current page
   //
   i = gpGlobals->iCurInvMenuItem / iItemsPerLine * iItemsPerLine - iItemsPerLine * iPageLineOffset;
   if (i < 0)
   {
      i = 0;
   }

   const int xBase = 0, yBase = 140;

   for (j = 0; j < iLinesPerPage; j++)
   {
      for (k = 0; k < iItemsPerLine; k++)
      {
         wObject = gpGlobals->rgInventory[i].wItem;
         bColor = MENUITEM_COLOR;

         if (i >= MAX_INVENTORY || wObject == 0)
         {
            //
            // End of the list reached
            //
            j = iLinesPerPage;
            break;
         }

         if (i == gpGlobals->iCurInvMenuItem)
         {
            if (!(gpGlobals->g.rgObject[wObject].item.wFlags & g_wItemFlags) ||
               (SHORT)gpGlobals->rgInventory[i].nAmount <= (SHORT)gpGlobals->rgInventory[i].nAmountInUse)
            {
               //
               // This item is not selectable
               //
               bColor = MENUITEM_COLOR_SELECTED_INACTIVE;
            }
            else
            {
               //
               // This item is selectable
               //
               if (gpGlobals->rgInventory[i].nAmount == 0)
               {
                  bColor = MENUITEM_COLOR_EQUIPPEDITEM;
               }
               else
               {
                  bColor = MENUITEM_COLOR_SELECTED;
               }
            }
         }
         else if (!(gpGlobals->g.rgObject[wObject].item.wFlags & g_wItemFlags) ||
            (SHORT)gpGlobals->rgInventory[i].nAmount <= (SHORT)gpGlobals->rgInventory[i].nAmountInUse)
         {
            //
            // This item is not selectable
            //
            bColor = MENUITEM_COLOR_INACTIVE;
         }
         else if (gpGlobals->rgInventory[i].nAmount == 0)
         {
            bColor = MENUITEM_COLOR_EQUIPPEDITEM;
         }

         //
         // Draw the text
         //
         PAL_DrawText(PAL_GetWord(wObject), PAL_XY(15 + k * iItemTextWidth, 12 + j * 18), bColor, TRUE, FALSE, FALSE);

         if (i == gpGlobals->iCurInvMenuItem)
         {
            cursorPos = PAL_XY(15 + iCursorXOffset + k * iItemTextWidth, 22 + j * 18);

            //
            // Draw the picture of current selected item
            //
            PAL_RLEBlitToSurfaceWithShadow(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_ITEMBOX), gpScreen,
               PAL_XY(xBase + 5, yBase + 5 - iPictureYOffset), TRUE);
            PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_ITEMBOX), gpScreen,
               PAL_XY(xBase, yBase - iPictureYOffset));

            if (PAL_MKFReadChunk(bufImage, 2048,
               gpGlobals->g.rgObject[wObject].item.wBitmap, gpGlobals->f.fpBALL) > 0)
            {
               PAL_RLEBlitToSurface(bufImage, gpScreen, PAL_XY(xBase + 8, yBase + 7 - iPictureYOffset));
            }
         }

         //
         // Draw the amount of this item
         //
         if ((SHORT)gpGlobals->rgInventory[i].nAmount - (SHORT)gpGlobals->rgInventory[i].nAmountInUse > 1)
         {
            PAL_DrawNumber(gpGlobals->rgInventory[i].nAmount - gpGlobals->rgInventory[i].nAmountInUse,
               2, PAL_XY(15 + iAmountXOffset + k * iItemTextWidth, 17 + j * 18), kNumColorCyan, kNumAlignRight);
         }

         i++;
      }
   }

   //
   // Draw the cursor on the current selected item
   //
   PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_CURSOR), gpScreen, cursorPos);

   wObject = gpGlobals->rgInventory[gpGlobals->iCurInvMenuItem].wItem;

   //
   // Draw the description of the selected item
   //
   if (!gConfig.fIsWIN95)
   {
      if (!g_fNoDesc && gpGlobals->lpObjectDesc != NULL)
	  {
         WCHAR szDesc[512], *next;
         const WCHAR *d = PAL_GetObjectDesc(gpGlobals->lpObjectDesc, wObject);

         if (d != NULL)
         {
            k = 150 - gConfig.ScreenLayout.ExtraItemDescLines * 16;
            wcscpy(szDesc, d);
            d = szDesc;

            while (TRUE)
            {
               next = wcschr(d, '*');
               if (next != NULL)
               {
                  *next++ = '\0';
               }

               PAL_DrawText(d, PAL_XY(75, k), DESCTEXT_COLOR, TRUE, FALSE, FALSE);
               k += 16;

               if (next == NULL)
               {
                  break;
               }

               d = next;
            }
         }
      }
   }
   else
   {
      if (!g_fNoDesc)
      {
         wScript = gpGlobals->g.rgObject[wObject].item.wScriptDesc;
         line = 0;
         while (wScript && gpGlobals->g.lprgScriptEntry[wScript].wOperation != 0)
         {
            if (gpGlobals->g.lprgScriptEntry[wScript].wOperation == 0xFFFF)
            {
               int line_incr = (gpGlobals->g.lprgScriptEntry[wScript].rgwOperand[1] != 1) ? 1 : 0;
               wScript = PAL_RunAutoScript(wScript, PAL_ITEM_DESC_BOTTOM | line);
               line += line_incr;
            }
            else
            {
               wScript = PAL_RunAutoScript(wScript, 0);
            }
         }
      }
   }

   if (g_InputState.dwKeyPress & kKeySearch)
   {
      if ((gpGlobals->g.rgObject[wObject].item.wFlags & g_wItemFlags) &&
         (SHORT)gpGlobals->rgInventory[gpGlobals->iCurInvMenuItem].nAmount >
         (SHORT)gpGlobals->rgInventory[gpGlobals->iCurInvMenuItem].nAmountInUse)
      {
         if (gpGlobals->rgInventory[gpGlobals->iCurInvMenuItem].nAmount > 0)
         {
            j = (gpGlobals->iCurInvMenuItem < iItemsPerLine * iPageLineOffset) ? (gpGlobals->iCurInvMenuItem / iItemsPerLine) : iPageLineOffset;
            k = gpGlobals->iCurInvMenuItem % iItemsPerLine;

            PAL_DrawText(PAL_GetWord(wObject), PAL_XY(15 + k * iItemTextWidth, 12 + j * 18), MENUITEM_COLOR_CONFIRMED, FALSE, FALSE, FALSE);

            //
            // Draw the cursor on the current selected item
            //
            PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_CURSOR), gpScreen, cursorPos);
         }

         return wObject;
      }
   }

   return 0xFFFF;
}

VOID
PAL_ItemSelectMenuInit(
   WORD                      wItemFlags
)
/*++
  Purpose:

    Initialize the item selection menu.

  Parameters:

    [IN]  wItemFlags - flags for usable item.

  Return value:

    None.

--*/
{
   int           i, j;
   WORD          w;

   g_wItemFlags = wItemFlags;

   //
   // Compress the inventory
   //
   PAL_CompressInventory();

   //
   // Count the total number of items in inventory
   //
   g_iNumInventory = 0;
   while (g_iNumInventory < MAX_INVENTORY &&
      gpGlobals->rgInventory[g_iNumInventory].wItem != 0)
   {
      g_iNumInventory++;
   }

   //
   // Also add usable equipped items to the list
   //
   if ((wItemFlags & kItemFlagUsable) && !gpGlobals->fInBattle)
   {
      for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
      {
         w = gpGlobals->rgParty[i].wPlayerRole;

         for (j = 0; j < MAX_PLAYER_EQUIPMENTS; j++)
         {
            if (gpGlobals->g.rgObject[gpGlobals->g.PlayerRoles.rgwEquipment[j][w]].item.wFlags & kItemFlagUsable)
            {
               if (g_iNumInventory < MAX_INVENTORY)
               {
                  gpGlobals->rgInventory[g_iNumInventory].wItem = gpGlobals->g.PlayerRoles.rgwEquipment[j][w];
                  gpGlobals->rgInventory[g_iNumInventory].nAmount = 0;
                  gpGlobals->rgInventory[g_iNumInventory].nAmountInUse = (WORD)-1;

                  g_iNumInventory++;
               }
            }
         }
      }
   }
}

WORD
PAL_ItemSelectMenu(
   LPITEMCHANGED_CALLBACK    lpfnMenuItemChanged,
   WORD                      wItemFlags
)
/*++
  Purpose:

    Show the item selection menu.

  Parameters:

    [IN]  lpfnMenuItemChanged - Callback function which is called when user
                                changed the current menu item.

    [IN]  wItemFlags - flags for usable item.

  Return value:

    The object ID of the selected item. 0 if cancelled.

--*/
{
   int              iPrevIndex;
   WORD             w;
   DWORD            dwTime;

   PAL_ItemSelectMenuInit(wItemFlags);
   iPrevIndex = gpGlobals->iCurInvMenuItem;

   PAL_ClearKeyState();

   if (lpfnMenuItemChanged != NULL)
   {
      g_fNoDesc = TRUE;
      (*lpfnMenuItemChanged)(gpGlobals->rgInventory[gpGlobals->iCurInvMenuItem].wItem);
   }

   dwTime = SDL_GetTicks();

   while (TRUE)
   {
      if (lpfnMenuItemChanged == NULL)
      {
         PAL_MakeScene();
      }

      w = PAL_ItemSelectMenuUpdate();
      VIDEO_UpdateScreen(NULL);

      PAL_ClearKeyState();

      PAL_ProcessEvent();
      while (!SDL_TICKS_PASSED(SDL_GetTicks(), dwTime))
      {
         PAL_ProcessEvent();
         if (g_InputState.dwKeyPress != 0)
         {
            break;
         }
         SDL_Delay(5);
      }

      dwTime = SDL_GetTicks() + FRAME_TIME;

      if (w != 0xFFFF)
      {
         g_fNoDesc = FALSE;
         return w;
      }

      if (iPrevIndex != gpGlobals->iCurInvMenuItem)
      {
         if (gpGlobals->iCurInvMenuItem >= 0 && gpGlobals->iCurInvMenuItem < MAX_INVENTORY)
         {
            if (lpfnMenuItemChanged != NULL)
            {
               (*lpfnMenuItemChanged)(gpGlobals->rgInventory[gpGlobals->iCurInvMenuItem].wItem);
            }
         }

         iPrevIndex = gpGlobals->iCurInvMenuItem;
      }
   }

   assert(FALSE);
   return 0; // should not really reach here
}
