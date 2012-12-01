/***************************************************************************
                          messagebox.h  -  description
                             -------------------
    begin                : Tue Nov 12 2002
    copyright            : (C) 2002 by blight
    email                : blight@Ashitaka
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef __MESSAGEBOX_H__
#define __MESSAGEBOX_H__

/** flags **/
// The message box contains three push buttons: Abort, Retry, and Ignore.
#define MB_ABORTRETRYIGNORE     (0x00000001)
// Microsoft® Windows® 2000/XP: The message box contains three push buttons: Cancel, Try Again, Continue. Use this message box type instead of MB_ABORTRETRYIGNORE.
#define MB_CANCELTRYCONTINUE        (0x00000002)
// The message box contains one push button: OK. This is the default.
#define MB_OK               (0x00000004)
// The message box contains two push buttons: OK and Cancel.
#define MB_OKCANCEL         (0x00000008)
// The message box contains two push buttons: Retry and Cancel.
#define MB_RETRYCANCEL          (0x00000010)
// The message box contains two push buttons: Yes and No.
#define MB_YESNO            (0x00000020)
// The message box contains three push buttons: Yes, No, and Cancel.
#define MB_YESNOCANCEL          (0x00000040)

// An exclamation-point icon appears in the message box.
#define MB_ICONEXCLAMATION      (0x00000100)
#define MB_ICONWARNING          (0x00000100)
// An icon consisting of a lowercase letter i in a circle appears in the message box.
#define MB_ICONINFORMATION      (0x00000200)
#define MB_ICONASTERISK         (0x00000200)
// A question-mark icon appears in the message box.
#define MB_ICONQUESTION         (0x00000400)
// A stop-sign icon appears in the message box.
#define MB_ICONSTOP         (0x00000800)
#define MB_ICONERROR        (0x00000800)
#define MB_ICONHAND         (0x00000800)

#ifdef __cplusplus
extern "C"
{
#endif
   // returns 1 if the first button was clicked, 2 for the second and 3 for the third
   int messagebox( const char *title, int flags, const char *fmt, ... );
#ifdef __cplusplus
}
#endif

#endif  // __MESSAGEBOX_H__

