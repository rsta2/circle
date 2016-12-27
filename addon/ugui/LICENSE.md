/* -------------------------------------------------------------------------------- */
/* -- µGUI - Generic GUI module (C)Achim Döbler, 2015                            -- */
/* -------------------------------------------------------------------------------- */
// µGUI is a generic GUI module for embedded systems.
// This is a free software that is open for education, research and commercial
// developments under license policy of following terms.
//
//  Copyright (C) 2015, Achim Döbler, all rights reserved.
//  URL: http://www.embeddedlightning.com/
//
// * The µGUI module is a free software and there is NO WARRANTY.
// * No restriction on use. You can use, modify and redistribute it for
//   personal, non-profit or commercial products UNDER YOUR RESPONSIBILITY.
// * Redistributions of source code must retain the above copyright notice.
//
/* -------------------------------------------------------------------------------- */
/* -- MY SPECIAL THANKS GO TO                                                    -- */
/* -------------------------------------------------------------------------------- */
// Andrey Filimonov (-->https://github.com/Sermus)
// for giving valuable suggestions, reporting bugs and adding several new features.
// Andrey also put a lot of work in the implementaion of anti-aliased font support. 
//
// Mikhail Podkur (-->https://github.com/MikhailPodkur)
// for adding cyrillic 8x12 font, checkbox feature and RGB565 support.
//
// Gustavo Denardin
// for giving valuable suggestions regarding real-time os support.
//
// Samuel Kleiser
// for reporting bugs and giving examples how to improve µGUI.
/* -------------------------------------------------------------------------------- */
/* -- REVISION HISTORY                                                           -- */
/* -------------------------------------------------------------------------------- */
//  Dec 20, 2015  V0.31 Checkbox component with all funtions added.
//                      Cyrillic font 8x12 added.
//                      RGB565 color schema added.
//                      Windows components font could be getted from current GUI by default
//  Mar 18, 2015  V0.3  Driver support added.
//                      Window and object support added.
//                      Touch support added.
//                      Fixed some minor bugs.
//
//  Oct 20, 2014  V0.2  Function UG_DrawRoundFrame() added.
//                      Function UG_FillRoundFrame() added.
//                      Function UG_DrawArc() added.
//                      Fixed some minor bugs.
//
//  Oct 11, 2014  V0.1  First release.
/* -------------------------------------------------------------------------------- */
