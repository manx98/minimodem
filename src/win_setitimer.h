//
// Created by Manx98 on 2024/8/13.
//

#ifndef SIMPLEAUDIO_WIN_SETITIMER_H
#define SIMPLEAUDIO_WIN_SETITIMER_H

#ifdef __cplusplus
extern "C" {
#endif
void init_win_setitimer();

void win_setitimer(long int delay, void (*handler)());
#ifdef __cplusplus
}
#endif
#endif
