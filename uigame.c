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

static BOOL __buymenu_firsttime_render;

static WORD GetSavedTimes(int iSaveSlot)
{
	FILE *fp = UTIL_OpenFileAtPath(gConfig.pszSavePath, PAL_va(0, "%d.rpg", iSaveSlot));
	WORD wSavedTimes = 0;
	if (fp != NULL)
	{
		if (fread(&wSavedTimes, sizeof(WORD), 1, fp) == 1)
			wSavedTimes = SDL_SwapLE16(wSavedTimes);
		else
			wSavedTimes = 0;
		fclose(fp);
	}
	return wSavedTimes;
}

VOID
PAL_DrawOpeningMenuBackground(
   VOID
)
/*++
  Purpose:

    Draw the background of the main menu.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   LPBYTE        buf;

   buf = (LPBYTE)malloc(320 * 200);
   if (buf == NULL)
   {
      return;
   }

   //
   // Read the picture from fbp.mkf.
   //
   PAL_MKFDecompressChunk(buf, 320 * 200, MAINMENU_BACKGROUND_FBPNUM, gpGlobals->f.fpFBP);

   //
   // ...and blit it to the screen buffer.
   //
   PAL_FBPBlitToSurface(buf, gpScreen);
   VIDEO_UpdateScreen(NULL);

   free(buf);
}

INT
PAL_OpeningMenu(
   VOID
)
/*++
  Purpose:

    Show the opening menu.

  Parameters:

    None.

  Return value:

    Which saved slot to load from (1-5). 0 to start a new game.

--*/
{
   WORD          wItemSelected;
   WORD          wDefaultItem     = 0;
   INT           w[2] = { PAL_WordWidth(MAINMENU_LABEL_NEWGAME), PAL_WordWidth(MAINMENU_LABEL_LOADGAME) };

   MENUITEM      rgMainMenuItem[2] = {
      // value   label                     enabled   position
      {  0,      MAINMENU_LABEL_NEWGAME,   TRUE,     PAL_XY(125 - (w[0] > 4 ? (w[0] - 4) * 8 : 0), 95)  },
      {  1,      MAINMENU_LABEL_LOADGAME,  TRUE,     PAL_XY(125 - (w[1] > 4 ? (w[1] - 4) * 8 : 0), 112) }
   };

   //
   // Play the background music
   //
   AUDIO_PlayMusic(RIX_NUM_OPENINGMENU, TRUE, 1);

   //
   // Draw the background
   //
   PAL_DrawOpeningMenuBackground();
   PAL_FadeIn(0, FALSE, 1);

   while (TRUE)
   {
      //
      // Activate the menu
      //
      wItemSelected = PAL_ReadMenu(NULL, rgMainMenuItem, 2, wDefaultItem, MENUITEM_COLOR);

      if (wItemSelected == 0 || wItemSelected == MENUITEM_VALUE_CANCELLED)
      {
         //
         // Start a new game
         //
         wItemSelected = 0;
         break;
      }
      else
      {
         //
         // Load game
         //
         VIDEO_BackupScreen(gpScreen);
         wItemSelected = PAL_SaveSlotMenu(1);
         VIDEO_RestoreScreen(gpScreen);
         VIDEO_UpdateScreen(NULL);
         if (wItemSelected != MENUITEM_VALUE_CANCELLED)
         {
            break;
         }
         wDefaultItem = 0;
      }
   }

   //
   // Fade out the screen and the music
   //
   AUDIO_PlayMusic(0, FALSE, 1);
   PAL_FadeOut(1);

   if (wItemSelected == 0)
   {
      PAL_PlayAVI("3.avi");
   }

   return (INT)wItemSelected;
}

INT
PAL_SaveSlotMenu(
   WORD        wDefaultSlot
)
/*++
  Purpose:

    Show the load game menu.

  Parameters:

    [IN]  wDefaultSlot - default save slot number (1-5).

  Return value:

    Which saved slot to load from (1-5). MENUITEM_VALUE_CANCELLED if cancelled.

--*/
{
   LPBOX           rgpBox[5];
   int             i, w = PAL_WordMaxWidth(LOADMENU_LABEL_SLOT_FIRST, 5);
   int             dx = (w > 4) ? (w - 4) * 16 : 0;
   WORD            wItemSelected;

   MENUITEM        rgMenuItem[5];

   const SDL_Rect  rect = { 195 - dx, 7, 120 + dx, 190 };

   //
   // Create the boxes and create the menu items
   //
   for (i = 0; i < 5; i++)
   {
      // Fix render problem with shadow
      rgpBox[i] = PAL_CreateSingleLineBox(PAL_XY(195 - dx, 7 + 38 * i), 6 + (w > 4 ? w - 4 : 0), FALSE);

      rgMenuItem[i].wValue = i + 1;
      rgMenuItem[i].fEnabled = TRUE;
      rgMenuItem[i].wNumWord = LOADMENU_LABEL_SLOT_FIRST + i;
	  rgMenuItem[i].pos = PAL_XY(210 - dx, 17 + 38 * i);
   }

   //
   // Draw the numbers of saved times
   //
   for (i = 1; i <= 5; i++)
   {
      //
      // Draw the number
      //
      PAL_DrawNumber((UINT)GetSavedTimes(i), 4, PAL_XY(270, 38 * i - 17),
         kNumColorYellow, kNumAlignRight);
   }

   //
   // Activate the menu
   //
   wItemSelected = PAL_ReadMenu(NULL, rgMenuItem, 5, wDefaultSlot - 1, MENUITEM_COLOR);

   //
   // Delete the boxes
   //
   for (i = 0; i < 5; i++)
   {
      PAL_DeleteBox(rgpBox[i]);
   }

   VIDEO_UpdateScreen(&rect);

   return wItemSelected;
}

static
WORD
PAL_SelectionMenu(
	int   nWords,
	int   nDefault,
	WORD  wItems[]
)
/*++
  Purpose:

    Show a common selection box.

  Parameters:

    [IN]  nWords - number of emnu items.
	[IN]  nDefault - index of default item.
	[IN]  wItems - item word array.

  Return value:

    User-selected index.

--*/
{
	LPBOX           rgpBox[4];
	MENUITEM        rgMenuItem[4];
	int             w[4] = {
		(nWords >= 1 && wItems[0]) ? PAL_WordWidth(wItems[0]) : 1,
		(nWords >= 2 && wItems[1]) ? PAL_WordWidth(wItems[1]) : 1,
		(nWords >= 3 && wItems[2]) ? PAL_WordWidth(wItems[2]) : 1,
		(nWords >= 4 && wItems[3]) ? PAL_WordWidth(wItems[3]) : 1 };
	int             dx[4] = { (w[0] - 1) * 16, (w[1] - 1) * 16, (w[2] - 1) * 16, (w[3] - 1) * 16 }, i;
	PAL_POS         pos[4] = { PAL_XY(145, 110), PAL_XY(220 + dx[0], 110), PAL_XY(145, 160), PAL_XY(220 + dx[2], 160) };
	WORD            wReturnValue;

	const SDL_Rect  rect = { 130, 100, 125 + max(dx[0] + dx[1], dx[2] + dx[3]), 100 };

	for (i = 0; i < nWords; i++)
		if (nWords > i && !wItems[i])
			return MENUITEM_VALUE_CANCELLED;

	//
	// Create menu items
	//
	for (i = 0; i < nWords; i++)
	{
		rgMenuItem[i].fEnabled = TRUE;
		rgMenuItem[i].pos = pos[i];
		rgMenuItem[i].wValue = i;
		rgMenuItem[i].wNumWord = wItems[i];
	}

	//
	// Create the boxes
	//
	dx[1] = dx[0]; dx[3] = dx[2]; dx[0] = dx[2] = 0;
	for (i = 0; i < nWords; i++)
	{
		rgpBox[i] = PAL_CreateSingleLineBox(PAL_XY(130 + 75 * (i % 2) + dx[i], 100 + 50 * (i / 2)), w[i] + 1, TRUE);
	}

	//
	// Activate the menu
	//
	wReturnValue = PAL_ReadMenu(NULL, rgMenuItem, nWords, nDefault, MENUITEM_COLOR);

	//
	// Delete the boxes
	//
	for (i = 0; i < nWords; i++)
	{
		PAL_DeleteBox(rgpBox[i]);
	}

	VIDEO_UpdateScreen(&rect);

	return wReturnValue;
}

WORD
PAL_TripleMenu(
   WORD  wThirdWord
)
/*++
  Purpose:

    Show a triple-selection box.

  Parameters:

    None.

  Return value:

    User-selected index.

--*/
{
   WORD wItems[3] = { CONFIRMMENU_LABEL_NO, CONFIRMMENU_LABEL_YES, wThirdWord };
   return PAL_SelectionMenu(3, 0, wItems);
}

BOOL
PAL_ConfirmMenu(
   VOID
)
/*++
  Purpose:

    Show a "Yes or No?" confirm box.

  Parameters:

    None.

  Return value:

    TRUE if user selected Yes, FALSE if selected No.

--*/
{
   WORD wItems[2] = { CONFIRMMENU_LABEL_NO, CONFIRMMENU_LABEL_YES };
   WORD wReturnValue = PAL_SelectionMenu(2, 0, wItems);

   return (wReturnValue == MENUITEM_VALUE_CANCELLED || wReturnValue == 0) ? FALSE : TRUE;
}

BOOL
PAL_SwitchMenu(
   BOOL      fEnabled
)
/*++
  Purpose:

    Show a "Enable/Disable" selection box.

  Parameters:

    [IN]  fEnabled - whether the option is originally enabled or not.

  Return value:

    TRUE if user selected "Enable", FALSE if selected "Disable".

--*/
{
   WORD wItems[2] = { SWITCHMENU_LABEL_DISABLE, SWITCHMENU_LABEL_ENABLE };
   WORD wReturnValue = PAL_SelectionMenu(2, fEnabled ? 1 : 0, wItems);
   return (wReturnValue == MENUITEM_VALUE_CANCELLED) ? fEnabled : ((wReturnValue == 0) ? FALSE : TRUE);
}

#ifndef PAL_CLASSIC

static VOID
PAL_BattleSpeedMenu(
   VOID
)
/*++
  Purpose:

    Show the Battle Speed selection box.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   LPBOX           lpBox;
   WORD            wReturnValue;
   const SDL_Rect  rect = {131, 100, 165, 50};

   MENUITEM        rgMenuItem[5] = {
      { 1,   BATTLESPEEDMENU_LABEL_1,       TRUE,   PAL_XY(145, 110) },
      { 2,   BATTLESPEEDMENU_LABEL_2,       TRUE,   PAL_XY(170, 110) },
      { 3,   BATTLESPEEDMENU_LABEL_3,       TRUE,   PAL_XY(195, 110) },
      { 4,   BATTLESPEEDMENU_LABEL_4,       TRUE,   PAL_XY(220, 110) },
      { 5,   BATTLESPEEDMENU_LABEL_5,       TRUE,   PAL_XY(245, 110) },
   };

   //
   // Create the boxes
   //
   lpBox = PAL_CreateSingleLineBox(PAL_XY(131, 100), 8, TRUE);

   //
   // Activate the menu
   //
   wReturnValue = PAL_ReadMenu(NULL, rgMenuItem, 5, gpGlobals->bBattleSpeed - 1,
      MENUITEM_COLOR);

   //
   // Delete the boxes
   //
   PAL_DeleteBox(lpBox);

   VIDEO_UpdateScreen(&rect);

   if (wReturnValue != MENUITEM_VALUE_CANCELLED)
   {
      gpGlobals->bBattleSpeed = wReturnValue;
   }
}

#endif

LPBOX
PAL_ShowCash(
   DWORD      dwCash
)
/*++
  Purpose:

    Show the cash amount at the top left corner of the screen.

  Parameters:

    [IN]  dwCash - amount of cash.

  Return value:

    pointer to the saved screen part.

--*/
{
   LPBOX     lpBox;

   //
   // Create the box.
   //
   lpBox = PAL_CreateSingleLineBox(PAL_XY(0, 0), 5, TRUE);
   if (lpBox == NULL)
   {
      return NULL;
   }

   //
   // Draw the text label.
   //
   PAL_DrawText(PAL_GetWord(CASH_LABEL), PAL_XY(10, 10), 0, FALSE, FALSE, FALSE);

   //
   // Draw the cash amount.
   //
   PAL_DrawNumber(dwCash, 6, PAL_XY(49, 14), kNumColorYellow, kNumAlignRight);

   return lpBox;
}

static VOID
PAL_SystemMenu_OnItemChange(
   WORD        wCurrentItem
)
/*++
  Purpose:

    Callback function when user selected another item in the system menu.

  Parameters:

    [IN]  wCurrentItem - current selected item.

  Return value:

    None.

--*/
{
   gpGlobals->iCurSystemMenuItem = wCurrentItem - 1;
}

static BOOL
PAL_SystemMenu(
   VOID
)
/*++
  Purpose:

    Show the system menu.

  Parameters:

    None.

  Return value:

    TRUE if user made some operations in the menu, FALSE if user cancelled.

--*/
{
   LPBOX               lpMenuBox;
   WORD                wReturnValue;
   int                 iSlot, i;
   const SDL_Rect      rect = {40, 60, 280, 135};

   //
   // Create menu items
   //
   const MENUITEM      rgSystemMenuItem[] =
   {
      // value  label                        enabled   pos
      { 1,      SYSMENU_LABEL_SAVE,          TRUE,     PAL_XY(53, 72) },
      { 2,      SYSMENU_LABEL_LOAD,          TRUE,     PAL_XY(53, 72 + 18) },
      { 3,      SYSMENU_LABEL_MUSIC,         TRUE,     PAL_XY(53, 72 + 36) },
      { 4,      SYSMENU_LABEL_SOUND,         TRUE,     PAL_XY(53, 72 + 54) },
      { 5,      SYSMENU_LABEL_QUIT,          TRUE,     PAL_XY(53, 72 + 72) },
#if !defined(PAL_CLASSIC)
      { 6,      SYSMENU_LABEL_BATTLEMODE,    TRUE,     PAL_XY(53, 72 + 90) },
#endif
   };
   const int           nSystemMenuItem = sizeof(rgSystemMenuItem) / sizeof(MENUITEM);

   //
   // Create the menu box.
   //
   lpMenuBox = PAL_CreateBox(PAL_XY(40, 60), nSystemMenuItem - 1, PAL_MenuTextMaxWidth(rgSystemMenuItem, nSystemMenuItem) - 1, 0, TRUE);

   //
   // Perform the menu.
   //
   wReturnValue = PAL_ReadMenu(PAL_SystemMenu_OnItemChange, rgSystemMenuItem, nSystemMenuItem, gpGlobals->iCurSystemMenuItem, MENUITEM_COLOR);

   if (wReturnValue == MENUITEM_VALUE_CANCELLED)
   {
      //
      // User cancelled the menu
      //
      PAL_DeleteBox(lpMenuBox);
      VIDEO_UpdateScreen(&rect);
      return FALSE;
   }

   switch (wReturnValue)
   {
   case 1:
      //
      // Save game
      //
      iSlot = PAL_SaveSlotMenu(gpGlobals->bCurrentSaveSlot);

      if (iSlot != MENUITEM_VALUE_CANCELLED)
      {
         WORD wSavedTimes = 0;
         gpGlobals->bCurrentSaveSlot = (BYTE)iSlot;

         for (i = 1; i <= 5; i++)
         {
            WORD curSavedTimes = GetSavedTimes(i);
            if (curSavedTimes > wSavedTimes)
            {
               wSavedTimes = curSavedTimes;
            }
         }
         PAL_SaveGame(iSlot, wSavedTimes + 1);
      }
      break;

   case 2:
      //
      // Load game
      //
      iSlot = PAL_SaveSlotMenu(gpGlobals->bCurrentSaveSlot);
      if (iSlot != MENUITEM_VALUE_CANCELLED)
      {
         AUDIO_PlayMusic(0, FALSE, 1);
         PAL_FadeOut(1);
         PAL_ReloadInNextTick(iSlot);
      }
      break;

   case 3:
      //
      // Music
      //
      AUDIO_EnableMusic(PAL_SwitchMenu(AUDIO_MusicEnabled()));
      if (gConfig.eMIDISynth == SYNTH_NATIVE && gConfig.eMusicType == MUSIC_MIDI)
      {
         AUDIO_PlayMusic(AUDIO_MusicEnabled() ? gpGlobals->wNumMusic : 0, AUDIO_MusicEnabled(), 0);
      }
      break;

   case 4:
      //
      // Sound
      //
      AUDIO_EnableSound(PAL_SwitchMenu(AUDIO_SoundEnabled()));
      break;

   case 5:
      //
      // Quit
      //
      PAL_QuitGame();
      break;

#if !defined(PAL_CLASSIC)
   case 6:
      //
      // Battle Mode
      //
      PAL_BattleSpeedMenu();
      break;
#endif
   }

   PAL_DeleteBox(lpMenuBox);
   return TRUE;
}

VOID
PAL_InGameMagicMenu(
   VOID
)
/*++
  Purpose:

    Show the magic menu.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   MENUITEM         rgMenuItem[MAX_PLAYERS_IN_PARTY];
   int              i, y;
   static WORD      w;
   WORD             wMagic;

   if (gpGlobals->wMaxPartyMemberIndex == 0)
   {
      w = 0;
      goto start_magicmenu;
   }

   //
   // Draw the player info boxes
   //
   y = 45;

   for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
   {
      PAL_PlayerInfoBox(PAL_XY(y, 165), gpGlobals->rgParty[i].wPlayerRole, 100,
         TIMEMETER_COLOR_DEFAULT, TRUE);
      y += 78;
   }

   y = 75;

   //
   // Generate one menu items for each player in the party
   //
   for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
   {
      assert(i <= MAX_PLAYERS_IN_PARTY);

      rgMenuItem[i].wValue = i;
      rgMenuItem[i].wNumWord =
         gpGlobals->g.PlayerRoles.rgwName[gpGlobals->rgParty[i].wPlayerRole];
      rgMenuItem[i].fEnabled =
         (gpGlobals->g.PlayerRoles.rgwHP[gpGlobals->rgParty[i].wPlayerRole] > 0);
      rgMenuItem[i].pos = PAL_XY(48, y);

      y += 18;
   }

   //
   // Draw the box
   //
   PAL_CreateBox(PAL_XY(35, 62), gpGlobals->wMaxPartyMemberIndex, PAL_MenuTextMaxWidth(rgMenuItem, sizeof(rgMenuItem)/sizeof(MENUITEM)) - 1, 0, FALSE);

   w = PAL_ReadMenu(NULL, rgMenuItem, gpGlobals->wMaxPartyMemberIndex + 1, w, MENUITEM_COLOR);

   if (w == MENUITEM_VALUE_CANCELLED)
   {
      return;
   }

start_magicmenu:

   wMagic = 0;

   while (TRUE)
   {
      wMagic = PAL_MagicSelectionMenu(gpGlobals->rgParty[w].wPlayerRole, FALSE, wMagic);
      if (wMagic == 0)
      {
         break;
      }

      VIDEO_BackupScreen(gpScreen);

      if (gpGlobals->g.rgObject[wMagic].magic.wFlags & kMagicFlagApplyToAll)
      {
         gpGlobals->g.rgObject[wMagic].magic.wScriptOnUse =
            PAL_RunTriggerScript(gpGlobals->g.rgObject[wMagic].magic.wScriptOnUse, 0);

         if (g_fScriptSuccess)
         {
            gpGlobals->g.rgObject[wMagic].magic.wScriptOnSuccess =
               PAL_RunTriggerScript(gpGlobals->g.rgObject[wMagic].magic.wScriptOnSuccess, 0);

            if(g_fScriptSuccess)
               gpGlobals->g.PlayerRoles.rgwMP[gpGlobals->rgParty[w].wPlayerRole] -=
               gpGlobals->g.lprgMagic[gpGlobals->g.rgObject[wMagic].magic.wMagicNumber].wCostMP;
         }

         if (gpGlobals->fNeedToFadeIn)
         {
            PAL_FadeIn(gpGlobals->wNumPalette, gpGlobals->fNightPalette, 1);
            gpGlobals->fNeedToFadeIn = FALSE;
         }
      }
      else
      {
         //
         // Need to select which player to use the magic on.
         //
         WORD       wPlayer = 0;
         SDL_Rect   rect;

         while (wPlayer != MENUITEM_VALUE_CANCELLED)
         {
            //
            // Redraw the player info boxes first
            //
            y = 45;

            for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
            {
               PAL_PlayerInfoBox(PAL_XY(y, 165), gpGlobals->rgParty[i].wPlayerRole, 100,
                  TIMEMETER_COLOR_DEFAULT, TRUE);
               y += 78;
            }

            //
            // Draw the cursor on the selected item
            //
            rect.x = 0;
            rect.y = 158;
            rect.w = 320;
            rect.h = 6;

            VIDEO_RestoreScreen(gpScreen);

            PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_CURSOR_UP),
               gpScreen, PAL_XY(75 + 78 * wPlayer, rect.y));

            VIDEO_UpdateScreen(&rect);

            while (TRUE)
            {
               PAL_ClearKeyState();
               PAL_ProcessEvent();

               if (g_InputState.dwKeyPress & kKeyMenu)
               {
                  wPlayer = MENUITEM_VALUE_CANCELLED;
                  break;
               }
               else if (g_InputState.dwKeyPress & kKeySearch)
               {
                  gpGlobals->g.rgObject[wMagic].magic.wScriptOnUse =
                     PAL_RunTriggerScript(gpGlobals->g.rgObject[wMagic].magic.wScriptOnUse,
                        gpGlobals->rgParty[wPlayer].wPlayerRole);

                  if (g_fScriptSuccess)
                  {
                     gpGlobals->g.rgObject[wMagic].magic.wScriptOnSuccess =
                        PAL_RunTriggerScript(gpGlobals->g.rgObject[wMagic].magic.wScriptOnSuccess,
                           gpGlobals->rgParty[wPlayer].wPlayerRole);

                     if (g_fScriptSuccess)
                     {
                        gpGlobals->g.PlayerRoles.rgwMP[gpGlobals->rgParty[w].wPlayerRole] -=
                           gpGlobals->g.lprgMagic[gpGlobals->g.rgObject[wMagic].magic.wMagicNumber].wCostMP;

                        //
                        // Check if we have run out of MP
                        //
                        if (gpGlobals->g.PlayerRoles.rgwMP[gpGlobals->rgParty[w].wPlayerRole] <
                           gpGlobals->g.lprgMagic[gpGlobals->g.rgObject[wMagic].magic.wMagicNumber].wCostMP)
                        {
                           //
                           // Don't go further if run out of MP
                           //
                           wPlayer = MENUITEM_VALUE_CANCELLED;
                        }
                     }
                  }

                  break;
               }
               else if (g_InputState.dwKeyPress & (kKeyLeft | kKeyUp))
               {
                  if (wPlayer > 0)
                  {
                     wPlayer--;
                     break;
                  }
               }
               else if (g_InputState.dwKeyPress & (kKeyRight | kKeyDown))
               {
                  if (wPlayer < gpGlobals->wMaxPartyMemberIndex)
                  {
                     wPlayer++;
                     break;
                  }
               }

               SDL_Delay(1);
            }
         }
      }

      //
      // Redraw the player info boxes
      //
      y = 45;

      for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
      {
         PAL_PlayerInfoBox(PAL_XY(y, 165), gpGlobals->rgParty[i].wPlayerRole, 100,
            TIMEMETER_COLOR_DEFAULT, TRUE);
         y += 78;
      }
   }
}

static VOID
PAL_InventoryMenu(
   VOID
)
/*++
  Purpose:

    Show the inventory menu.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   static WORD      w = 0;

   MENUITEM        rgMenuItem[2] =
   {
      // value  label                     enabled   pos
      { 1,      INVMENU_LABEL_EQUIP,      TRUE,     PAL_XY(43, 73) },
      { 2,      INVMENU_LABEL_USE,        TRUE,     PAL_XY(43, 73 + 18) },
   };

   PAL_CreateBox(PAL_XY(30, 60), 1, PAL_MenuTextMaxWidth(rgMenuItem, sizeof(rgMenuItem)/sizeof(MENUITEM)) - 1, 0, FALSE);

   w = PAL_ReadMenu(NULL, rgMenuItem, 2, w - 1, MENUITEM_COLOR);

   switch (w)
   {
   case 1:
      PAL_GameEquipItem();
      break;

   case 2:
      PAL_GameUseItem();
      break;
   }
}

static VOID
PAL_InGameMenu_OnItemChange(
   WORD        wCurrentItem
)
/*++
  Purpose:

    Callback function when user selected another item in the in-game menu.

  Parameters:

    [IN]  wCurrentItem - current selected item.

  Return value:

    None.

--*/
{
   gpGlobals->iCurMainMenuItem = wCurrentItem - 1;
}

VOID
PAL_InGameMenu(
   VOID
)
/*++
  Purpose:

    Show the in-game main menu.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   LPBOX                lpCashBox, lpMenuBox;
   WORD                 wReturnValue;
   
   // Fix render problem with shadow
   VIDEO_BackupScreen(gpScreen);

   //
   // Create menu items
   //
   MENUITEM        rgMainMenuItem[5] =
   {
      // value  label                      enabled   pos
      { 1,      GAMEMENU_LABEL_STATUS,     TRUE,     PAL_XY(16, 50) },
      { 2,      GAMEMENU_LABEL_MAGIC,      TRUE,     PAL_XY(16, 50 + 18) },
      { 3,      GAMEMENU_LABEL_INVENTORY,  TRUE,     PAL_XY(16, 50 + 36) },
      { 4,      GAMEMENU_LABEL_CHEAT,      TRUE,     PAL_XY(16, 50 + 54) },
      { 5,      GAMEMENU_LABEL_SYSTEM,     TRUE,     PAL_XY(16, 50 + 72) },
   };

   //
   // Display the cash amount.
   //
   lpCashBox = PAL_ShowCash(gpGlobals->dwCash);

   //
   // Create the menu box.
   //
   // Fix render problem with shadow
   lpMenuBox = PAL_CreateBox(PAL_XY(3, 37), 3, PAL_MenuTextMaxWidth(rgMainMenuItem, 5) - 1, 0, FALSE);

   //
   // Process the menu
   //
   while (TRUE)
   {
      wReturnValue = PAL_ReadMenu(PAL_InGameMenu_OnItemChange, rgMainMenuItem, 5,
         gpGlobals->iCurMainMenuItem, MENUITEM_COLOR);

      if (wReturnValue == MENUITEM_VALUE_CANCELLED)
      {
         break;
      }

      switch (wReturnValue)
      {
      case 1:
         //
         // Status
         //
         PAL_PlayerStatus();
         goto out;

      case 2:
         //
         // Magic
         //
         PAL_InGameMagicMenu();
         goto out;

      case 3:
         //
         // Inventory
         //
         PAL_InventoryMenu();
         goto out;

      case 4:
         PAL_CheatMenu();
         goto out;
      case 5:
         //
         // System
         //
         if (PAL_SystemMenu())
         {
            goto out;
         }
         break;
      }
   }

out:
   //
   // Remove the boxes.
   //
   PAL_DeleteBox(lpCashBox);
   PAL_DeleteBox(lpMenuBox);

   // Fix render problem with shadow
   VIDEO_RestoreScreen(gpScreen);
}

VOID
PAL_PlayerStatus(
   VOID
)
/*++
  Purpose:

    Show the player status.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   PAL_LARGE BYTE   bufBackground[320 * 200];
   PAL_LARGE BYTE   bufImage[PAL_RLEBUFSIZE];
   PAL_LARGE BYTE   bufImageBox[50 * 49];
   int              labels0[] = {
      STATUS_LABEL_EXP, STATUS_LABEL_LEVEL, STATUS_LABEL_HP,
      STATUS_LABEL_MP
   };
   int              labels1[] = {
      STATUS_LABEL_EXP_LAYOUT, STATUS_LABEL_LEVEL_LAYOUT, STATUS_LABEL_HP_LAYOUT,
      STATUS_LABEL_MP_LAYOUT
   };
   int              labels[] = {
      STATUS_LABEL_ATTACKPOWER, STATUS_LABEL_MAGICPOWER, STATUS_LABEL_RESISTANCE,
      STATUS_LABEL_DEXTERITY, STATUS_LABEL_FLEERATE
   };
   int              iCurrent;
   int              iPlayerRole;
   int              i, j;
   WORD             w;

   PAL_MKFDecompressChunk(bufBackground, 320 * 200, STATUS_BACKGROUND_FBPNUM, gpGlobals->f.fpFBP);
   iCurrent = 0;

   if (gConfig.fUseCustomScreenLayout)
   {
      for (i = 0; i < 49; i++)
      {
         memcpy(&bufImageBox[i * 50], &bufBackground[(i + 39) * 320 + 247], 50);
      }
      for (i = 0; i < 49; i++)
      {
         memcpy(&bufBackground[(i + 125) * 320 + 81], &bufBackground[(i + 125) * 320 + 81 - 50], 50);
         memcpy(&bufBackground[(i + 141) * 320 + 141], &bufBackground[(i + 141) * 320 + 81 - 50], 50);
         memcpy(&bufBackground[(i + 133) * 320 + 201], &bufBackground[(i + 133) * 320 + 81 - 50], 50);
         memcpy(&bufBackground[(i + 101) * 320 + 251], &bufBackground[(i + 101) * 320 + 81 - 50], 50);
         memcpy(&bufBackground[(i + 39) * 320 + 247], &bufBackground[(i + 39) * 320 + 189 - 50], 50);
         if (i > 0) memcpy(&bufBackground[(i - 1) * 320 + 189], &bufBackground[(i - 1) * 320 + 189 - 50], 50);
      }
      for(i = 0; i < MAX_PLAYER_EQUIPMENTS; i++)
      {
         short x = PAL_X(gConfig.ScreenLayout.RoleEquipImageBoxes[i]);
         short y = PAL_Y(gConfig.ScreenLayout.RoleEquipImageBoxes[i]);
         short sx = (x < 0) ? -x : 0, sy = (y < 0) ? -y : 0, d = (x > 270) ? x - 270 : 0;
         if (sx >= 50 || sy >= 49 || x >= 320 || y >= 200) continue;
         for (; sy < 49 && y + sy < 200; sy++)
         {
            memcpy(&bufBackground[(y + sy) * 320 + x + sx], &bufImageBox[sy * 50 + sx], 50 - sx - d);
         }
      }
   }

   while (iCurrent >= 0 && iCurrent <= gpGlobals->wMaxPartyMemberIndex)
   {
      iPlayerRole = gpGlobals->rgParty[iCurrent].wPlayerRole;

      //
      // Draw the background image
      //
      PAL_FBPBlitToSurface(bufBackground, gpScreen);

      //
      // Draw the image of player role
      //
      if (PAL_MKFReadChunk(bufImage, PAL_RLEBUFSIZE, gpGlobals->g.PlayerRoles.rgwAvatar[iPlayerRole], gpGlobals->f.fpRGM) > 0)
      {
         PAL_RLEBlitToSurface(bufImage, gpScreen, gConfig.ScreenLayout.RoleImage);
      }

      //
      // Draw the equipments
      //
      for (i = 0; i < MAX_PLAYER_EQUIPMENTS; i++)
      {
         int offset;

         w = gpGlobals->g.PlayerRoles.rgwEquipment[i][iPlayerRole];

         if (w == 0)
         {
            continue;
         }

         //
         // Draw the image
         //
         if (PAL_MKFReadChunk(bufImage, PAL_RLEBUFSIZE,
            gpGlobals->g.rgObject[w].item.wBitmap, gpGlobals->f.fpBALL) > 0)
         {
            PAL_RLEBlitToSurface(bufImage, gpScreen,
               PAL_XY_OFFSET(gConfig.ScreenLayout.RoleEquipImageBoxes[i], 1, 1));
         }

         //
         // Draw the text label
         //
         offset = PAL_WordWidth(w) * 16;
         if (PAL_X(gConfig.ScreenLayout.RoleEquipNames[i]) + offset > 320)
         {
            offset = 320 - PAL_X(gConfig.ScreenLayout.RoleEquipNames[i]) - offset;
         }
         else
         {
            offset = 0;
         }
         int index = &gConfig.ScreenLayout.RoleEquipNames[i] - gConfig.ScreenLayoutArray;
         BOOL fShadow = (gConfig.ScreenLayoutFlag[index] & DISABLE_SHADOW) ? FALSE : TRUE;
         BOOL fUse8x8Font = (gConfig.ScreenLayoutFlag[index] & USE_8x8_FONT) ? TRUE : FALSE;
         PAL_DrawText(PAL_GetWord(w), PAL_XY_OFFSET(gConfig.ScreenLayout.RoleEquipNames[i], offset, 0), STATUS_COLOR_EQUIPMENT, fShadow, FALSE, fUse8x8Font);
      }

      //
      // Draw the text labels
      //
      for (i = 0; i < sizeof(labels0) / sizeof(int); i++)
      {
         int index = labels1[i];
         BOOL fShadow = (gConfig.ScreenLayoutFlag[index] & DISABLE_SHADOW) ? FALSE : TRUE;
         BOOL fUse8x8Font = (gConfig.ScreenLayoutFlag[index] & USE_8x8_FONT) ? TRUE : FALSE;
         PAL_DrawText(PAL_GetWord(labels0[i]), *(&gConfig.ScreenLayout.RoleExpLabel+i), MENUITEM_COLOR, fShadow, FALSE, fUse8x8Font);
      }
      for (i = 0; i < sizeof(labels) / sizeof(int); i++)
      {
         int index = &gConfig.ScreenLayout.RoleStatusLabels[i] - gConfig.ScreenLayoutArray;
         BOOL fShadow = (gConfig.ScreenLayoutFlag[index] & DISABLE_SHADOW) ? FALSE : TRUE;
         BOOL fUse8x8Font = (gConfig.ScreenLayoutFlag[index] & USE_8x8_FONT) ? TRUE : FALSE;
         PAL_DrawText(PAL_GetWord(labels[i]), gConfig.ScreenLayout.RoleStatusLabels[i], MENUITEM_COLOR, fShadow, FALSE, fUse8x8Font);
      }

      PAL_DrawText(PAL_GetWord(gpGlobals->g.PlayerRoles.rgwName[iPlayerRole]),
         gConfig.ScreenLayout.RoleName, MENUITEM_COLOR_CONFIRMED, TRUE, FALSE, FALSE);

      //
      // Draw the stats
      //
      if (gConfig.ScreenLayout.RoleExpSlash != 0)
	  {
         PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH), gpScreen, gConfig.ScreenLayout.RoleExpSlash);
      }
      if (gConfig.ScreenLayout.RoleHPSlash != 0)
	  {
         PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH), gpScreen, gConfig.ScreenLayout.RoleHPSlash);
      }
      if (gConfig.ScreenLayout.RoleMPSlash != 0)
	  {
         PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH), gpScreen, gConfig.ScreenLayout.RoleMPSlash);
      }

      PAL_DrawNumber(gpGlobals->Exp.rgPrimaryExp[iPlayerRole].wExp, 5,
         gConfig.ScreenLayout.RoleCurrExp, kNumColorYellow, kNumAlignRight);
      PAL_DrawNumber(gpGlobals->g.rgLevelUpExp[gpGlobals->g.PlayerRoles.rgwLevel[iPlayerRole]], 5,
         gConfig.ScreenLayout.RoleNextExp, kNumColorCyan, kNumAlignRight);
      PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwLevel[iPlayerRole], 2,
         gConfig.ScreenLayout.RoleLevel, kNumColorYellow, kNumAlignRight);
      PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwHP[iPlayerRole], 4,
         gConfig.ScreenLayout.RoleCurHP, kNumColorYellow, kNumAlignRight);
      PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwMaxHP[iPlayerRole], 4,
         gConfig.ScreenLayout.RoleMaxHP, kNumColorBlue, kNumAlignRight);
      PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwMP[iPlayerRole], 4,
         gConfig.ScreenLayout.RoleCurMP, kNumColorYellow, kNumAlignRight);
      PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwMaxMP[iPlayerRole], 4,
         gConfig.ScreenLayout.RoleMaxMP, kNumColorBlue, kNumAlignRight);

      PAL_DrawNumber(PAL_GetPlayerAttackStrength(iPlayerRole), 4,
         gConfig.ScreenLayout.RoleStatusValues[0], kNumColorYellow, kNumAlignRight);
      PAL_DrawNumber(PAL_GetPlayerMagicStrength(iPlayerRole), 4,
         gConfig.ScreenLayout.RoleStatusValues[1], kNumColorYellow, kNumAlignRight);
      PAL_DrawNumber(PAL_GetPlayerDefense(iPlayerRole), 4,
         gConfig.ScreenLayout.RoleStatusValues[2], kNumColorYellow, kNumAlignRight);
      PAL_DrawNumber(PAL_GetPlayerDexterity(iPlayerRole), 4,
         gConfig.ScreenLayout.RoleStatusValues[3], kNumColorYellow, kNumAlignRight);
      PAL_DrawNumber(PAL_GetPlayerFleeRate(iPlayerRole), 4,
         gConfig.ScreenLayout.RoleStatusValues[4], kNumColorYellow, kNumAlignRight);

      //
      // Draw all poisons
      //
      for (i = j = 0; i < MAX_POISONS; i++)
      {
         w = gpGlobals->rgPoisonStatus[i][iCurrent].wPoisonID;

         if (w != 0 && gpGlobals->g.rgObject[w].poison.wPoisonLevel <= 3)
         {
            PAL_DrawText(PAL_GetWord(w), gConfig.ScreenLayout.RolePoisonNames[j++], (BYTE)(gpGlobals->g.rgObject[w].poison.wColor + 10), TRUE, FALSE, FALSE);
         }
      }

      //
      // Update the screen
      //
      VIDEO_UpdateScreen(NULL);

      //
      // Wait for input
      //
      PAL_ClearKeyState();

      while (TRUE)
      {
         UTIL_Delay(1);

         if (g_InputState.dwKeyPress & kKeyMenu)
         {
            iCurrent = -1;
            break;
         }
         else if (g_InputState.dwKeyPress & (kKeyLeft | kKeyUp))
         {
            iCurrent--;
            break;
         }
         else if (g_InputState.dwKeyPress & (kKeyRight | kKeyDown | kKeySearch))
         {
            iCurrent++;
            break;
         }
      }
   }
}

WORD
PAL_ItemUseMenu(
   WORD           wItemToUse
)
/*++
  Purpose:

    Show the use item menu.

  Parameters:

    [IN]  wItemToUse - the object ID of the item to use.

  Return value:

    The selected player to use the item onto.
    MENUITEM_VALUE_CANCELLED if user cancelled.

--*/
{
   BYTE           bColor, bSelectedColor;
   PAL_LARGE BYTE bufImage[2048];
   DWORD          dwColorChangeTime;
   static SHORT   sSelectedPlayer = 0;
   SDL_Rect       rect = {110, 2, 200, 180};
   int            i;

   bSelectedColor = MENUITEM_COLOR_SELECTED_FIRST;
   dwColorChangeTime = 0;

   while (TRUE)
   {
      if (sSelectedPlayer > gpGlobals->wMaxPartyMemberIndex)
      {
         sSelectedPlayer = 0;
      }

      //
      // Draw the box
      //
      PAL_CreateBox(PAL_XY(110, 2), 7, 9, 0, FALSE);

      //
      // Draw the stats of the selected player
      //
      PAL_DrawText(PAL_GetWord(STATUS_LABEL_LEVEL), PAL_XY(200, 16),
         ITEMUSEMENU_COLOR_STATLABEL, TRUE, FALSE, FALSE);
      PAL_DrawText(PAL_GetWord(STATUS_LABEL_HP), PAL_XY(200, 34),
         ITEMUSEMENU_COLOR_STATLABEL, TRUE, FALSE, FALSE);
      PAL_DrawText(PAL_GetWord(STATUS_LABEL_MP), PAL_XY(200, 52),
         ITEMUSEMENU_COLOR_STATLABEL, TRUE, FALSE, FALSE);
      PAL_DrawText(PAL_GetWord(STATUS_LABEL_ATTACKPOWER), PAL_XY(200, 70),
         ITEMUSEMENU_COLOR_STATLABEL, TRUE, FALSE, FALSE);
      PAL_DrawText(PAL_GetWord(STATUS_LABEL_MAGICPOWER), PAL_XY(200, 88),
         ITEMUSEMENU_COLOR_STATLABEL, TRUE, FALSE, FALSE);
      PAL_DrawText(PAL_GetWord(STATUS_LABEL_RESISTANCE), PAL_XY(200, 106),
         ITEMUSEMENU_COLOR_STATLABEL, TRUE, FALSE, FALSE);
      PAL_DrawText(PAL_GetWord(STATUS_LABEL_DEXTERITY), PAL_XY(200, 124),
         ITEMUSEMENU_COLOR_STATLABEL, TRUE, FALSE, FALSE);
      PAL_DrawText(PAL_GetWord(STATUS_LABEL_FLEERATE), PAL_XY(200, 142),
         ITEMUSEMENU_COLOR_STATLABEL, TRUE, FALSE, FALSE);

      i = gpGlobals->rgParty[sSelectedPlayer].wPlayerRole;

      PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwLevel[i], 4, PAL_XY(240, 20),
         kNumColorYellow, kNumAlignRight);

      PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH), gpScreen,
         PAL_XY(263, 38));
      PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwMaxHP[i], 4,
         PAL_XY(261, 40), kNumColorBlue, kNumAlignRight);
      PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwHP[i], 4,
         PAL_XY(240, 37), kNumColorYellow, kNumAlignRight);

      PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH), gpScreen,
         PAL_XY(263, 56));
      PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwMaxMP[i], 4,
         PAL_XY(261, 58), kNumColorBlue, kNumAlignRight);
      PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwMP[i], 4,
         PAL_XY(240, 55), kNumColorYellow, kNumAlignRight);

      PAL_DrawNumber(PAL_GetPlayerAttackStrength(i), 4, PAL_XY(240, 74),
         kNumColorYellow, kNumAlignRight);
      PAL_DrawNumber(PAL_GetPlayerMagicStrength(i), 4, PAL_XY(240, 92),
         kNumColorYellow, kNumAlignRight);
      PAL_DrawNumber(PAL_GetPlayerDefense(i), 4, PAL_XY(240, 110),
         kNumColorYellow, kNumAlignRight);
      PAL_DrawNumber(PAL_GetPlayerDexterity(i), 4, PAL_XY(240, 128),
         kNumColorYellow, kNumAlignRight);
      PAL_DrawNumber(PAL_GetPlayerFleeRate(i), 4, PAL_XY(240, 146),
         kNumColorYellow, kNumAlignRight);

      //
      // Draw the names of the players in the party
      //
      for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
      {
         if (i == sSelectedPlayer)
         {
            bColor = bSelectedColor;
         }
         else
         {
            bColor = MENUITEM_COLOR;
         }

         PAL_DrawText(PAL_GetWord(gpGlobals->g.PlayerRoles.rgwName[gpGlobals->rgParty[i].wPlayerRole]),
            PAL_XY(125, 16 + 20 * i), bColor, TRUE, FALSE, FALSE);
      }

      PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_ITEMBOX), gpScreen,
         PAL_XY(120, 80));

      i = PAL_GetItemAmount(wItemToUse);

      if (i > 0)
      {
         //
         // Draw the picture of the item
         //
         if (PAL_MKFReadChunk(bufImage, 2048,
            gpGlobals->g.rgObject[wItemToUse].item.wBitmap, gpGlobals->f.fpBALL) > 0)
         {
            PAL_RLEBlitToSurface(bufImage, gpScreen, PAL_XY(127, 88));
         }

         //
         // Draw the amount and label of the item
         //
         PAL_DrawText(PAL_GetWord(wItemToUse), PAL_XY(116, 143), STATUS_COLOR_EQUIPMENT, TRUE, FALSE, FALSE);
         PAL_DrawNumber(i, 2, PAL_XY(170, 133), kNumColorCyan, kNumAlignRight);
      }

      //
      // Update the screen area
      //
      VIDEO_UpdateScreen(&rect);

      //
      // Wait for key
      //
      PAL_ClearKeyState();

      while (TRUE)
      {
         //
         // See if we should change the highlight color
         //
         if (SDL_TICKS_PASSED(SDL_GetTicks(), dwColorChangeTime))
         {
            if ((WORD)bSelectedColor + 1 >=
               (WORD)MENUITEM_COLOR_SELECTED_FIRST + MENUITEM_COLOR_SELECTED_TOTALNUM)
            {
               bSelectedColor = MENUITEM_COLOR_SELECTED_FIRST;
            }
            else
            {
               bSelectedColor++;
            }

            dwColorChangeTime = SDL_GetTicks() + (600 / MENUITEM_COLOR_SELECTED_TOTALNUM);

            //
            // Redraw the selected item.
            //
            PAL_DrawText(
               PAL_GetWord(gpGlobals->g.PlayerRoles.rgwName[gpGlobals->rgParty[sSelectedPlayer].wPlayerRole]),
               PAL_XY(125, 16 + 20 * sSelectedPlayer), bSelectedColor, FALSE, TRUE, FALSE);
         }

         PAL_ProcessEvent();

         if (g_InputState.dwKeyPress != 0)
         {
            break;
         }

         SDL_Delay(1);
      }

      if (i <= 0)
      {
         return MENUITEM_VALUE_CANCELLED;
      }

      if (g_InputState.dwKeyPress & (kKeyUp | kKeyLeft))
      {
         sSelectedPlayer--;
         if (sSelectedPlayer < 0)
         {
            sSelectedPlayer = gpGlobals->wMaxPartyMemberIndex;
         }
      }
      else if (g_InputState.dwKeyPress & (kKeyDown | kKeyRight))
      {
         sSelectedPlayer++;
         if (sSelectedPlayer > gpGlobals->wMaxPartyMemberIndex)
         {
            sSelectedPlayer = 0;
         }
      }
      else if (g_InputState.dwKeyPress & kKeyMenu)
      {
         break;
      }
      else if (g_InputState.dwKeyPress & kKeySearch)
      {
         return gpGlobals->rgParty[sSelectedPlayer].wPlayerRole;
      }
   }

   return MENUITEM_VALUE_CANCELLED;
}

static VOID
PAL_BuyMenu_OnItemChange(
   WORD           wCurrentItem
)
/*++
  Purpose:

    Callback function which is called when player selected another item
    in the buy menu.

  Parameters:

    [IN]  wCurrentItem - current item on the menu, indicates the object ID of
                         the currently selected item.

  Return value:

    None.

--*/
{
   const SDL_Rect      rect = {20, 8, 300, 175};
   int                 i, j, n, iPlayerID, x, y;
   PAL_LARGE BYTE      bufImage[2048];

   //
   // Prepare item bakcground box pos
   //
   x = 40, y = 8;

   if( __buymenu_firsttime_render )
      PAL_RLEBlitToSurfaceWithShadow(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_ITEMBOX), gpScreen, PAL_XY(x + 6, y + 6), TRUE);
   //
   // Draw the picture of current selected item
   //
   PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_ITEMBOX), gpScreen,
      PAL_XY(x, y));

   //
   // Prepare item pos
   //
   x = 48, y = 15;

   if (PAL_MKFReadChunk(bufImage, 2048,
      gpGlobals->g.rgObject[wCurrentItem].item.wBitmap, gpGlobals->f.fpBALL) > 0)
   {
      PAL_RLEBlitToSurface(bufImage, gpScreen, PAL_XY(x, y));
   }

   //
   // See how many of this item we have in the inventory
   //
   n = 0;

   for (i = 0; i < MAX_INVENTORY; i++)
   {
      if (gpGlobals->rgInventory[i].wItem == 0)
      {
         break;
      }
      else if (gpGlobals->rgInventory[i].wItem == wCurrentItem)
      {
         n = gpGlobals->rgInventory[i].nAmount;
         break;
      }
   }

   for (i = 0; i < MAX_PLAYER_EQUIPMENTS; i++)
   {
      for (j = 0; j <= gpGlobals->wMaxPartyMemberIndex; j++)
      {
         iPlayerID = gpGlobals->rgParty[j].wPlayerRole;

         if (gpGlobals->g.PlayerRoles.rgwEquipment[i][iPlayerID] == wCurrentItem) n++;
      }
   }

   //
   // Prepare inventory quantities pos
   //
   x = 20, y = 100;

   if( __buymenu_firsttime_render )
      PAL_CreateSingleLineBoxWithShadow(PAL_XY(x, y), 5, FALSE, 6);
   else
   //
   // Draw the amount of this item in the inventory
   //
   PAL_CreateSingleLineBoxWithShadow(PAL_XY(x, y), 5, FALSE, 0);
   PAL_DrawText(PAL_GetWord(BUYMENU_LABEL_CURRENT), PAL_XY(x + 10, y + 10), 0, FALSE, FALSE, FALSE);
   PAL_DrawNumber(n, 6, PAL_XY(x + 49, y + 15), kNumColorYellow, kNumAlignRight);

   //
   // Prepare inventory quantities pos
   //
   x = 20, y = 141;

   if( __buymenu_firsttime_render )
      PAL_CreateSingleLineBoxWithShadow(PAL_XY(x, y), 5, FALSE, 6);
   else
   //
   // Draw the cash amount
   //
   PAL_CreateSingleLineBoxWithShadow(PAL_XY(x, y), 5, FALSE, 0);
   PAL_DrawText(PAL_GetWord(CASH_LABEL), PAL_XY(x + 10, y + 10), 0, FALSE, FALSE, FALSE);
   PAL_DrawNumber(gpGlobals->dwCash, 6, PAL_XY(x + 49, y + 15), kNumColorYellow, kNumAlignRight);

   VIDEO_UpdateScreen(&rect);
   
   __buymenu_firsttime_render = FALSE;
}


static void CheatExec(LPCWSTR msg) {
   SDL_Rect fr; fr.x=0; fr.y=88; fr.w=320; fr.h=24;
   SDL_FillRect(gpScreen, &fr, 0x2C);
   PAL_DrawText(msg, PAL_XY(110, 92), 0x4F, TRUE, FALSE, FALSE);
   VIDEO_UpdateScreen(NULL);
   AUDIO_PlaySound(43);
   UTIL_Delay(500);
}

//
// Portable shop system
//
#define MAX_SHOP_ITEMS 200

typedef struct {
   WORD id;
   WORD price;
} SHOPITEM;

static int _shopSort(const void *a, const void *b) {
   return ((const SHOPITEM *)a)->price - ((const SHOPITEM *)b)->price;
}

VOID
PAL_ShopMenu(
   VOID
)
{
   // Build item list: all objects with price > 0 and a valid bitmap
   SHOPITEM items[MAX_SHOP_ITEMS];
   int count = 0;
   for (int i = 91; i < MAX_OBJECTS && count < MAX_SHOP_ITEMS; i++) {
      WORD price = gpGlobals->g.rgObject[i].item.wPrice;
      WORD bmp = gpGlobals->g.rgObject[i].item.wBitmap;
      if (price > 0 && bmp != 0) {
         items[count].id = (WORD)i;
         items[count].price = price;
         count++;
      }
   }

   // Sort by price (cheapest first)
   qsort(items, count, sizeof(SHOPITEM), _shopSort);

   // Shop interface
   int sel = 0, pg = 0, pp = 7, pgc = (count + pp - 1) / pp;
   SDL_Surface *saved = SDL_CreateRGBSurface(SDL_SWSURFACE, 320, 200, 8, 0, 0, 0, 0);
   SDL_BlitSurface(gpScreen, NULL, saved, NULL);
   BOOL done = FALSE;
   WCHAR wb[64];

   while (!done) {
      // Draw background
      { SDL_Rect _bg; _bg.x=10; _bg.y=10; _bg.w=300; _bg.h=180; SDL_FillRect(gpScreen, &_bg, 0x00);
      _bg.x=11; _bg.y=11; _bg.w=298; _bg.h=178; SDL_FillRect(gpScreen, &_bg, 0x11); }
      SDL_Rect tr; tr.x=10; tr.y=10; tr.w=300; tr.h=14;
      SDL_FillRect(gpScreen, &tr, 0x2C);

      // Title
      PAL_swprintf(wb, 64, L"\u5546 \u57CE  %d/%d  \u91D1\u94B1:%d", pg+1, pgc, gpGlobals->dwCash);
      PAL_DrawText(wb, PAL_XY(20, 11), 0x4F, TRUE, FALSE, FALSE);
      PAL_DrawText(L"#", PAL_XY(15, 26), 0x1B, TRUE, FALSE, TRUE);

      // Items
      int st = pg * pp, ed = st + pp;
      if (ed > count) ed = count;
      for (int i = st; i < ed; i++) {
         int j = i - st, y = 28 + j * 20;
         if (i == sel) {
            SDL_Rect sr; sr.x = 14; sr.y = y - 2; sr.w = 290; sr.h = 18;
            SDL_FillRect(gpScreen, &sr, 0x1B);
         }
         // Item name
         LPCWSTR name = PAL_GetWord(items[i].id);
         if (!name || !name[0]) name = L"?";
         PAL_DrawText(name, PAL_XY(28, y), (i == sel) ? 0x4F : 0x1C, TRUE, FALSE, FALSE);
         // Item price
         PAL_swprintf(wb, 64, L"%d", items[i].price);
         PAL_DrawText(wb, PAL_XY(250, y), (gpGlobals->dwCash >= items[i].price) ? 0x2C : 0x4A, TRUE, FALSE, TRUE);
      }

      // Help text
      PAL_DrawText(L"[A]\u8D2D\u4E70 [B]\u8FD4\u56DE [\u2191\u2193]\u9009\u62E9 [\u2190\u2192]\u9875", PAL_XY(16, 170), 0x1B, TRUE, FALSE, TRUE);

      VIDEO_UpdateScreen(NULL);
      PAL_ClearKeyState();
      UTIL_Delay(80);

      // Input handling
      if (g_InputState.dir == kDirNorth && sel > 0) sel--;
      else if (g_InputState.dir == kDirSouth && sel < count - 1) sel++;
      else if (g_InputState.dir == kDirWest && pg > 0) { pg--; sel = pg * pp; }
      else if (g_InputState.dir == kDirEast && pg < pgc - 1) { pg++; sel = pg * pp; }
      else if (g_InputState.dwKeyPress & kKeyMenu) { done = TRUE; }
      else if (g_InputState.dwKeyPress & kKeySearch) {
         WORD id = items[sel].id;
         WORD price = items[sel].price;
         if (gpGlobals->dwCash >= price) {
            gpGlobals->dwCash -= price;
            PAL_AddItemToInventory(id, 1);
            PAL_swprintf(wb, 64, L"\u8D2D\u4E70\u6210\u529F! %ls", PAL_GetWord(id));
            CheatExec(wb);
         } else {
            CheatExec(L"\u91D1\u94B1\u4E0D\u8DB3!");
         }
      }
      pg = sel / pp;
   }

   SDL_BlitSurface(saved, NULL, gpScreen, NULL);
   SDL_FreeSurface(saved);
   PAL_ClearKeyState();
}

//
// Boss Rush system
//
#define BOSSRUSH_COUNT 12
static const WORD _bossTeams[BOSSRUSH_COUNT] = {1,3,5,8,10,13,17,20,25,30,35,40};
static int _brLevel = 0; // 0=not started, 1..BOSSRUSH_COUNT=progress, BOSSRUSH_COUNT=completed

static void BossRushWait(BOOL playSound) {
   g_InputState.dir = kDirUnknown;
   PAL_ClearKeyState();
   if (playSound) AUDIO_PlaySound(43);
   int _tw = 0;
   while (1) {
      PAL_ClearKeyState();
      UTIL_Delay(50);
      if (g_InputState.dwKeyPress & (kKeyMenu | kKeySearch)) break;
      if (++_tw > 60) break; // auto-continue after ~3 seconds
   }
   PAL_ClearKeyState();
}

static void BossRushDrawBox(int x, int y, int w, int h, BYTE color) {
   SDL_Rect r; r.x=x; r.y=y; r.w=w; r.h=h;
   SDL_FillRect(gpScreen, &r, color);
   r.x=x+1; r.y=y+1; r.w=w-2; r.h=h-2;
   SDL_FillRect(gpScreen, &r, 0x11);
}

static int BossRushGetBossName(WORD teamID, WCHAR *buf, int bufSize) {
   // Get the name of the first enemy in this team
   WORD w = gpGlobals->g.lprgEnemyTeam[teamID].rgwEnemy[0];
   if (w != 0 && w != 0xFFFF) {
      LPCWSTR name = PAL_GetWord(w);
      if (name && name[0]) {
         return PAL_swprintf(buf, bufSize, L"%ls", name);
      }
   }
   return PAL_swprintf(buf, bufSize, L"BOSS #%d", teamID);
}

static int BossRushGetBossSubtitle(int bossIdx, WCHAR *buf, int bufSize) {
   // Flavor subtitles for each boss
   static const LPCWSTR _subs[BOSSRUSH_COUNT] = {
      L"初入江湖的第一战",       // 1
      L"蛇穴之中的凶险",         // 3
      L"妖狐的魅惑陷阱",         // 5
      L"将军冢内的亡魂",         // 8
      L"毒物环绕的绝境",         // 10
      L"盘丝洞中的阴谋",         // 13
      L"苗疆高手拦路",           // 17
      L"道观深处的强敌",         // 20
      L"水底魔兽的突袭",         // 25
      L"拜月教主的野心",         // 30
      L"锁妖塔的镇守者",         // 35
      L"最终的宿命对决"          // 40
   };
   if (bossIdx >= 0 && bossIdx < BOSSRUSH_COUNT) {
      return PAL_swprintf(buf, bufSize, L"%ls", _subs[bossIdx]);
   }
   return 0;
}

static void BossRushIntro(int bossIdx) {
   // Dramatic intro screen before each boss
   WCHAR nameBuf[64], subBuf[64];
   BossRushGetBossName(_bossTeams[bossIdx], nameBuf, 64);
   BossRushGetBossSubtitle(bossIdx, subBuf, 64);

   // Backup screen
   SDL_Surface *saved = SDL_CreateRGBSurface(SDL_SWSURFACE, 320, 200, 8, 0, 0, 0, 0);
   SDL_BlitSurface(gpScreen, NULL, saved, NULL);

   // Animate: fade in dark
   for (int a = 0; a < 3; a++) {
      BossRushDrawBox(0, 0, 320, 200, 0x00);
      VIDEO_UpdateScreen(NULL);
      UTIL_Delay(100);
   }

   // Draw dramatic intro
   BossRushDrawBox(30, 40, 260, 120, 0x00);

   WCHAR fNum[16];
   PAL_swprintf(fNum, 16, L"\u7B2C %d / %d \u5C42", bossIdx + 1, BOSSRUSH_COUNT);
   PAL_DrawText(fNum, PAL_XY(90, 55), 0x1B, TRUE, FALSE, FALSE);

   // Boss name (big, bright)
   PAL_DrawText(nameBuf, PAL_XY(100, 75), 0x2C, TRUE, FALSE, FALSE);

   // Subtitle
   PAL_DrawText(subBuf, PAL_XY(80, 98), 0x1C, TRUE, FALSE, FALSE);

   // Hint
   PAL_DrawText(L"[A] \u7EE7\u7EED", PAL_XY(130, 135), 0x4F, TRUE, FALSE, TRUE);

   VIDEO_UpdateScreen(NULL);
   BossRushWait(TRUE);

   // Restore screen
   SDL_BlitSurface(saved, NULL, gpScreen, NULL);
   SDL_FreeSurface(saved);
}

static void BossRushReward(int bossIdx) {
   // Give reward for clearing a boss
   // Gold scales with boss number
   int baseGold = 500 + bossIdx * 300;
   int bonus = RandomLong(0, 300);
   DWORD goldReward = baseGold + bonus;
   gpGlobals->dwCash += goldReward;

   WCHAR msg[64];
   // Try to give a random consumable item
   int itemCount = 0;
   WORD rewardItem = 0;
   for (int ii = 1; ii < MAX_OBJECTS; ii++) {
      WORD flags = gpGlobals->g.rgObject[ii].item.wFlags;
      if (flags & kItemFlagConsuming) {
         itemCount++;
         if (RandomLong(1, itemCount) == 1) rewardItem = (WORD)ii;
      }
   }
   if (rewardItem) {
      // Add item to inventory
      int slot = -1;
      for (int si = 0; si < MAX_INVENTORY; si++) {
         if (gpGlobals->rgInventory[si].wItem == rewardItem) {
            gpGlobals->rgInventory[si].nAmount++;
            if (gpGlobals->rgInventory[si].nAmount > 999) gpGlobals->rgInventory[si].nAmount = 999;
            slot = si; break;
         }
         if (gpGlobals->rgInventory[si].wItem == 0 && slot < 0) slot = si;
      }
      if (slot >= 0 && gpGlobals->rgInventory[slot].wItem != rewardItem) {
         gpGlobals->rgInventory[slot].wItem = rewardItem;
         gpGlobals->rgInventory[slot].nAmount = 1;
         gpGlobals->rgInventory[slot].nAmountInUse = 0;
      }
      LPCWSTR itemName = PAL_GetWord(rewardItem);
      if (itemName && itemName[0]) {
         PAL_swprintf(msg, 64, L"\u80DC\u5229! +%d\u91D1\u94B1 + %ls", goldReward, itemName);
      } else {
         PAL_swprintf(msg, 64, L"\u80DC\u5229! +%d\u91D1\u94B1", goldReward);
      }
   } else {
      PAL_swprintf(msg, 64, L"\u80DC\u5229! +%d\u91D1\u94B1", goldReward);
   }

   CheatExec(msg);
}

static void BossRushVictoryCelebration(void) {
   // Victory animation - flash effect
   for (int f = 0; f < 6; f++) {
      SDL_Rect fr; fr.x=0; fr.y=0; fr.w=320; fr.h=200;
      SDL_FillRect(gpScreen, &fr, (f % 2 == 0) ? 0x2C : 0x1B);
      VIDEO_UpdateScreen(NULL);
      UTIL_Delay(150);
   }

   // Show big congratulations
   BossRushDrawBox(20, 30, 280, 140, 0x00);
   WCHAR wb[48];
   PAL_swprintf(wb, 48, L"\u7B2C %d / %d \u5C42 \u5168\u90E8\u6253\u901A!", BOSSRUSH_COUNT, BOSSRUSH_COUNT);
   PAL_DrawText(L"\u606D\u559C\u901A\u5173!", PAL_XY(80, 45), 0x2C, TRUE, FALSE, FALSE);
   PAL_DrawText(L"\u4F60\u5DF2\u6210\u4E3A\u4ED9\u5251\u5947\u4FA0\u4F20\u5927\u5E08\uFF01", PAL_XY(30, 70), 0x1B, TRUE, FALSE, FALSE);
   PAL_DrawText(L"\u8BE5\u9000\u51FA\u6E38\u620F\u4E86\uFF08\u5F00\u4E2A\u73A9\u7B11\uFF09", PAL_XY(45, 90), 0x1C, TRUE, FALSE, FALSE);
   PAL_DrawText(L"[A] \u7EE7\u7EED", PAL_XY(130, 130), 0x4F, TRUE, FALSE, TRUE);
   VIDEO_UpdateScreen(NULL);
   BossRushWait(TRUE);
}

//
// Boss Rush stats tracking
//
static struct {
   int floorsCleared;
   DWORD startTime;
   DWORD endTime;
} _brStats;

static void BossRushResetStats(void) {
   _brStats.floorsCleared = 0;
   _brStats.startTime = SDL_GetTicks();
   _brStats.endTime = 0;
}

static void BossRushShowRestShop(int floorCleared) {
   SDL_Surface *saved = SDL_CreateRGBSurface(SDL_SWSURFACE, 320, 200, 8, 0, 0, 0, 0);
   SDL_BlitSurface(gpScreen, NULL, saved, NULL);

   WORD _shopItems[6];
   WORD _itemPool[] = {162,163,164,165,166,167,168,169,170,171,172,
      173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190};
   int poolSz = sizeof(_itemPool)/sizeof(_itemPool[0]);
   for (int si = 0; si < 6; si++) {
      if (si == 0) _shopItems[si] = 162;
      else {
         int ri = RandomLong(0, poolSz-1);
         _shopItems[si] = _itemPool[ri];
      }
   }

   int sel = 0, done = 0, itemCount = 6;
   WCHAR wb[64];
   while (!done) {
      BossRushDrawBox(20, 15, 280, 170, 0x00);
      PAL_DrawText(L"\u2014 \u4F11\u606F\u7AD9 \u2014", PAL_XY(90, 25), 0x2C, TRUE, FALSE, FALSE);
      PAL_swprintf(wb, 64, L"\u91D1\u94B1: %d", gpGlobals->dwCash);
      PAL_DrawText(wb, PAL_XY(70, 38), 0x1B, TRUE, FALSE, FALSE);

      for (int si = 0; si < itemCount; si++) {
         int sy = 56 + si * 18;
         if (si == sel) {
            SDL_Rect sr; sr.x=24; sr.y=sy-2; sr.w=268; sr.h=16; SDL_FillRect(gpScreen, &sr, 0x1B);
         }
         WORD iid = _shopItems[si];
         LPCWSTR iname = PAL_GetWord(iid);
         if (!iname || !iname[0]) iname = L"?";
         WORD price = gpGlobals->g.rgObject[iid].item.wPrice;
         PAL_DrawText(iname, PAL_XY(30, sy), (si==sel)?0x4F:0x1C, TRUE, FALSE, FALSE);
         PAL_swprintf(wb, 64, L"%d", price);
         PAL_DrawText(wb, PAL_XY(240, sy), gpGlobals->dwCash >= price ? 0x2C : 0x4A, TRUE, FALSE, TRUE);
      }
      PAL_DrawText(L"[A]\u8D2D\u4E70 [B]\u7EE7\u7EED", PAL_XY(80, 168), 0x4F, TRUE, FALSE, TRUE);
      VIDEO_UpdateScreen(NULL);
      PAL_ClearKeyState();
      UTIL_Delay(80);

      if (g_InputState.dir == kDirNorth && sel > 0) sel--;
      else if (g_InputState.dir == kDirSouth && sel < itemCount - 1) sel++;
      else if (g_InputState.dwKeyPress & kKeySearch) {
         WORD iid = _shopItems[sel];
         WORD price = gpGlobals->g.rgObject[iid].item.wPrice;
         if (gpGlobals->dwCash >= price) {
            gpGlobals->dwCash -= price;
            PAL_AddItemToInventory(iid, 1);
            PAL_swprintf(wb, 64, L"\u8D2D\u4E70\u6210\u529F! -%d", price);
            CheatExec(wb);
         } else {
            CheatExec(L"\u91D1\u94B1\u4E0D\u8DB3!");
         }
      } else if (g_InputState.dwKeyPress & kKeyMenu) {
         done = 1;
      }
   }
   SDL_BlitSurface(saved, NULL, gpScreen, NULL);
   SDL_FreeSurface(saved);
   PAL_ClearKeyState();
}

static void BossRushRun(void) {
   WORD saveHP[MAX_PLAYER_ROLES];
   WORD saveMP[MAX_PLAYER_ROLES];

   // Save current HP/MP for restore on quit
   for (int i = 0; i < MAX_PLAYER_ROLES; i++) {
      saveHP[i] = gpGlobals->g.PlayerRoles.rgwHP[i];
      saveMP[i] = gpGlobals->g.PlayerRoles.rgwMP[i];
   }

   // Reset stats
   BossRushResetStats();

   int startLevel = (_brLevel > 0 && _brLevel < BOSSRUSH_COUNT) ? _brLevel : 0;
   int cleared = 0;

   for (int bi = startLevel; bi < BOSSRUSH_COUNT; bi++) {
      // Show intro screen for this boss
      BossRushIntro(bi);

      // Save state before battle for potential retry
      WORD preHP[MAX_PLAYER_ROLES], preMP[MAX_PLAYER_ROLES];
      for (int i = 0; i < MAX_PLAYER_ROLES; i++) {
         preHP[i] = gpGlobals->g.PlayerRoles.rgwHP[i];
         preMP[i] = gpGlobals->g.PlayerRoles.rgwMP[i];
      }

      // Set difficulty scaling: each floor adds 15% more stats
      // Floor 1 = 115%, Floor 12 = 100 + 11*15 = 265%
      g_wBossRushScale = 100 + bi * 15;

      // Start the battle
      int result = PAL_StartBattle(_bossTeams[bi], TRUE);

      // Reset scaling
      g_wBossRushScale = 100;

      if (result == kBattleResultWon) {
         cleared = bi + 1;
         _brLevel = cleared;
         _brStats.floorsCleared = cleared;

         // Full HP/MP restore
         for (int m = 0; m < MAX_PLAYER_ROLES; m++) {
            gpGlobals->g.PlayerRoles.rgwHP[m] = gpGlobals->g.PlayerRoles.rgwMaxHP[m];
            gpGlobals->g.PlayerRoles.rgwMP[m] = gpGlobals->g.PlayerRoles.rgwMaxMP[m];
         }

         // Show reward
         BossRushReward(bi);

         // Checkpoint: ask player if they want to continue or exit
         {
            SDL_Surface *_chkSaved = SDL_CreateRGBSurface(SDL_SWSURFACE, 320, 200, 8, 0, 0, 0, 0);
            SDL_BlitSurface(gpScreen, NULL, _chkSaved, NULL);
            { SDL_Rect _cb; _cb.x=40; _cb.y=55; _cb.w=240; _cb.h=70; SDL_FillRect(gpScreen, &_cb, 0x00);
            _cb.x=41; _cb.y=56; _cb.w=238; _cb.h=68; SDL_FillRect(gpScreen, &_cb, 0x11); }
            WCHAR _cw[48];
            PAL_swprintf(_cw, 48, L"\u5DF2\u901A\u5173\u7B2C %d / %d \u5C42", cleared, BOSSRUSH_COUNT);
            PAL_DrawText(_cw, PAL_XY(65, 68), 0x2C, TRUE, FALSE, FALSE);
            PAL_DrawText(L"[A] \u7EE7\u7EED\u6311\u6218  [B] \u9000\u51FA", PAL_XY(60, 95), 0x4F, TRUE, FALSE, TRUE);
            VIDEO_UpdateScreen(NULL);
            g_InputState.dir = kDirUnknown;
            PAL_ClearKeyState();
            BOOL _chkQuit = FALSE;
            int _chkWait = 0;
            while (1) {
               PAL_ClearKeyState();
               UTIL_Delay(50);
               if (g_InputState.dwKeyPress & kKeySearch) { break; }   // A = continue
               if (g_InputState.dwKeyPress & kKeyMenu) { _chkQuit = TRUE; break; }  // B = quit
               if (++_chkWait > 120) break; // auto-continue after ~6s
            }
            PAL_ClearKeyState();
            SDL_BlitSurface(_chkSaved, NULL, gpScreen, NULL);
            SDL_FreeSurface(_chkSaved);
            if (_chkQuit) break;
         }

         // Rest shop after floors 4 and 8
         if (cleared == 4 || cleared == 8) {
            BossRushShowRestShop(cleared);
         }

      } else {
         // Player lost - restore HP/MP to before battle
         for (int i = 0; i < MAX_PLAYER_ROLES; i++) {
            gpGlobals->g.PlayerRoles.rgwHP[i] = preHP[i];
            gpGlobals->g.PlayerRoles.rgwMP[i] = preMP[i];
         }

         // Show defeat screen and offer retry
         BossRushDrawBox(30, 50, 260, 80, 0x00);
         PAL_DrawText(L"\u6218\u8D25!", PAL_XY(130, 60), 0x4A, TRUE, FALSE, FALSE);
         WCHAR wb[40];
         PAL_swprintf(wb, 40, L"\u5012\u5728\u7B2C %d \u5C42", bi + 1);
         PAL_DrawText(wb, PAL_XY(100, 80), 0x1C, TRUE, FALSE, FALSE);
         PAL_DrawText(L"[A] \u91CD\u8BD5  [B] \u8FD4\u56DE", PAL_XY(65, 105), 0x4F, TRUE, FALSE, TRUE);
         VIDEO_UpdateScreen(NULL);

         g_InputState.dir = kDirUnknown;
         PAL_ClearKeyState();
         BOOL retry = FALSE;
         int _tw = 0;
         while (1) {
            PAL_ClearKeyState();
            UTIL_Delay(50);
            if (g_InputState.dwKeyPress & kKeySearch) { retry = TRUE; break; }
            if (g_InputState.dwKeyPress & kKeyMenu) { break; }
            if (++_tw > 120) break;
         }
         PAL_ClearKeyState();

         if (retry) {
            bi--;
            continue;
         }
         break;
      }
   }

   _brStats.endTime = SDL_GetTicks();

   // Show summary with stats
   {
      SDL_Surface *saved = SDL_CreateRGBSurface(SDL_SWSURFACE, 320, 200, 8, 0, 0, 0, 0);
      SDL_BlitSurface(gpScreen, NULL, saved, NULL);

      if (cleared >= BOSSRUSH_COUNT) {
         BossRushVictoryCelebration();
      }

      // Stats summary
      BossRushDrawBox(30, 20, 260, 160, 0x00);
      WCHAR wb[80];

      if (cleared >= BOSSRUSH_COUNT) {
         PAL_DrawText(L"\u901A\u5173\u7EDF\u8BA1", PAL_XY(100, 30), 0x2C, TRUE, FALSE, FALSE);
      } else {
         PAL_swprintf(wb, 80, L"\u6253\u901A: %d / %d", cleared, BOSSRUSH_COUNT);
         PAL_DrawText(wb, PAL_XY(100, 30), 0x4A, TRUE, FALSE, FALSE);
      }

      DWORD elapsed = _brStats.endTime - _brStats.startTime;
      int mins = elapsed / 60000;
      int secs = (elapsed % 60000) / 1000;
      PAL_swprintf(wb, 80, L"\u7528\u65F6: %02d:%02d", mins, secs);
      PAL_DrawText(wb, PAL_XY(50, 50), 0x1B, TRUE, FALSE, FALSE);

      PAL_swprintf(wb, 80, L"\u6253\u901A\u5C42\u6570: %d", cleared);
      PAL_DrawText(wb, PAL_XY(50, 68), 0x1C, TRUE, FALSE, FALSE);

      // Difficulty info
      PAL_swprintf(wb, 80, L"\u96BE\u5EA6: \u6BCF\u5C42 +15%%");
      PAL_DrawText(wb, PAL_XY(50, 86), 0x1C, TRUE, FALSE, FALSE);

      if (cleared >= BOSSRUSH_COUNT) {
         PAL_DrawText(L"\u5DF2\u83B7\u5F97\u4ED9\u5251\u901A\u5173\u5956\u7AE0!", PAL_XY(30, 110), 0x2C, TRUE, FALSE, FALSE);
      } else {
         PAL_swprintf(wb, 80, L"\u5012\u5728\u7B2C %d \u5C42", cleared + 1);
         PAL_DrawText(wb, PAL_XY(50, 110), 0x4A, TRUE, FALSE, FALSE);
      }

      PAL_DrawText(L"[A] \u8FD4\u56DE\u83DC\u5355", PAL_XY(100, 145), 0x4F, TRUE, FALSE, TRUE);
      VIDEO_UpdateScreen(NULL);
      BossRushWait(FALSE);

      SDL_BlitSurface(saved, NULL, gpScreen, NULL);
      SDL_FreeSurface(saved);
   }

   if (cleared >= BOSSRUSH_COUNT) {
      _brLevel = 0;
   }

   for (int i = 0; i < MAX_PLAYER_ROLES; i++) {
      if (saveHP[i] > 0) gpGlobals->g.PlayerRoles.rgwHP[i] = saveHP[i];
      if (saveMP[i] > 0) gpGlobals->g.PlayerRoles.rgwMP[i] = saveMP[i];
   }

   g_InputState.dir = kDirUnknown;
   PAL_ClearKeyState();
}

//
// Teleport / Fast Travel
//
typedef struct {
   WORD scene;
   WORD tileX;
   WORD tileY;
   LPCWSTR name;
} TRAVELDEST;

static const TRAVELDEST _travelDests[] = {
   {  1, 0, 0, L"\u4F59\u676D\u53BF" },
   {  3, 0, 0, L"\u4ED9\u7075\u5C9B" },
   {  4, 0, 0, L"\u6C34\u6708\u5BAB" },
   { 20, 0, 0, L"\u82CF\u5DDE\u57CE" },
   { 30, 0, 0, L"\u9690\u9F99\u7A9F" },
   { 37, 0, 0, L"\u767D\u6CB3\u6751" },
   { 50, 0, 0, L"\u7389\u4F5B\u5BFA" },
   { 60, 0, 0, L"\u9ED1\u6C34\u9547" },
   { 70, 0, 0, L"\u9B3C\u9634\u5C71" },
   { 80, 0, 0, L"\u626C\u5DDE\u57CE" },
   { 90, 0, 0, L"\u86E4\u86AA\u5C71" },
   {100, 0, 0, L"\u4EAC\u57CE" },
   {110, 0, 0, L"\u8700\u5C71" },
   {115, 0, 0, L"\u9501\u5996\u5854" },
   {120, 0, 0, L"\u5927\u7406" },
   {130, 0, 0, L"\u5357\u8BDB" },
   {140, 0, 0, L"\u8BD5\u70BC\u7A9F" },
   {150, 0, 0, L"\u795E\u6BBF" },
};
#define _TRAVEL_COUNT (sizeof(_travelDests) / sizeof(_travelDests[0]))

static void PAL_TeleportTo(WORD scene, WORD tileX, WORD tileY, BOOL withTransition)
{
   // For scenic teleport (tileX/tileY == 0), find the center of the map
   WORD useX = tileX, useY = tileY;
   if (useX == 0 && useY == 0 && scene > 0 && scene <= MAX_SCENES) {
      WORD mapNum = gpGlobals->g.rgScene[scene - 1].wMapNum;
      if (mapNum > 0) {
         FILE *fpMAP = UTIL_OpenRequiredFile("map.mkf");
         FILE *fpGOP = UTIL_OpenRequiredFile("gop.mkf");
         LPPALMAP pMap = PAL_LoadMap(mapNum, fpMAP, fpGOP);
         if (pMap != NULL) {
            // Scan for walkable tiles (have ground tile, no blocking object)
            WORD totalX = 0, totalY = 0, count = 0;
            for (int mx = 2; mx < 126; mx++) {
               for (int my = 2; my < 62; my++) {
                  WORD t0 = pMap->Tiles[mx][my][0];
                  WORD t1 = pMap->Tiles[mx][my][1];
                  if (t0 != 0 && t1 == 0) {
                     totalX += mx; totalY += my; count++;
                  }
               }
            }
            if (count > 0) {
               useX = totalX / count;
               useY = totalY / count;
            }
            free(pMap);
            fclose(fpGOP);
            fclose(fpMAP);
         } else {
            fclose(fpGOP);
            fclose(fpMAP);
         }
      }
   }

   // Convert tile coords to pixel coords (isometric: 32px wide, 16px tall per tile)
   int px = (int)useX * 32 + 16;
   int py = (int)useY * 16 + 8;

   if (withTransition) {
      // Loading transition
      SDL_Surface *_loadSaved = SDL_CreateRGBSurface(SDL_SWSURFACE, 320, 200, 8, 0, 0, 0, 0);
      SDL_BlitSurface(gpScreen, NULL, _loadSaved, NULL);
      SDL_Rect _lr; _lr.x=0; _lr.y=0; _lr.w=320; _lr.h=200;
      SDL_FillRect(gpScreen, &_lr, 0x2C);
      VIDEO_UpdateScreen(NULL); UTIL_Delay(60);
      SDL_FillRect(gpScreen, &_lr, 0x00);
      PAL_DrawText(L"\u4F20\u9001\u4E2D\u2026", PAL_XY(120, 90), 0x2C, TRUE, FALSE, FALSE);
      VIDEO_UpdateScreen(NULL); UTIL_Delay(250);
      SDL_BlitSurface(_loadSaved, NULL, gpScreen, NULL);
      SDL_FreeSurface(_loadSaved);
   }

   gpGlobals->wNumScene = scene;
   gpGlobals->partyoffset = PAL_XY(160, 100);
   gpGlobals->viewport = PAL_XY(px - 160, py - 100);
   gpGlobals->wLayer = 0;
   gpGlobals->fEnteringScene = TRUE;
   gpGlobals->wPartyDirection = 2;
   PAL_SetLoadFlags(kLoadScene);
}

VOID
PAL_TeleportMenu(VOID)
{
   SDL_Surface *saved = SDL_CreateRGBSurface(SDL_SWSURFACE, 320, 200, 8, 0, 0, 0, 0);
   SDL_BlitSurface(gpScreen, NULL, saved, NULL);
   int sel = 0, pg = 0, pp = 7;
   BOOL done = FALSE;
   int tab = 0; // 0=scenic, 1=waypoints

   while (!done) {
      SDL_Rect bg; bg.x=10; bg.y=10; bg.w=300; bg.h=180;
      SDL_FillRect(gpScreen, &bg, 0x00);
      bg.x=11; bg.y=11; bg.w=298; bg.h=178;
      SDL_FillRect(gpScreen, &bg, 0x11);
      SDL_Rect tr; tr.x=10; tr.y=10; tr.w=300; tr.h=14;
      SDL_FillRect(gpScreen, &tr, 0x2C);

      // Tab bar
      SDL_Rect tb1; tb1.x=14; tb1.y=26; tb1.w=142; tb1.h=13;
      SDL_FillRect(gpScreen, &tb1, (tab==0)?0x1B:0x11);
      PAL_DrawText(L"\u666F\u70B9\u4F20\u9001", PAL_XY(30, 27), (tab==0)?0x4F:0x1C, TRUE, FALSE, FALSE);
      SDL_Rect tb2; tb2.x=162; tb2.y=26; tb2.w=142; tb2.h=13;
      SDL_FillRect(gpScreen, &tb2, (tab==1)?0x1B:0x11);
      PAL_DrawText(L"\u5B9A\u70B9\u4F20\u9001", PAL_XY(182, 27), (tab==1)?0x4F:0x1C, TRUE, FALSE, FALSE);

      WCHAR _w[48];

      if (tab == 0) {
         // Scenic teleport
         WORD validScenes[_TRAVEL_COUNT];
         int validCount = 0;
         for (int di = 0; di < _TRAVEL_COUNT; di++) {
            WORD s = _travelDests[di].scene;
            for (int vi = 0; vi < MAX_TRAVEL_LOCATIONS && gpGlobals->rgTravelScenesVisited[vi] != 0; vi++) {
               if (gpGlobals->rgTravelScenesVisited[vi] == s) { validScenes[validCount++] = (WORD)di; break; }
            }
         }

         int pgc = (validCount + pp - 1) / pp; if (pgc < 1) pgc = 1;
         PAL_swprintf(_w, 24, L"%d/%d", pg+1, pgc);
         PAL_DrawText(_w, PAL_XY(265, 12), 0x1C, TRUE, FALSE, TRUE);

         if (validCount == 0) {
            PAL_DrawText(L"\u8FD8\u6CA1\u6709\u53BB\u8FC7\u4EFB\u4F55\u5730\u65B9\u2026", PAL_XY(40, 80), 0x1C, TRUE, FALSE, FALSE);
         } else {
            int st = pg * pp, ed = st + pp; if (ed > validCount) ed = validCount;
            for (int i = st; i < ed; i++) {
               int j = i - st, y = 44 + j * 17;
               if (i == sel) { SDL_Rect sr; sr.x=14; sr.y=y-2; sr.w=290; sr.h=16; SDL_FillRect(gpScreen, &sr, 0x1B); }
               int di = validScenes[i];
               PAL_DrawText(_travelDests[di].name, PAL_XY(22, y), (i==sel)?0x4F:0x1C, TRUE, FALSE, FALSE);
               PAL_swprintf(_w, 24, L"#%d", _travelDests[di].scene);
               PAL_DrawText(_w, PAL_XY(260, y), (i==sel)?0x7C:0x18, TRUE, FALSE, TRUE);
            }
         }
      } else {
         // Waypoint tab
         // Count waypoints
         int wpCount = 0;
         for (int wi = 0; wi < MAX_WAYPOINTS; wi++) {
            if (gpGlobals->rgWaypointScene[wi] != 0) wpCount++;
         }

         int pgc = (wpCount + pp - 1) / pp; if (pgc < 1) pgc = 1;
         PAL_swprintf(_w, 24, L"%d/%d", pg+1, pgc);
         PAL_DrawText(_w, PAL_XY(265, 12), 0x1C, TRUE, FALSE, TRUE);

         if (wpCount == 0) {
            PAL_DrawText(L"\u8FD8\u6CA1\u6709\u8BB0\u5F55\u5B9A\u70B9\u2026", PAL_XY(40, 80), 0x1C, TRUE, FALSE, FALSE);
            PAL_DrawText(L"\u5728\u5730\u56FE\u4E0A\u6309 [\u2191] \u4FDD\u5B58\u5F53\u524D\u4F4D\u7F6E", PAL_XY(24, 100), 0x1B, TRUE, FALSE, FALSE);
         } else {
            int st = pg * pp, ed = st + pp; if (ed > wpCount) ed = wpCount;
            // Build linear index
            WORD wpIdx[MAX_WAYPOINTS];
            int wic = 0;
            for (int wi = 0; wi < MAX_WAYPOINTS; wi++) {
               if (gpGlobals->rgWaypointScene[wi] != 0) wpIdx[wic++] = (WORD)wi;
            }
            for (int i = st; i < ed; i++) {
               int j = i - st, y = 44 + j * 17;
               if (i == sel) { SDL_Rect sr; sr.x=14; sr.y=y-2; sr.w=290; sr.h=16; SDL_FillRect(gpScreen, &sr, 0x1B); }
               int wi = wpIdx[i];
               PAL_swprintf(_w, 48, L"\u5B9A\u70B9 #%d", wi+1);
               PAL_DrawText(_w, PAL_XY(22, y), (i==sel)?0x4F:0x1C, TRUE, FALSE, FALSE);
               PAL_swprintf(_w, 24, L"#%d", gpGlobals->rgWaypointScene[wi]);
               PAL_DrawText(_w, PAL_XY(260, y), (i==sel)?0x7C:0x18, TRUE, FALSE, TRUE);
            }
         }
      }

      PAL_DrawText(L"[A]\u4F20\u9001 [\u2190\u2192]\u5207\u6362 [Y]\u4FDD\u5B58 [B]\u8FD4\u56DE", PAL_XY(16, 176), 0x11, TRUE, FALSE, TRUE);
      VIDEO_UpdateScreen(NULL);
      PAL_ClearKeyState();
      UTIL_Delay(80);

      if (g_InputState.dir == kDirNorth && sel > 0) { sel--; pg = sel / pp; }
      else if (g_InputState.dir == kDirSouth) {
         int maxItems = (tab == 0) ? ({
            int _c = 0; for (int _ = 0; _ < _TRAVEL_COUNT; _++) {
               for (int __ = 0; __ < MAX_TRAVEL_LOCATIONS && gpGlobals->rgTravelScenesVisited[__] != 0; __++) {
                  if (gpGlobals->rgTravelScenesVisited[__] == _travelDests[_].scene) { _c++; break; }
               }
            } _c;
         }) : ({
            int _c = 0; for (int _ = 0; _ < MAX_WAYPOINTS; _++) if (gpGlobals->rgWaypointScene[_] != 0) _c++; _c;
         });
         if (sel < maxItems - 1) { sel++; pg = sel / pp; }
      }
      else if (g_InputState.dir == kDirWest || g_InputState.dir == kDirEast) {
         tab = !tab; sel = 0; pg = 0;
      }
      else if (g_InputState.dwKeyPress & kKeyMenu) { done = TRUE; }
      // Y key: save waypoint
      else if (g_InputState.dwKeyPress & kKeyForce) {
         if (tab == 1) PAL_SaveWaypoint();
      }
      // A key: teleport
      else if (g_InputState.dwKeyPress & kKeySearch) {
         if (tab == 0) {
            WORD validScenes[_TRAVEL_COUNT];
            int validCount = 0;
            for (int di = 0; di < _TRAVEL_COUNT; di++) {
               WORD s = _travelDests[di].scene;
               for (int vi = 0; vi < MAX_TRAVEL_LOCATIONS && gpGlobals->rgTravelScenesVisited[vi] != 0; vi++) {
                  if (gpGlobals->rgTravelScenesVisited[vi] == s) { validScenes[validCount++] = (WORD)di; break; }
               }
            }
            if (sel >= 0 && sel < validCount) {
               int di = validScenes[sel];
               PAL_TeleportTo(_travelDests[di].scene, _travelDests[di].tileX, _travelDests[di].tileY, TRUE);
               SDL_BlitSurface(saved, NULL, gpScreen, NULL);
               SDL_FreeSurface(saved); PAL_ClearKeyState(); return;
            }
         } else {
            // Waypoint teleport
            WORD wpIdx[MAX_WAYPOINTS]; int wic = 0;
            for (int wi = 0; wi < MAX_WAYPOINTS; wi++) if (gpGlobals->rgWaypointScene[wi] != 0) wpIdx[wic++] = (WORD)wi;
            if (sel >= 0 && sel < wic) {
               int wi = wpIdx[sel];
               PAL_TeleportTo(gpGlobals->rgWaypointScene[wi], gpGlobals->rgWaypointX[wi], gpGlobals->rgWaypointY[wi], TRUE);
               SDL_BlitSurface(saved, NULL, gpScreen, NULL);
               SDL_FreeSurface(saved); PAL_ClearKeyState(); return;
            }
         }
      }
   }
   SDL_BlitSurface(saved, NULL, gpScreen, NULL);
   SDL_FreeSurface(saved);
   PAL_ClearKeyState();
}

//
// Save current position as a waypoint (callable from game)
//
VOID
PAL_SaveWaypoint(VOID)
{
   BYTE bx, by, bh;
   int sx = PAL_X(gpGlobals->viewport) + PAL_X(gpGlobals->partyoffset);
   int sy = PAL_Y(gpGlobals->viewport) + PAL_Y(gpGlobals->partyoffset);
   PAL_POS_TO_XYH(PAL_XY(sx, sy), bx, by, bh);

   // Find empty slot or overwrite oldest
   int slot = -1;
   for (int i = 0; i < MAX_WAYPOINTS; i++) {
      if (gpGlobals->rgWaypointScene[i] == 0) { slot = i; break; }
   }
   if (slot < 0) slot = 0; // overwrite first slot if full

   gpGlobals->rgWaypointScene[slot] = gpGlobals->wNumScene;
   gpGlobals->rgWaypointX[slot] = bx;
   gpGlobals->rgWaypointY[slot] = by;

   WCHAR _w[48];
   PAL_swprintf(_w, 48, L"\u5B9A\u70B9 #%d \u5DF2\u4FDD\u5B58 (#%d)", slot+1, gpGlobals->wNumScene);
   CheatExec(_w);
}

//
// Bestiary / Item Compendium
//
static void BestiaryDrawBox(int x, int y, int w, int h) {
   SDL_Rect r; r.x=x; r.y=y; r.w=w; r.h=h; SDL_FillRect(gpScreen, &r, 0x00);
   r.x=x+1; r.y=y+1; r.w=w-2; r.h=h-2; SDL_FillRect(gpScreen, &r, 0x11);
}

VOID
PAL_BestiaryMenu(
   VOID
)
{
   SDL_Surface *saved = SDL_CreateRGBSurface(SDL_SWSURFACE, 320, 200, 8, 0, 0, 0, 0);
   SDL_BlitSurface(gpScreen, NULL, saved, NULL);

   int tab = 0; // 0 = monsters, 1 = items
   int sel = 0;
   int pg = 0, pp = 7;
   BOOL done = FALSE;

   while (!done) {
      BestiaryDrawBox(10, 10, 300, 180);

      // Title bar
      SDL_Rect tr; tr.x=10; tr.y=10; tr.w=300; tr.h=14; SDL_FillRect(gpScreen, &tr, 0x2C);
      PAL_DrawText(L"\u56FE\u9274", PAL_XY(135, 11), 0x4F, TRUE, FALSE, FALSE);

      // Tab buttons
      SDL_Rect tb1; tb1.x=20; tb1.y=27; tb1.w=130; tb1.h=14;
      SDL_FillRect(gpScreen, &tb1, (tab==0)?0x1B:0x11);
      PAL_DrawText(L"\u602A\u7269\u56FE\u9274", PAL_XY(40, 28), (tab==0)?0x4F:0x1C, TRUE, FALSE, FALSE);
      SDL_Rect tb2; tb2.x=160; tb2.y=27; tb2.w=130; tb2.h=14;
      SDL_FillRect(gpScreen, &tb2, (tab==1)?0x1B:0x11);
      PAL_DrawText(L"\u7269\u54C1\u56FE\u9274", PAL_XY(180, 28), (tab==1)?0x4F:0x1C, TRUE, FALSE, FALSE);

      if (tab == 0) {
         // Monster tab
         int st = pg * pp, ed = st + pp;
         if (ed > gpGlobals->iBestiaryCount) ed = gpGlobals->iBestiaryCount;
         int pgc = (gpGlobals->iBestiaryCount + pp - 1) / pp;
         if (pgc < 1) pgc = 1;

         // Page indicator
         WCHAR _w[24];
         PAL_swprintf(_w, 24, L"%d/%d", pg+1, pgc);
         PAL_DrawText(_w, PAL_XY(265, 12), 0x1C, TRUE, FALSE, TRUE);

         // Column headers
         PAL_DrawText(L"\u540D\u79F0", PAL_XY(22, 44), 0x1B, TRUE, FALSE, FALSE);
         PAL_DrawText(L"Lv", PAL_XY(115, 44), 0x1B, TRUE, FALSE, TRUE);
         PAL_DrawText(L"HP", PAL_XY(155, 44), 0x1B, TRUE, FALSE, TRUE);
         PAL_DrawText(L"\u653B", PAL_XY(195, 44), 0x1B, TRUE, FALSE, FALSE);
         PAL_DrawText(L"\u9632", PAL_XY(225, 44), 0x1B, TRUE, FALSE, FALSE);
         PAL_DrawText(L"\u51FB\u6740", PAL_XY(253, 44), 0x1B, TRUE, FALSE, FALSE);

         for (int i = st; i < ed; i++) {
            int j = i - st, y = 58 + j * 16;
            if (i == sel) {
               SDL_Rect sr; sr.x=14; sr.y=y-2; sr.w=290; sr.h=15; SDL_FillRect(gpScreen, &sr, 0x1B);
            }
            WORD oid = gpGlobals->rgBestiary[i];
            LPCWSTR _name = PAL_GetWord(oid);
            if (!_name) _name = L"?";
            PAL_DrawText(_name, PAL_XY(22, y), (i==sel)?0x4F:0x1C, TRUE, FALSE, FALSE);

            // Stats from the enemy data
            WORD eid = gpGlobals->g.rgObject[oid].enemy.wEnemyID;
            LPENEMY pe = &gpGlobals->g.lprgEnemy[eid];
            PAL_DrawNumber(pe->wLevel, 2, PAL_XY(122, y), kNumColorYellow, kNumAlignRight);
            PAL_DrawNumber(pe->wHealth, 4, PAL_XY(162, y), kNumColorYellow, kNumAlignRight);
            PAL_DrawNumber(pe->wAttackStrength, 3, PAL_XY(200, y), kNumColorYellow, kNumAlignRight);
            PAL_DrawNumber(pe->wDefense, 3, PAL_XY(230, y), kNumColorYellow, kNumAlignRight);
            PAL_DrawNumber(gpGlobals->rgBestiaryKills[i], 3, PAL_XY(265, y), kNumColorCyan, kNumAlignRight);
         }

         if (sel >= 0 && sel < gpGlobals->iBestiaryCount) {
            WORD oid = gpGlobals->rgBestiary[sel];
            WORD eid = gpGlobals->g.rgObject[oid].enemy.wEnemyID;
            LPENEMY pe = &gpGlobals->g.lprgEnemy[eid];
            PAL_swprintf(_w, 24, L"\u9B54\u653B:%d \u901F:%d \u7ECF\u9A8C:%d \u91D1\u94B1:%d",
               pe->wMagicStrength, pe->wDexterity, pe->wExp, pe->wCash);
            PAL_DrawText(_w, PAL_XY(22, 165), 0x1C, TRUE, FALSE, FALSE);
         }

      } else {
         // Item tab
         // Build item list from game data
         WORD itemList[256];
         int itemCount = 0;
         for (int ii = 91; ii < MAX_OBJECTS && itemCount < 256; ii++) {
            WORD bm = gpGlobals->g.rgObject[ii].item.wBitmap;
            WORD pr = gpGlobals->g.rgObject[ii].item.wPrice;
            if (bm != 0 || pr != 0) {
               itemList[itemCount++] = (WORD)ii;
            }
         }

         int st = pg * pp, ed = st + pp;
         if (ed > itemCount) ed = itemCount;
         int pgc = (itemCount + pp - 1) / pp;
         if (pgc < 1) pgc = 1;

         WCHAR _w[24];
         PAL_swprintf(_w, 24, L"%d/%d", pg+1, pgc);
         PAL_DrawText(_w, PAL_XY(265, 12), 0x1C, TRUE, FALSE, TRUE);

         for (int i = st; i < ed; i++) {
            int j = i - st, y = 48 + j * 16;
            if (i == sel) {
               SDL_Rect sr; sr.x=14; sr.y=y-2; sr.w=290; sr.h=15; SDL_FillRect(gpScreen, &sr, 0x1B);
            }
            WORD iid = itemList[i];
            LPCWSTR iname = PAL_GetWord(iid);
            if (!iname) iname = L"?";
            PAL_DrawText(iname, PAL_XY(22, y), (i==sel)?0x4F:0x1C, TRUE, FALSE, FALSE);

            // Price
            WORD pr = gpGlobals->g.rgObject[iid].item.wPrice;
            PAL_DrawNumber(pr, 5, PAL_XY(200, y), kNumColorYellow, kNumAlignRight);

            // Owned count
            int amt = PAL_GetItemAmount(iid);
            PAL_DrawNumber(amt, 3, PAL_XY(260, y), kNumColorCyan, kNumAlignRight);
         }

         if (sel >= 0 && sel < itemCount) {
            PAL_swprintf(_w, 24, L"\u6301\u6709\u6570:%d", PAL_GetItemAmount(itemList[sel]));
            PAL_DrawText(_w, PAL_XY(22, 160), 0x1C, TRUE, FALSE, FALSE);
         }
      }

      PAL_DrawText(L"[A]\u5207\u6362 [D]\u6807\u7B7E [B]\u8FD4\u56DE [\u2191\u2193]\u9009\u62E9 [\u2190\u2192]\u9875", PAL_XY(16, 176), 0x11, TRUE, FALSE, FALSE);

      VIDEO_UpdateScreen(NULL);
      PAL_ClearKeyState();
      UTIL_Delay(80);

      if (g_InputState.dir == kDirNorth && sel > 0) sel--;
      else if (g_InputState.dir == kDirSouth) {
         int maxItems = (tab == 0) ? gpGlobals->iBestiaryCount : (tab == 1 ? 256 : 0);
         if (sel < maxItems - 1) sel++;
      }
      else if (g_InputState.dir == kDirWest && pg > 0) { pg--; sel = pg * pp; }
      else if (g_InputState.dir == kDirEast) {
         int maxItems = (tab == 0) ? gpGlobals->iBestiaryCount : (tab == 1 ? 256 : 0);
         int pgc = (maxItems + pp - 1) / pp;
         if (pgc < 1) pgc = 1;
         if (pg < pgc - 1) { pg++; sel = pg * pp; }
      }
      else if (g_InputState.dwKeyPress & kKeyMenu) { done = TRUE; }
      else if (g_InputState.dwKeyPress & kKeySearch) {
         // Toggle tab
         tab = !tab; sel = 0; pg = 0;
      }
      // Tab switching with shoulder/minimap key
      else if (g_InputState.dwKeyPress & kKeyDefend) {
         tab = !tab; sel = 0; pg = 0;
      }
   }

   SDL_BlitSurface(saved, NULL, gpScreen, NULL);
   SDL_FreeSurface(saved);
   PAL_ClearKeyState();
}

VOID PAL_CheatMenu(void) {
   int sel=0, pg=0, pp=4, total=13, pgc=(total+pp-1)/pp;
   BOOL done=0;
   VIDEO_BackupScreen(gpScreen);
   while(!done) {
      PAL_ClearDialog(FALSE);
      VIDEO_RestoreScreen(gpScreen);
      SDL_Rect bg; bg.x=14; bg.y=18; bg.w=290; bg.h=155;
      SDL_FillRect(gpScreen, &bg, 0x00);
      bg.x=15; bg.y=19; bg.w=288; bg.h=153;
      SDL_FillRect(gpScreen, &bg, 0x11);
      SDL_Rect tr; tr.x=14;tr.y=18;tr.w=290;tr.h=14;
      SDL_FillRect(gpScreen,&tr,0x2C);
      WCHAR b[48];
      PAL_swprintf(b,48,L"%d/%d",pg+1,pgc);
      PAL_DrawText(b,PAL_XY(130,20),0x4F,TRUE,FALSE,FALSE);
      int st=pg*pp,ed=st+pp; if(ed>total)ed=total;
      for(int i=st;i<ed;i++) {
         int j=i-st,y=42+j*22;
         if(i==sel){SDL_Rect sr;sr.x=18;sr.y=y-2;sr.w=278;sr.h=18;SDL_FillRect(gpScreen,&sr,0x1B);}
          LPCWSTR lbl;
          switch(i) {
             default: lbl = L"?"; break;
            case 0: lbl = L"天降橫財"; break;
            case 1: lbl = L"起死回生"; break;
            case 2: lbl = L"逆天改命"; break;
            case 3: lbl = L"無盡酒神"; break;
            case 4: lbl = L"全隊升一級"; break;
            case 5: lbl = L"御劍飛行"; break;
            case 6: lbl = L"AI自動戰鬥"; break;
            case 7: lbl = L"無遇敵模式"; break;
            case 8: lbl = L"地圖全亮"; break;
            case 9: lbl = L"全物品獲取"; break;
            case 10: lbl = L"BOSS連戰"; break;
            case 11: lbl = L"\u56FE\u9274"; break;
            case 12: lbl = L"\u5730\u56FE\u4F20\u9001"; break;
         }
         PAL_DrawText(lbl,PAL_XY(26,y),(i==sel)?0x4F:0x1C,TRUE,FALSE,FALSE);
         if(i==0)PAL_DrawText(L"+9999",PAL_XY(200,y),(i==sel)?0x7C:0x18,TRUE,FALSE,TRUE);
         if(i==1)PAL_DrawText(L"HP/MP",PAL_XY(200,y),(i==sel)?0x7C:0x18,TRUE,FALSE,TRUE);
         if(i==2)PAL_DrawText(L"MAX",PAL_XY(200,y),(i==sel)?0x7C:0x18,TRUE,FALSE,TRUE);
         if(i==3)PAL_DrawText(L"99",PAL_XY(200,y),(i==sel)?0x7C:0x18,TRUE,FALSE,TRUE);
         if(i==4)PAL_DrawText(L"Lv+1",PAL_XY(200,y),(i==sel)?0x7C:0x18,TRUE,FALSE,TRUE);
         if(i==5)PAL_DrawText(gpGlobals->fNoClip?L"ON":L"OFF",PAL_XY(240,y),gpGlobals->fNoClip?0x2C:0x4A,TRUE,FALSE,TRUE);
         if(i==6)PAL_DrawText(gpGlobals->fAIAutoBattle?L"ON":L"OFF",PAL_XY(240,y),gpGlobals->fAIAutoBattle?0x2C:0x4A,TRUE,FALSE,TRUE);
         if(i==7)PAL_DrawText(gpGlobals->fNoEncounter?L"ON":L"OFF",PAL_XY(240,y),gpGlobals->fNoEncounter?0x2C:0x4A,TRUE,FALSE,TRUE);
         if(i==8)PAL_DrawText(gpGlobals->fMapReveal?L"ON":L"OFF",PAL_XY(240,y),gpGlobals->fMapReveal?0x2C:0x4A,TRUE,FALSE,TRUE);
         if(i==9)PAL_DrawText(gpGlobals->fAllItems?L"ON":L"OFF",PAL_XY(240,y),gpGlobals->fAllItems?0x2C:0x4A,TRUE,FALSE,TRUE);
         if(i==10){
            LPCWSTR _brs;
            if(_brLevel==0) _brs = L"未挑战";
            else if(_brLevel>=BOSSRUSH_COUNT) _brs = L"已通關";
            else { static WCHAR _brb[8]; PAL_swprintf(_brb,8,L"%dF",_brLevel); _brs = _brb; }
            PAL_DrawText(_brs,PAL_XY(240,y),0x1C,TRUE,FALSE,TRUE);
         }
         if(i==11)PAL_DrawText(gpGlobals->iBestiaryCount>0?L"\u2605":L"",PAL_XY(240,y),0x1C,TRUE,FALSE,TRUE);
      }
      PAL_DrawText(L"<>Pg /\\\\Sel A=OK B=Back",PAL_XY(26,168),0x1B,TRUE,FALSE,TRUE);
      VIDEO_UpdateScreen(NULL); PAL_ClearKeyState(); UTIL_Delay(80);
      if(g_InputState.dir==kDirNorth&&sel>0)sel--;
      else if(g_InputState.dir==kDirSouth&&sel<total-1)sel++;
      else if(g_InputState.dir==kDirWest&&pg>0){pg--;sel=pg*pp;}
      else if(g_InputState.dir==kDirEast&&pg<pgc-1){pg++;sel=pg*pp;}
      else if(g_InputState.dwKeyPress&kKeyMenu)done=1;
      else if(g_InputState.dwKeyPress&kKeySearch) {
         switch(sel) {
         case 0: gpGlobals->dwCash+=9999; CheatExec(L"+9999"); break;
         case 1: for(int i=0;i<=gpGlobals->wMaxPartyMemberIndex&&i<MAX_PLAYER_ROLES;i++){WORD r=gpGlobals->rgParty[i].wPlayerRole;if(r<MAX_PLAYER_ROLES){gpGlobals->g.PlayerRoles.rgwHP[r]=gpGlobals->g.PlayerRoles.rgwMaxHP[r];gpGlobals->g.PlayerRoles.rgwMP[r]=gpGlobals->g.PlayerRoles.rgwMaxMP[r];}}CheatExec(L"OK");break;
         case 2: for(int i=0;i<MAX_PLAYER_ROLES;i++){gpGlobals->g.PlayerRoles.rgwLevel[i]=99;gpGlobals->g.PlayerRoles.rgwMaxHP[i]=9999;gpGlobals->g.PlayerRoles.rgwMaxMP[i]=9999;gpGlobals->g.PlayerRoles.rgwHP[i]=9999;gpGlobals->g.PlayerRoles.rgwMP[i]=9999;gpGlobals->g.PlayerRoles.rgwAttackStrength[i]=999;gpGlobals->g.PlayerRoles.rgwMagicStrength[i]=999;gpGlobals->g.PlayerRoles.rgwDefense[i]=999;gpGlobals->g.PlayerRoles.rgwDexterity[i]=999;}CheatExec(L"OK");break;
         case 3: {int f=0;for(int i=0;i<MAX_INVENTORY;i++){if(gpGlobals->rgInventory[i].wItem==0){gpGlobals->rgInventory[i].wItem=114;gpGlobals->rgInventory[i].nAmount=99;gpGlobals->rgInventory[i].nAmountInUse=0;f=1;break;}else if(gpGlobals->rgInventory[i].wItem==114){gpGlobals->rgInventory[i].nAmount+=99;if(gpGlobals->rgInventory[i].nAmount>999)gpGlobals->rgInventory[i].nAmount=999;f=1;break;}}CheatExec(f?L"OK":L"Full");}break;
         case 4: for(int i=0;i<=gpGlobals->wMaxPartyMemberIndex&&i<MAX_PLAYER_ROLES;i++){WORD r=gpGlobals->rgParty[i].wPlayerRole;if(r<MAX_PLAYER_ROLES&&gpGlobals->g.PlayerRoles.rgwLevel[r]<99){gpGlobals->g.PlayerRoles.rgwLevel[r]++;gpGlobals->g.PlayerRoles.rgwHP[r]=gpGlobals->g.PlayerRoles.rgwMaxHP[r];gpGlobals->g.PlayerRoles.rgwMP[r]=gpGlobals->g.PlayerRoles.rgwMaxMP[r];}}CheatExec(L"OK");break;
         case 5: gpGlobals->fNoClip=!gpGlobals->fNoClip; CheatExec(gpGlobals->fNoClip?L"ON":L"OFF"); break;
         case 6: gpGlobals->fAIAutoBattle=!gpGlobals->fAIAutoBattle; CheatExec(gpGlobals->fAIAutoBattle?L"ON":L"OFF"); break;
         case 7: gpGlobals->fNoEncounter=!gpGlobals->fNoEncounter; CheatExec(gpGlobals->fNoEncounter?L"ON":L"OFF"); break;
         case 8: {
            gpGlobals->fMapReveal=!gpGlobals->fMapReveal;
            if(gpGlobals->fMapReveal){for(int mx=0;mx<MAP_TILE_MAX_X;mx++)for(int my=0;my<MAP_TILE_MAX_Y;my++)for(int mh=0;mh<MAP_TILE_MAX_H;mh++)gpGlobals->explored[mh*MAP_TILE_MAX_X*MAP_TILE_MAX_Y+my*MAP_TILE_MAX_X+mx]=1;}
            else{for(int mx=0;mx<MAP_TILE_MAX_X;mx++)for(int my=0;my<MAP_TILE_MAX_Y;my++)for(int mh=0;mh<MAP_TILE_MAX_H;mh++)gpGlobals->explored[mh*MAP_TILE_MAX_X*MAP_TILE_MAX_Y+my*MAP_TILE_MAX_X+mx]=0;}
            CheatExec(gpGlobals->fMapReveal?L"ON":L"OFF");
         } break;
         case 9: {
            static INVENTORY _snap[MAX_INVENTORY];
            static BOOL _hasSnap = FALSE;
            if (!gpGlobals->fAllItems) {
               if (!_hasSnap) {
                  memcpy(_snap, gpGlobals->rgInventory, sizeof(INVENTORY) * MAX_INVENTORY);
                  _hasSnap = TRUE;
               }
               for (int ii = 1; ii < MAX_OBJECTS; ii++) {
                  WORD bm = gpGlobals->g.rgObject[ii].item.wBitmap;
                  WORD pr = gpGlobals->g.rgObject[ii].item.wPrice;
                  if (bm == 0 && pr == 0) continue;
                  int slot = -1;
                  for (int si = 0; si < MAX_INVENTORY; si++) {
                     if (gpGlobals->rgInventory[si].wItem == (WORD)ii) {
                        gpGlobals->rgInventory[si].nAmount += 99;
                        if (gpGlobals->rgInventory[si].nAmount > 999) gpGlobals->rgInventory[si].nAmount = 999;
                        slot = si; break;
                     }
                     if (gpGlobals->rgInventory[si].wItem == 0 && slot < 0) slot = si;
                  }
                  if (slot >= 0 && gpGlobals->rgInventory[slot].wItem != (WORD)ii) {
                     gpGlobals->rgInventory[slot].wItem = (WORD)ii;
                     gpGlobals->rgInventory[slot].nAmount = 99;
                     gpGlobals->rgInventory[slot].nAmountInUse = 0;
                  }
               }
               gpGlobals->fAllItems = TRUE;
               CheatExec(L"ON");
            } else {
               if (_hasSnap) {
                  memcpy(gpGlobals->rgInventory, _snap, sizeof(INVENTORY) * MAX_INVENTORY);
               }
               gpGlobals->fAllItems = FALSE;
               CheatExec(L"OFF");
            }
          } break;
           case 10: BossRushRun(); break;
           case 11: PAL_BestiaryMenu(); break;
           case 12: PAL_TeleportMenu(); break;
          }
      }
      pg=sel/pp;
   }
   PAL_ClearDialog(FALSE);
   VIDEO_RestoreScreen(gpScreen);
}

VOID
PAL_BuyMenu(
   WORD           wStoreNum
)
/*++
  Purpose:

    Show the buy item menu.

  Parameters:

    [IN]  wStoreNum - number of the store to buy items from.

  Return value:

    None.

--*/
{
   MENUITEM        rgMenuItem[MAX_STORE_ITEM];
   int             i, y;
   WORD            w;

   //
   // create the menu items
   //
   y = 21;

   for (i = 0; i < MAX_STORE_ITEM; i++)
   {
      if (gpGlobals->g.lprgStore[wStoreNum].rgwItems[i] == 0)
      {
         break;
      }

      rgMenuItem[i].wValue = gpGlobals->g.lprgStore[wStoreNum].rgwItems[i];
      rgMenuItem[i].wNumWord = gpGlobals->g.lprgStore[wStoreNum].rgwItems[i];
      rgMenuItem[i].fEnabled = TRUE;
      rgMenuItem[i].pos = PAL_XY(150, y);

      y += 18;
   }

   //
   // Draw the box
   //
   PAL_CreateBox(PAL_XY(122, 8), 8, 8, 1, FALSE);

   //
   // Draw the number of prices
   //
   for (y = 0; y < i; y++)
   {
      w = gpGlobals->g.rgObject[rgMenuItem[y].wValue].item.wPrice;
      PAL_DrawNumber(w, 6, PAL_XY(238, 26 + y * 18), kNumColorYellow, kNumAlignRight);
   }

   w = 0;
   __buymenu_firsttime_render = TRUE;

   while (TRUE)
   {
      w = PAL_ReadMenu(PAL_BuyMenu_OnItemChange, rgMenuItem, i, w, MENUITEM_COLOR);

      if (w == MENUITEM_VALUE_CANCELLED)
      {
         break;
      }

      if (gpGlobals->g.rgObject[w].item.wPrice <= gpGlobals->dwCash)
      {
         if (PAL_ConfirmMenu())
         {
            //
            // Player bought an item
            //
            gpGlobals->dwCash -= gpGlobals->g.rgObject[w].item.wPrice;
            PAL_AddItemToInventory(w, 1);
         }
      }

      //
      // Place the cursor to the current item on next loop
      //
      for (y = 0; y < i; y++)
      {
         if (w == rgMenuItem[y].wValue)
         {
            w = y;
            break;
         }
      }
   }
}

static VOID
PAL_SellMenu_OnItemChange(
   WORD         wCurrentItem
)
/*++
  Purpose:

    Callback function which is called when player selected another item
    in the sell item menu.

  Parameters:

    [IN]  wCurrentItem - current item on the menu, indicates the object ID of
                         the currently selected item.

  Return value:

    None.

--*/
{
   WORD x = 100, y = 150;

   //
   // Draw the cash amount
   //
   PAL_CreateSingleLineBoxWithShadow(PAL_XY(x, y), 5, FALSE, 0);
   PAL_DrawText(PAL_GetWord(CASH_LABEL), PAL_XY(x + 10, y + 10), 0, FALSE, FALSE, FALSE);
   PAL_DrawNumber(gpGlobals->dwCash, 6, PAL_XY(x + 48, y + 15), kNumColorYellow, kNumAlignRight);

   x += 124;

   //
   // Draw the price
   //
   PAL_CreateSingleLineBoxWithShadow(PAL_XY(x, y), 5, FALSE, 0);

   if (gpGlobals->g.rgObject[wCurrentItem].item.wFlags & kItemFlagSellable)
   {
      PAL_DrawText(PAL_GetWord(SELLMENU_LABEL_PRICE), PAL_XY(x + 10, y + 10), 0, FALSE, FALSE, FALSE);
      PAL_DrawNumber(gpGlobals->g.rgObject[wCurrentItem].item.wPrice / 2, 6,
         PAL_XY(x + 48, y + 15), kNumColorYellow, kNumAlignRight);
   }
}

VOID
PAL_SellMenu(
   VOID
)
/*++
  Purpose:

    Show the sell item menu.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   WORD      w;

   while (TRUE)
   {
      w = PAL_ItemSelectMenu(PAL_SellMenu_OnItemChange, kItemFlagSellable);
      if (w == 0)
      {
         break;
      }

      if (PAL_ConfirmMenu())
      {
         if (PAL_AddItemToInventory(w, -1))
         {
            gpGlobals->dwCash += gpGlobals->g.rgObject[w].item.wPrice / 2;
         }
      }
   }
}

VOID
PAL_EquipItemMenu(
   WORD        wItem
)
/*++
  Purpose:

    Show the menu which allow players to equip the specified item.

  Parameters:

    [IN]  wItem - the object ID of the item.

  Return value:

    None.

--*/
{
   PAL_LARGE BYTE   bufBackground[320 * 200];
   PAL_LARGE BYTE   bufImageBox[72 * 72];
   PAL_LARGE BYTE   bufImage[2048];
   WORD             w;
   int              iCurrentPlayer, i;
   BYTE             bColor, bSelectedColor;
   DWORD            dwColorChangeTime;

   gpGlobals->wLastUnequippedItem = wItem;

   PAL_MKFDecompressChunk(bufBackground, 320 * 200, EQUIPMENU_BACKGROUND_FBPNUM,
      gpGlobals->f.fpFBP);

   if (gConfig.fUseCustomScreenLayout)
   {
      int x = PAL_X(gConfig.ScreenLayout.EquipImageBox);
      int y = PAL_Y(gConfig.ScreenLayout.EquipImageBox);
      for (i = 8; i < 72; i++)
      {
         memcpy(&bufBackground[i * 320 + 92], &bufBackground[(i + 128) * 320 + 92], 32);
         memcpy(&bufBackground[(i + 64) * 320 + 92], &bufBackground[(i + 128) * 320 + 92], 32);
      }
      for (i = 9; i < 90; i++)
      {
         memcpy(&bufBackground[i * 320 + 226], &bufBackground[(i + 104) * 320 + 226], 32);
      }
      for (i = 99; i < 113; i++)
      {
         memcpy(&bufBackground[i * 320 + 226], &bufBackground[(i + 16) * 320 + 226], 32);
      }
      for (i = 8; i < 80; i++)
      {
         memcpy(&bufImageBox[(i - 8) * 72], &bufBackground[i * 320 + 8], 72);
         memcpy(&bufBackground[i * 320 + 8], &bufBackground[(i + 72) * 320 + 8], 72);
      }
      for (i = 0; i < 72; i++)
      {
         memcpy(&bufBackground[(i + y) * 320 + x], &bufImageBox[i * 72], 72);
      }
   }

   iCurrentPlayer = 0;
   bSelectedColor = MENUITEM_COLOR_SELECTED_FIRST;
   dwColorChangeTime = SDL_GetTicks() + (600 / MENUITEM_COLOR_SELECTED_TOTALNUM);

   while (TRUE)
   {
      wItem = gpGlobals->wLastUnequippedItem;

      //
      // Draw the background
      //
      PAL_FBPBlitToSurface(bufBackground, gpScreen);

      //
      // Draw the item picture
      //
      if (PAL_MKFReadChunk(bufImage, 2048,
         gpGlobals->g.rgObject[wItem].item.wBitmap, gpGlobals->f.fpBALL) > 0)
      {
         PAL_RLEBlitToSurface(bufImage, gpScreen, PAL_XY_OFFSET(gConfig.ScreenLayout.EquipImageBox, 8, 8));
      }

      if (gConfig.fUseCustomScreenLayout)
      {
         int labels1[] = { STATUS_LABEL_ATTACKPOWER, STATUS_LABEL_MAGICPOWER, STATUS_LABEL_RESISTANCE, STATUS_LABEL_DEXTERITY, STATUS_LABEL_FLEERATE };
         int labels2[] = { EQUIP_LABEL_HEAD, EQUIP_LABEL_SHOULDER, EQUIP_LABEL_BODY, EQUIP_LABEL_HAND, EQUIP_LABEL_FOOT, EQUIP_LABEL_NECK };
		 for (i = 0; i < sizeof(labels1) / sizeof(int); i++)
         {
            int index = &gConfig.ScreenLayout.EquipStatusLabels[i] - gConfig.ScreenLayoutArray;
            BOOL fShadow = (gConfig.ScreenLayoutFlag[index] & DISABLE_SHADOW) ? FALSE : TRUE;
            BOOL fUse8x8Font = (gConfig.ScreenLayoutFlag[index] & USE_8x8_FONT) ? TRUE : FALSE;
            PAL_DrawText(PAL_GetWord(labels1[i]), gConfig.ScreenLayoutArray[index], MENUITEM_COLOR, fShadow, FALSE, fUse8x8Font);
         }
		 for (i = 0; i < sizeof(labels2) / sizeof(int); i++)
         {
            int index = &gConfig.ScreenLayout.EquipLabels[i] - gConfig.ScreenLayoutArray;
            BOOL fShadow = (gConfig.ScreenLayoutFlag[index] & DISABLE_SHADOW) ? FALSE : TRUE;
            BOOL fUse8x8Font = (gConfig.ScreenLayoutFlag[index] & USE_8x8_FONT) ? TRUE : FALSE;
            PAL_DrawText(PAL_GetWord(labels2[i]), gConfig.ScreenLayoutArray[index], MENUITEM_COLOR, fShadow, FALSE, fUse8x8Font);
         }
      }

      //
      // Draw the current equipment of the selected player
      //
      w = gpGlobals->rgParty[iCurrentPlayer].wPlayerRole;
      for (i = 0; i < MAX_PLAYER_EQUIPMENTS; i++)
      {
         if (gpGlobals->g.PlayerRoles.rgwEquipment[i][w] != 0)
         {
            PAL_DrawText(PAL_GetWord(gpGlobals->g.PlayerRoles.rgwEquipment[i][w]),
				gConfig.ScreenLayout.EquipNames[i], MENUITEM_COLOR, TRUE, FALSE, FALSE);
         }
      }

      //
      // Draw the stats of the currently selected player
      //
      PAL_DrawNumber(PAL_GetPlayerAttackStrength(w), 4, gConfig.ScreenLayout.EquipStatusValues[0], kNumColorCyan, kNumAlignRight);
      PAL_DrawNumber(PAL_GetPlayerMagicStrength(w), 4, gConfig.ScreenLayout.EquipStatusValues[1], kNumColorCyan, kNumAlignRight);
      PAL_DrawNumber(PAL_GetPlayerDefense(w), 4, gConfig.ScreenLayout.EquipStatusValues[2], kNumColorCyan, kNumAlignRight);
      PAL_DrawNumber(PAL_GetPlayerDexterity(w), 4, gConfig.ScreenLayout.EquipStatusValues[3], kNumColorCyan, kNumAlignRight);
      PAL_DrawNumber(PAL_GetPlayerFleeRate(w), 4, gConfig.ScreenLayout.EquipStatusValues[4], kNumColorCyan, kNumAlignRight);

      //
      // Draw a box for player selection
      //
      PAL_CreateBox(gConfig.ScreenLayout.EquipRoleListBox, gpGlobals->wMaxPartyMemberIndex, PAL_WordMaxWidth(36, 4) - 1, 0, FALSE);

      //
      // Draw the label of players
      //
      for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
      {
         w = gpGlobals->rgParty[i].wPlayerRole;

         if (iCurrentPlayer == i)
         {
            if (gpGlobals->g.rgObject[wItem].item.wFlags & (kItemFlagEquipableByPlayerRole_First << w))
            {
               bColor = bSelectedColor;
            }
            else
            {
               bColor = MENUITEM_COLOR_SELECTED_INACTIVE;
            }
         }
         else
         {
            if (gpGlobals->g.rgObject[wItem].item.wFlags & (kItemFlagEquipableByPlayerRole_First << w))
            {
               bColor = MENUITEM_COLOR;
            }
            else
            {
               bColor = MENUITEM_COLOR_INACTIVE;
            }
         }

         PAL_DrawText(PAL_GetWord(gpGlobals->g.PlayerRoles.rgwName[w]),
            PAL_XY_OFFSET(gConfig.ScreenLayout.EquipRoleListBox, 13, 13 + 18 * i), bColor, TRUE, FALSE, FALSE);
      }

      //
      // Draw the text label and amount of the item
      //
      if (wItem != 0)
      {
         PAL_DrawText(PAL_GetWord(wItem), gConfig.ScreenLayout.EquipItemName, MENUITEM_COLOR_CONFIRMED, TRUE, FALSE, FALSE);
         PAL_DrawNumber(PAL_GetItemAmount(wItem), 2, gConfig.ScreenLayout.EquipItemAmount, kNumColorCyan, kNumAlignRight);
      }

      //
      // Update the screen
      //
      VIDEO_UpdateScreen(NULL);

      //
      // Accept input
      //
      PAL_ClearKeyState();

      while (TRUE)
      {
         PAL_ProcessEvent();

         //
         // See if we should change the highlight color
         //
         if (SDL_TICKS_PASSED(SDL_GetTicks(), dwColorChangeTime))
         {
            if ((WORD)bSelectedColor + 1 >=
               (WORD)MENUITEM_COLOR_SELECTED_FIRST + MENUITEM_COLOR_SELECTED_TOTALNUM)
            {
               bSelectedColor = MENUITEM_COLOR_SELECTED_FIRST;
            }
            else
            {
               bSelectedColor++;
            }

            dwColorChangeTime = SDL_GetTicks() + (600 / MENUITEM_COLOR_SELECTED_TOTALNUM);

            //
            // Redraw the selected item if needed.
            //
            w = gpGlobals->rgParty[iCurrentPlayer].wPlayerRole;

            if (gpGlobals->g.rgObject[wItem].item.wFlags & (kItemFlagEquipableByPlayerRole_First << w))
            {
               PAL_DrawText(PAL_GetWord(gpGlobals->g.PlayerRoles.rgwName[w]),
                  PAL_XY_OFFSET(gConfig.ScreenLayout.EquipRoleListBox, 13, 13 + 18 * iCurrentPlayer), bSelectedColor, TRUE, TRUE, FALSE);
            }
         }

         if (g_InputState.dwKeyPress != 0)
         {
            break;
         }

         SDL_Delay(1);
      }

      if (wItem == 0)
      {
         return;
      }

      if (g_InputState.dwKeyPress & (kKeyUp | kKeyLeft))
      {
         iCurrentPlayer--;
         if (iCurrentPlayer < 0)
         {
            iCurrentPlayer = gpGlobals->wMaxPartyMemberIndex;
         }
      }
      else if (g_InputState.dwKeyPress & (kKeyDown | kKeyRight))
      {
         iCurrentPlayer++;
         if (iCurrentPlayer > gpGlobals->wMaxPartyMemberIndex)
         {
            iCurrentPlayer = 0;
         }
      }
      else if (g_InputState.dwKeyPress & kKeyMenu)
      {
         return;
      }
      else if (g_InputState.dwKeyPress & kKeySearch)
      {
         w = gpGlobals->rgParty[iCurrentPlayer].wPlayerRole;

         if (gpGlobals->g.rgObject[wItem].item.wFlags & (kItemFlagEquipableByPlayerRole_First << w))
         {
            //
            // Run the equip script
            //
            gpGlobals->g.rgObject[wItem].item.wScriptOnEquip =
               PAL_RunTriggerScript(gpGlobals->g.rgObject[wItem].item.wScriptOnEquip,
                  gpGlobals->rgParty[iCurrentPlayer].wPlayerRole);
         }
      }
   }
}

VOID
PAL_QuitGame(
   VOID
)
{
#if PAL_HAS_CONFIG_PAGE
	WORD wReturnValue = PAL_TripleMenu(SYSMENU_LABEL_LAUNCHSETTING);
#else
	WORD wReturnValue = PAL_ConfirmMenu(); // No config menu available
#endif
	if (wReturnValue == 1 || wReturnValue == 2)
	{
		if (wReturnValue == 2) gConfig.fLaunchSetting = TRUE;
		PAL_SaveConfig();		// Keep the fullscreen state
		AUDIO_PlayMusic(0, FALSE, 2);
		PAL_FadeOut(2);
		PAL_Shutdown(0);
	}
}
