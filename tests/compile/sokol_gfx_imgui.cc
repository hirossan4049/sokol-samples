#include "sokol_app.h"
#include "sokol_gfx.h"
#include "imgui.h"
#include "sokol_imgui.h"
#define SOKOL_IMPL
#include "sokol_gfx_imgui.h"

void use_gfx_imgui_impl() {
    sg_imgui_init({});
}

