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

//
// Minimap system - Enhanced version
//
// Three zoom levels:
//   0 = Off
//   1 = Small  (64x64 px, 2 tiles/px)
//   2 = Medium (128x96 px, 1 tile/px)
//
#define MINIMAP_ZOOM_MAX      2

static BYTE   g_bMMZoom   = 1;          // current zoom level
static BOOL   g_bMMWorldMap = FALSE;

static void MM_SetZoom(BYTE z)
{
   g_bMMZoom = (z > MINIMAP_ZOOM_MAX) ? 0 : z;
   gpGlobals->fShowMinimap = (g_bMMZoom > 0);
}

static void MM_ToggleZoom(void)
{
   MM_SetZoom((g_bMMZoom + 1) % (MINIMAP_ZOOM_MAX + 1));
}

static void MM_InitForMap(void)
{
   LPPALMAP pMap = PAL_GetCurrentMap();
   if (!pMap) return;
   int mx = 0, my = 0;
   for (int x = 0; x < 128; x++)
      for (int y = 0; y < 64; y++)
         for (int h = 0; h < 2; h++)
            if (pMap->Tiles[x][y][h]) { if (x > mx) mx = x; if (y > my) my = y; }
   gpGlobals->iMinimapTileW = mx + 1;
   gpGlobals->iMinimapTileH = my + 1;
   gpGlobals->iMinimapMapNum = gpGlobals->wNumScene;
   gpGlobals->fAutoNavActive = FALSE;
   gpGlobals->fShowMinimap = (g_bMMZoom > 0);
   gpGlobals->fFullscreenMap = FALSE;
   BYTE bx, by, bh;
   int sx = PAL_X(gpGlobals->viewport) + PAL_X(gpGlobals->partyoffset);
   int sy = PAL_Y(gpGlobals->viewport) + PAL_Y(gpGlobals->partyoffset);
   PAL_POS_TO_XYH(PAL_XY(sx, sy), bx, by, bh);
   if (bx < MAP_TILE_MAX_X && by < MAP_TILE_MAX_Y && bh < MAP_TILE_MAX_H)
      gpGlobals->explored[bh*MAP_TILE_MAX_X*MAP_TILE_MAX_Y + by*MAP_TILE_MAX_X + bx] = 1;
}

static void MM_MarkExplored(void)
{
   BYTE bx, by, bh;
   int sx = PAL_X(gpGlobals->viewport) + PAL_X(gpGlobals->partyoffset);
   int sy = PAL_Y(gpGlobals->viewport) + PAL_Y(gpGlobals->partyoffset);
   PAL_POS_TO_XYH(PAL_XY(sx, sy), bx, by, bh);
   if (bx < MAP_TILE_MAX_X && by < MAP_TILE_MAX_Y && bh < MAP_TILE_MAX_H)
      gpGlobals->explored[bh*MAP_TILE_MAX_X*MAP_TILE_MAX_Y + by*MAP_TILE_MAX_X + bx] = 1;
}

//
// Classify a tile for minimap rendering
//
typedef enum {
   kMMTileVoid,       // no tile at all
   kMMTileFloor,      // has ground tile (walkable)
   kMMTileWall,       // has object tile (blocked)
   kMMTileSpecial,    // has event/trigger tile
} MMTILETYPE;

static MMTILETYPE MM_ClassifyTile(LPPALMAP pMap, int x, int y)
{
   WORD t0 = pMap->Tiles[x][y][0]; // ground layer
   WORD t1 = pMap->Tiles[x][y][1]; // object layer
   WORD t2 = pMap->Tiles[x][y][2]; // event layer (not used in standard maps)

   if (t0 == 0 && t1 == 0) return kMMTileVoid;
   if (t1 != 0) return kMMTileWall;   // obstacle/wall takes priority
   if (t0 != 0) return kMMTileFloor;  // walkable ground
   return kMMTileVoid;
}

//
// Draw a direction indicator (triangle) for player position
//
static void MM_DrawPlayerDot(SDL_Surface *lpSurface, int cx, int cy, BYTE dir)
{
   SDL_Rect rc;
   // Draw a 3x3 player indicator with direction triangle
   // dir: 0=South, 1=West, 2=North, 3=East
   switch (dir) {
   case 0: // South (down)
      rc.x = cx; rc.y = cy-1; rc.w = 3; rc.h = 1; SDL_FillRect(lpSurface, &rc, 0x4A);
      rc.x = cx+1; rc.y = cy; rc.w = 1; rc.h = 2; SDL_FillRect(lpSurface, &rc, 0x4A);
      break;
   case 1: // West (left)
      rc.x = cx-1; rc.y = cy; rc.w = 3; rc.h = 1; SDL_FillRect(lpSurface, &rc, 0x4A);
      rc.x = cx; rc.y = cy+1; rc.w = 2; rc.h = 1; SDL_FillRect(lpSurface, &rc, 0x4A);
      break;
   case 2: // North (up)
      rc.x = cx; rc.y = cy+1; rc.w = 3; rc.h = 1; SDL_FillRect(lpSurface, &rc, 0x4A);
      rc.x = cx+1; rc.y = cy-1; rc.w = 1; rc.h = 2; SDL_FillRect(lpSurface, &rc, 0x4A);
      break;
   case 3: // East (right)
      rc.x = cx-1; rc.y = cy; rc.w = 3; rc.h = 1; SDL_FillRect(lpSurface, &rc, 0x4A);
      rc.x = cx-1; rc.y = cy+1; rc.w = 2; rc.h = 1; SDL_FillRect(lpSurface, &rc, 0x4A);
      break;
   default:
      rc.x = cx; rc.y = cy; rc.w = 3; rc.h = 3;
      SDL_FillRect(lpSurface, &rc, 0x4A);
   }
}

static void MM_DrawMinimap(SDL_Surface *lpSurface)
{
   LPPALMAP pMap = PAL_GetCurrentMap();
   if (!pMap || !gpGlobals->fShowMinimap || gpGlobals->fFullscreenMap) return;

   int scale = (g_bMMZoom == 2) ? 1 : 2; // 1 = 1px/tile (medium), 2 = 2tiles/px (small)
   int mmW = 64 * 2 / scale;  // 64px for small (128 tiles / 2), 128px for medium
   int mmH = 32 * 2 / scale;  // 32px for small (64 tiles / 2), 64px for medium
   if (mmW > 128) mmW = 128;
   if (mmH > 64) mmH = 64;

   int ox = 320 - mmW - 4;
   int oy = 4;

   // Border (subtle dark)
   SDL_Rect rc;
   rc.x = ox - 1; rc.y = oy - 1; rc.w = mmW + 2; rc.h = mmH + 2;
   SDL_FillRect(lpSurface, &rc, 0x11);
   rc.x = ox; rc.y = oy; rc.w = mmW; rc.h = mmH;
   SDL_FillRect(lpSurface, &rc, 0x00);

   int bw = gpGlobals->iMinimapTileW;
   int bh = gpGlobals->iMinimapTileH;
   if (bw < 2) bw = 2;
   if (bh < 2) bh = 2;

   // Clamp rendered area
   int maxX = (bw < 128) ? bw : 128;
   int maxY = (bh < 64) ? bh : 64;

   // Draw tiles
   for (int x = 0; x < maxX; x += scale)
   {
      for (int y = 0; y < maxY; y += scale)
      {
         // Determine tile type
         MMTILETYPE tt = MM_ClassifyTile(pMap, x, y);
         if (tt == kMMTileVoid) continue;

         // Check explored status
         BOOL explored = FALSE;
         for (int h = 0; h < MAP_TILE_MAX_H; h++)
         {
            int idx = h * MAP_TILE_MAX_X * MAP_TILE_MAX_Y + y * MAP_TILE_MAX_X + x;
            if (x < MAP_TILE_MAX_X && y < MAP_TILE_MAX_Y && gpGlobals->explored[idx])
            {
               explored = TRUE;
               break;
            }
         }

         BYTE color;
         if (explored)
         {
            switch (tt)
            {
            case kMMTileWall:    color = 0x1C; break; // gray
            case kMMTileFloor:   color = 0x2C; break; // green
            case kMMTileSpecial: color = 0x5B; break; // yellow
            default:             color = 0x11; break;
            }
         }
         else
         {
            // Fog of war - show outline but dim
            switch (tt)
            {
            case kMMTileWall:    color = 0x11; break; // dark gray
            case kMMTileFloor:   color = 0x22; break; // dark green
            case kMMTileSpecial: color = 0x33; break; // dark yellow
            default:             color = 0x00; break;
            }
         }

         int px = ox + x / scale;
         int py = oy + y / scale;
         if (px >= ox && px < ox + mmW && py >= oy && py < oy + mmH)
         {
            rc.x = px; rc.y = py; rc.w = 1; rc.h = 1;
            SDL_FillRect(lpSurface, &rc, color);
         }
      }
   }

   // Draw player position with direction indicator
   BYTE px, py, ph;
   int sx = PAL_X(gpGlobals->viewport) + PAL_X(gpGlobals->partyoffset);
   int sy = PAL_Y(gpGlobals->viewport) + PAL_Y(gpGlobals->partyoffset);
   PAL_POS_TO_XYH(PAL_XY(sx, sy), px, py, ph);

   int ppx = ox + px / scale;
   int ppy = oy + py / scale;
   if (ppx >= ox && ppx < ox + mmW && ppy >= oy && ppy < oy + mmH)
   {
      MM_DrawPlayerDot(lpSurface, ppx - 1, ppy - 1, (BYTE)(gpGlobals->wPartyDirection & 3));
   }

   // Draw viewport indicator (where the visible screen is)
   if (scale == 1)
   {
      // Medium zoom: show viewport rectangle
      int vx = ox + PAL_X(gpGlobals->viewport) / 16 / scale;
      int vy = oy + PAL_Y(gpGlobals->viewport) / 16 / scale;
      int vw = 320 / 16 / scale;
      int vh = 200 / 16 / scale;
      if (vw < 2) vw = 2;
      if (vh < 2) vh = 2;
      SDL_Rect vr;
      vr.x = vx; vr.y = vy; vr.w = vw; vr.h = 1; SDL_FillRect(lpSurface, &vr, 0x1B);
      vr.y = vy + vh; SDL_FillRect(lpSurface, &vr, 0x1B);
      vr.x = vx; vr.y = vy; vr.w = 1; vr.h = vh; SDL_FillRect(lpSurface, &vr, 0x1B);
      vr.x = vx + vw; SDL_FillRect(lpSurface, &vr, 0x1B);
   }

   // Scene info below minimap
   WCHAR _buf[24];
   if (scale == 2) {
      // Small mode - shorter format to fit 64px width
      PAL_swprintf(_buf, 24, L"%d,%d #%d", px, py, gpGlobals->wNumScene);
   } else {
      PAL_swprintf(_buf, 24, L"#%03d  %d,%d", gpGlobals->wNumScene, px, py);
   }
   PAL_DrawText(_buf, PAL_XY(ox, oy + mmH + 1), 0x1B, TRUE, FALSE, TRUE);
}


VOID
PAL_GameUpdate(
   BOOL       fTrigger
)
/*++
  Purpose:

    The main game logic routine. Update the status of everything.

  Parameters:

    [IN]  fTrigger - whether to process trigger events or not.

  Return value:

    None.

--*/
{
   WORD            wEventObjectID, wDir;
   int             i;
   LPEVENTOBJECT   p;
   WORD            wResult;

   //
   // Check for trigger events
   //
   if (fTrigger)
   {
      //
      // Check if we are entering a new scene
      //
      if (gpGlobals->fEnteringScene)
      {
         //
         // Run the script for entering the scene
         //
         gpGlobals->fEnteringScene = FALSE;

         i = gpGlobals->wNumScene - 1;
         gpGlobals->g.rgScene[i].wScriptOnEnter = PAL_RunTriggerScript(gpGlobals->g.rgScene[i].wScriptOnEnter, 0xFFFF);

         if (gpGlobals->fInMainGame) MM_InitForMap();

         //
         // Teleport: mark this scene as visited
         //
         {
            WORD ws = gpGlobals->wNumScene;
            BOOL found = FALSE;
            for (int _ti = 0; _ti < MAX_TRAVEL_LOCATIONS; _ti++) {
               if (gpGlobals->rgTravelScenesVisited[_ti] == ws) { found = TRUE; break; }
               if (gpGlobals->rgTravelScenesVisited[_ti] == 0) break;
            }
            if (!found) {
               for (int _ti = 0; _ti < MAX_TRAVEL_LOCATIONS; _ti++) {
                  if (gpGlobals->rgTravelScenesVisited[_ti] == 0) {
                     gpGlobals->rgTravelScenesVisited[_ti] = ws;
                     break;
                  }
               }
            }
         }

         if (gpGlobals->fEnteringScene)
         {
            //
            // Don't go further as we're switching to another scene
            //
            return;
         }

         PAL_ClearKeyState();
         PAL_MakeScene();
      }

      //
      // Loop through all event objects in the current scene
      //
      for (wEventObjectID = gpGlobals->g.rgScene[gpGlobals->wNumScene - 1].wEventObjectIndex + 1;
         wEventObjectID <= gpGlobals->g.rgScene[gpGlobals->wNumScene].wEventObjectIndex;
         wEventObjectID++)
      {
         p = &gpGlobals->g.lprgEventObject[wEventObjectID - 1];

         if (p->sVanishTime != 0)
         {
            //
            // Update the vanish time for all event objects
            //
            p->sVanishTime += ((p->sVanishTime < 0) ? 1 : -1);
            continue;
         }

         if (p->sState < 0)
         {
            if (p->x < PAL_X(gpGlobals->viewport) ||
               p->x > PAL_X(gpGlobals->viewport) + 320 ||
               p->y < PAL_Y(gpGlobals->viewport) ||
               p->y > PAL_Y(gpGlobals->viewport) + 320)
            {
               p->sState = abs(p->sState);
               p->wCurrentFrameNum = 0;
            }
         }
         else if (p->sState > 0 && p->wTriggerMode >= kTriggerTouchNear)
         {
            //
            // This event object can be triggered without manually exploring
            //
            if (abs(PAL_X(gpGlobals->viewport) + PAL_X(gpGlobals->partyoffset) - p->x) +
               abs(PAL_Y(gpGlobals->viewport) + PAL_Y(gpGlobals->partyoffset) - p->y) * 2 <
               (p->wTriggerMode - kTriggerTouchNear) * 32 + 16)
            {
               //
               // Player is in the trigger zone.
               //

               if (p->nSpriteFrames)
               {
                  //
                  // The sprite has multiple frames. Try to adjust the direction.
                  //
                  int                xOffset, yOffset;

                  p->wCurrentFrameNum = 0;

                  xOffset = PAL_X(gpGlobals->viewport) + PAL_X(gpGlobals->partyoffset) - p->x;
                  yOffset = PAL_Y(gpGlobals->viewport) + PAL_Y(gpGlobals->partyoffset) - p->y;

                  if (xOffset > 0)
                  {
                     p->wDirection = ((yOffset > 0) ? kDirEast : kDirNorth);
                  }
                  else
                  {
                     p->wDirection = ((yOffset > 0) ? kDirSouth : kDirWest);
                  }

                  //
                  // Redraw the scene
                  //
                  PAL_UpdatePartyGestures(FALSE);

                  PAL_MakeScene();
                  VIDEO_UpdateScreen(NULL);
               }

               //
               // Execute the script.
               //
               p->wTriggerScript = PAL_RunTriggerScript(p->wTriggerScript, wEventObjectID);

               PAL_ClearKeyState();

               if (gpGlobals->fEnteringScene)
               {
                  //
                  // Don't go further on scene switching
                  //
                  return;
               }
            }
         }
      }
   }

   //
   // Run autoscript for each event objects
   //
   for (wEventObjectID = gpGlobals->g.rgScene[gpGlobals->wNumScene - 1].wEventObjectIndex + 1;
      wEventObjectID <= gpGlobals->g.rgScene[gpGlobals->wNumScene].wEventObjectIndex;
      wEventObjectID++)
   {
      p = &gpGlobals->g.lprgEventObject[wEventObjectID - 1];

      if (p->sState > 0 && p->sVanishTime == 0)
      {
         WORD wScriptEntry = p->wAutoScript;
         if (wScriptEntry != 0)
         {
            p->wAutoScript = PAL_RunAutoScript(wScriptEntry, wEventObjectID);
            if (gpGlobals->fEnteringScene)
            {
               //
               // Don't go further on scene switching
               //
               return;
            }
         }
      }

      //
      // Check if the player is in the way
      //
      if (fTrigger && p->sState >= kObjStateBlocker && p->wSpriteNum != 0 &&
         abs(p->x - PAL_X(gpGlobals->viewport) - PAL_X(gpGlobals->partyoffset)) +
         abs(p->y - PAL_Y(gpGlobals->viewport) - PAL_Y(gpGlobals->partyoffset)) * 2 <= 12)
      {
         //
         // Player is in the way, try to move a step
         //
         wDir = (p->wDirection + 1) % 4;
         for (i = 0; i < 4; i++)
         {
            int              x, y;
            PAL_POS          pos;

            x = PAL_X(gpGlobals->viewport) + PAL_X(gpGlobals->partyoffset);
            y = PAL_Y(gpGlobals->viewport) + PAL_Y(gpGlobals->partyoffset);

            x += ((wDir == kDirWest || wDir == kDirSouth) ? -16 : 16);
            y += ((wDir == kDirWest || wDir == kDirNorth) ? -8 : 8);

            pos = PAL_XY(x, y);

            if (!PAL_CheckObstacleWithRange(pos, TRUE, 0, TRUE))
            {
               //
               // move here
               //
               gpGlobals->viewport = PAL_XY(
                  PAL_X(pos) - PAL_X(gpGlobals->partyoffset),
                  PAL_Y(pos) - PAL_Y(gpGlobals->partyoffset));

               break;
            }

            wDir = (wDir + 1) % 4;
         }
      }
   }

   if (--gpGlobals->wChasespeedChangeCycles == 0)
   {
      gpGlobals->wChaseRange = 1;
   }

   gpGlobals->dwFrameNum++;
}

VOID
PAL_GameUseItem(
   VOID
)
/*++
  Purpose:

    Allow player use an item in the game.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   WORD         wObject;

   while (TRUE)
   {
      wObject = PAL_ItemSelectMenu(NULL, kItemFlagUsable);

      if (wObject == 0)
      {
         return;
      }

      if (!(gpGlobals->g.rgObject[wObject].item.wFlags & kItemFlagApplyToAll))
      {
         //
         // Select the player to use the item on
         //
         WORD     wPlayer = 0;

         while (TRUE)
         {
            wPlayer = PAL_ItemUseMenu(wObject);

            if (wPlayer == MENUITEM_VALUE_CANCELLED)
            {
               break;
            }

            //
            // Run the script
            //
            gpGlobals->g.rgObject[wObject].item.wScriptOnUse =
               PAL_RunTriggerScript(gpGlobals->g.rgObject[wObject].item.wScriptOnUse, wPlayer);

            //
            // Remove the item if the item is consuming and the script succeeded
            //
            if ((gpGlobals->g.rgObject[wObject].item.wFlags & kItemFlagConsuming) &&
               g_fScriptSuccess)
            {
               PAL_AddItemToInventory(wObject, -1);
            }
         }
      }
      else
      {
         //
         // Run the script
         //
         gpGlobals->g.rgObject[wObject].item.wScriptOnUse =
            PAL_RunTriggerScript(gpGlobals->g.rgObject[wObject].item.wScriptOnUse, 0xFFFF);

         //
         // Remove the item if the item is consuming and the script succeeded
         //
         if ((gpGlobals->g.rgObject[wObject].item.wFlags & kItemFlagConsuming) &&
            g_fScriptSuccess)
         {
            PAL_AddItemToInventory(wObject, -1);
         }

         return;
      }
   }
}

VOID
PAL_GameEquipItem(
   VOID
)
/*++
  Purpose:

    Allow player equip an item in the game.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   WORD      wObject;

   while (TRUE)
   {
      wObject = PAL_ItemSelectMenu(NULL, kItemFlagEquipable);

      if (wObject == 0)
      {
         return;
      }

      PAL_EquipItemMenu(wObject);
   }
}

TRIGGERRANGE
PAL_GetSearchTriggerRange(
   VOID
)
/*++
  Purpose:

    Calculate 13 checkpoint coordinates for manual event search.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   int                x, y, xOffset, yOffset, i;
   LPEVENTOBJECT      p;
   TRIGGERRANGE       rgRange;

   //
   // Get the party location
   //
   x = PAL_X(gpGlobals->viewport) + PAL_X(gpGlobals->partyoffset);
   y = PAL_Y(gpGlobals->viewport) + PAL_Y(gpGlobals->partyoffset);

   if (gpGlobals->wPartyDirection == kDirNorth || gpGlobals->wPartyDirection == kDirEast)
   {
      xOffset = 16;
   }
   else
   {
      xOffset = -16;
   }

   if (gpGlobals->wPartyDirection == kDirEast || gpGlobals->wPartyDirection == kDirSouth)
   {
      yOffset = 8;
   }
   else
   {
      yOffset = -8;
   }

   rgRange.rgPos[0] = PAL_XY(x, y);

   for (i = 0; i < 4; i++)
   {
      rgRange.rgPos[i * 3 + 1] = PAL_XY(x + xOffset, y + yOffset);
      rgRange.rgPos[i * 3 + 2] = PAL_XY(x, y + yOffset * 2);
      rgRange.rgPos[i * 3 + 3] = PAL_XY(x + 2 * xOffset, y);
      x += xOffset;
      y += yOffset;
   }

   return rgRange;
}

VOID
PAL_Search(
   VOID
)
/*++
  Purpose:

    Process searching trigger events.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   int                x, y, xOffset, yOffset, dx, dy, dh, ex, ey, eh, i, k, l;
   LPEVENTOBJECT      p;
   TRIGGERRANGE       rgRange;

   rgRange = PAL_GetSearchTriggerRange();

   for (i = 0; i < 13; i++)
   {
      //
      // Convert to map location
      //
      dh = ((PAL_X(rgRange.rgPos[i]) % 32) ? 1 : 0);
      dx = PAL_X(rgRange.rgPos[i]) / 32;
      dy = PAL_Y(rgRange.rgPos[i]) / 16;

      //
      // Loop through all event objects
      //
      for (k = gpGlobals->g.rgScene[gpGlobals->wNumScene - 1].wEventObjectIndex;
         k < gpGlobals->g.rgScene[gpGlobals->wNumScene].wEventObjectIndex; k++)
      {
         p = &(gpGlobals->g.lprgEventObject[k]);
         ex = p->x / 32;
         ey = p->y / 16;
         eh = ((p->x % 32) ? 1 : 0);

         if (p->sState <= 0 || p->wTriggerMode >= kTriggerTouchNear ||
            p->wTriggerMode * 6 - 4 < i || dx != ex || dy != ey || dh != eh)
         {
            continue;
         }

         //
         // Adjust direction/gesture for party members and the event object
         //
         if (p->nSpriteFrames * 4 > p->wCurrentFrameNum)
         {
            p->wCurrentFrameNum = 0; // use standing gesture
            p->wDirection = (gpGlobals->wPartyDirection + 2) % 4; // face the party

            for (l = 0; l <= gpGlobals->wMaxPartyMemberIndex; l++)
            {
               //
               // All party members should face the event object
               //
               gpGlobals->rgParty[l].wFrame = gpGlobals->wPartyDirection * 3;
            }

            //
            // Redraw everything
            //
            PAL_MakeScene();
            VIDEO_UpdateScreen(NULL);
         }

         //
         // Execute the script
         //
         p->wTriggerScript = PAL_RunTriggerScript(p->wTriggerScript, k + 1);

         //
         // Clear inputs and delay for a short time
         //
         UTIL_Delay(50);
         PAL_ClearKeyState();

         return; // don't go further
      }
   }
}

VOID
PAL_StartFrame(
   VOID
)
/*++
  Purpose:

    Starts a video frame. Called once per video frame.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   //
   // Run the game logic of one frame
   //
   PAL_GameUpdate(TRUE);
   if (gpGlobals->fEnteringScene)
   {
      return;
   }

   //
   // Update the positions and gestures of party members
   //
   PAL_UpdateParty();

   //
   // Update the scene
   //
   PAL_MakeScene();
   MM_MarkExplored();
   if (!gpGlobals->fFullscreenMap && gpGlobals->fShowMinimap) MM_DrawMinimap(gpScreen);
   VIDEO_UpdateScreen(NULL);

   if (g_InputState.dwKeyPress & kKeyMenu)
   {
      //
      // Show the in-game menu
      //
      PAL_InGameMenu();
   }
   else if (g_InputState.dwKeyPress & kKeyDefend)
   {
      //
      // Toggle minimap zoom level
      //
      MM_ToggleZoom();
   }
   else if (g_InputState.dwKeyPress & kKeyUseItem)
   {
      //
      // Show the use item menu
      //
      PAL_GameUseItem();
   }
   else if (g_InputState.dwKeyPress & kKeyThrowItem)
   {
      //
      // Show the equipment menu
      //
      PAL_GameEquipItem();
   }
   else if (g_InputState.dwKeyPress & kKeyForce)
   {
      //
      // Show the shop
      //
      PAL_ShopMenu();
   }
   else if (g_InputState.dwKeyPress & kKeyStatus)
   {
      //
      // Show the player status
      //
      PAL_PlayerStatus();
   }
   else if (g_InputState.dwKeyPress & kKeySearch)
   {
      //
      // Process search events
      //
      PAL_Search();
   }
   else if (g_InputState.dwKeyPress & kKeyFlee)
   {
      //
      // Quit Game
      //
      PAL_QuitGame();
   }
}

static inline VOID
PAL_WaitForKeyInternal(
   WORD      wTimeOut,
   BOOL      fAllowAnyKey
)
/*++
  Purpose:

    Wait for any key.

  Parameters:

    [IN]  wTimeOut - the maximum time of the waiting. 0 = wait forever.

    [IN]  fAllowAnyKey - Whether any key are allowed. If no, only KeySearch and KeyMenu allowed.

  Return value:

    None.

--*/
{
   DWORD     dwTimeOut = SDL_GetTicks() + wTimeOut;

   PAL_ClearKeyState();

   while (wTimeOut == 0 || !SDL_TICKS_PASSED(SDL_GetTicks(), dwTimeOut))
   {
      UTIL_Delay(5);

      if (g_InputState.dwKeyPress && fAllowAnyKey
         || g_InputState.dwKeyPress & (kKeySearch | kKeyMenu))
      {
         break;
      }
   }
}

VOID
PAL_WaitForKey(
   WORD      wTimeOut
)
/*++
  Purpose:

    Wait for KeySearch and KeyMenu.

  Parameters:

    [IN]  wTimeOut - the maximum time of the waiting. 0 = wait forever.

  Return value:

    None.

--*/
{
   PAL_WaitForKeyInternal(wTimeOut, FALSE);
}

VOID
PAL_WaitForAnyKey(
   WORD      wTimeOut
)
/*++
  Purpose:

    Wait for any key.

  Parameters:

    [IN]  wTimeOut - the maximum time of the waiting. 0 = wait forever.

  Return value:

    None.

--*/
{
   PAL_WaitForKeyInternal(wTimeOut, TRUE);
}
