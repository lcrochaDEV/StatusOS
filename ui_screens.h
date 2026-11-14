#ifndef UI_SCREENS_H
#define UI_SCREENS_H

#include <lvgl.h>

// Apenas um wrapper simples para gerenciar as duas telas no mesmo objeto
// para evitar mudanças estruturais grandes no .ino

inline lv_obj_t* g_screen_main = nullptr;
inline lv_obj_t* g_screen_wol = nullptr;
inline lv_obj_t* g_screen_crypto = nullptr;

#endif
