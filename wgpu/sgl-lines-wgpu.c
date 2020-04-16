//------------------------------------------------------------------------------
//  sgl-lines-wgpu.c
//  Line rendering with sokol_gl.h
//------------------------------------------------------------------------------
#define SOKOL_IMPL
#define SOKOL_WGPU
#include "sokol_gfx.h"
#include "wgpu_entry.h"
#define SOKOL_GL_IMPL
#include "sokol_gl.h"

#define SAMPLE_COUNT (4)

static struct {
    sg_pass_action pass_action;
    sgl_pipeline depth_test_pip;
} state;

static void init(void) {
    sg_setup(&(sg_desc){
        .context = {
            .color_format = wgpu_get_color_format(),
            .wgpu = {
                .device = wgpu_get_device(),
                .render_view_cb = wgpu_get_render_view,
                .resolve_view_cb = wgpu_get_resolve_view,
                .depth_stencil_view_cb = wgpu_get_depth_stencil_view
            }
        }
    });

    /* setup sokol-gl */
    sgl_setup(&(sgl_desc_t){
        .sample_count = SAMPLE_COUNT
    });

    /* a pipeline object with less-equal depth-testing */
    state.depth_test_pip = sgl_make_pipeline(&(sg_pipeline_desc){
        .depth_stencil = {
            .depth_write_enabled = true,
            .depth_compare_func = SG_COMPAREFUNC_LESS_EQUAL
        }
    });

    /* a default pass action */
    state.pass_action = (sg_pass_action) {
        .colors[0] = {
            .action = SG_ACTION_CLEAR,
            .val = { 0.0f, 0.0f, 0.0f, 1.0f }
        }
    };
}

static void grid(float y, uint32_t frame_count) {
    const int num = 64;
    const float dist = 4.0f;
    const float z_offset = (dist / 8) * (frame_count & 7);
    sgl_begin_lines();
    for (int i = 0; i < num; i++) {
        float x = i * dist - num * dist * 0.5f;
        sgl_v3f(x, y, -num * dist);
        sgl_v3f(x, y, 0.0f);
    }
    for (int i = 0; i < num; i++) {
        float z = z_offset + i * dist - num * dist;
        sgl_v3f(-num * dist * 0.5f, y, z);
        sgl_v3f(num * dist * 0.5f, y, z);
    }
    sgl_end();
}

static void floaty_thingy(uint32_t frame_count) {
    const uint32_t num_segs = 32;
    uint32_t start = frame_count % (num_segs * 2);
    if (start < num_segs) {
        start = 0;
    }
    else {
        start -= num_segs;
    }
    uint32_t end = frame_count % (num_segs * 2);
    if (end > num_segs) {
        end = num_segs;
    }
    const float dx = 0.25f;
    const float dy = 0.25f;
    const float x0 = -(num_segs * dx * 0.5f);
    const float x1 = -x0;
    const float y0 = -(num_segs * dy * 0.5f);
    const float y1 = -y0;
    sgl_begin_lines();
    for (uint32_t i = start; i < end; i++) {
        float x = i * dx;
        float y = i * dy;
        sgl_v2f(x0 + x, y0); sgl_v2f(x1, y0 + y);
        sgl_v2f(x1 - x, y1); sgl_v2f(x0, y1 - y);
        sgl_v2f(x0 + x, y1); sgl_v2f(x1, y1 - y);
        sgl_v2f(x1 - x, y0); sgl_v2f(x0, y0 + y);
    }
    sgl_end();
}

static inline uint32_t xorshift32(void) {
    static uint32_t x = 0x12345678;
    x ^= x<<13;
    x ^= x>>17;
    x ^= x<<5;
    return x;
}

static inline float rnd(void) {
    return (((float)(xorshift32() & 0xFFFF)) / 0x10000) * 2.0f - 1.0f;
}

#define RING_NUM (1024)
#define RING_MASK (RING_NUM-1)
static void hairball(uint32_t frame_count) {
    static float ring[RING_NUM][6];
    static uint32_t head = 0;

    float vx = rnd();
    float vy = rnd();
    float vz = rnd();
    float r = (rnd() + 1.0f) * 0.5f;
    float g = (rnd() + 1.0f) * 0.5f;
    float b = (rnd() + 1.0f) * 0.5f;
    float x = ring[head][0];
    float y = ring[head][1];
    float z = ring[head][2];
    head = (head + 1) & RING_MASK;
    ring[head][0] = x * 0.9f + vx;
    ring[head][1] = y * 0.9f + vy;
    ring[head][2] = z * 0.9f + vz;
    ring[head][3] = r;
    ring[head][4] = g;
    ring[head][5] = b;

    sgl_begin_line_strip();
    for (uint32_t i = (head + 1) & RING_MASK; i != head; i = (i + 1) & RING_MASK) {
        sgl_c3f(ring[i][3], ring[i][4], ring[i][5]);
        sgl_v3f(ring[i][0], ring[i][1], ring[i][2]);
    }
    sgl_end();
}

static void frame(void) {
    const float aspect = (float)wgpu_width() / (float)wgpu_height();
    static uint32_t frame_count = 0;
    frame_count++;

    sgl_defaults();
    sgl_push_pipeline();
    sgl_load_pipeline(state.depth_test_pip);
    sgl_matrix_mode_projection();
    sgl_perspective(sgl_rad(45.0f), aspect, 0.1f, 1000.0f);
    sgl_matrix_mode_modelview();
    sgl_translate(sinf(frame_count * 0.02f) * 16.0f, sinf(frame_count * 0.01f) * 4.0f, 0.0f);
    sgl_c3f(1.0f, 0.0f, 1.0f);
    grid(-7.0f, frame_count);
    grid(+7.0f, frame_count);
    sgl_push_matrix();
        sgl_translate(0.0f, 0.0f, -30.0f);
        sgl_rotate(frame_count * 0.05f, 0.0f, 1.0f, 1.0f);
        sgl_c3f(1.0f, 1.0f, 0.0);
        floaty_thingy(frame_count);
    sgl_pop_matrix();
    sgl_push_matrix();
        sgl_translate(-sinf(frame_count * 0.02f) * 32.0f, 0.0f, -70.0f + cosf(frame_count * 0.01f) * 50.0f);
        sgl_rotate(frame_count * 0.05f, 0.0f, -1.0f, 1.0f);
        sgl_c3f(0.0f, 1.0f, 0.0f);
        floaty_thingy(frame_count + 32);
    sgl_pop_matrix();
    sgl_push_matrix();
        sgl_translate(-sinf(frame_count * 0.02f) * 16.0f, 0.0f, -30.0f);
        sgl_rotate(frame_count * 0.01f, sinf(frame_count * 0.005f), 0.0f, 1.0f);
        sgl_c3f(0.5f, 1.0f, 0.0f);
        hairball(frame_count);
    sgl_pop_matrix();
    sgl_pop_pipeline();

    /* sokol-gfx default pass with the actual sokol-gl drawing */
    sg_begin_default_pass(&state.pass_action, wgpu_width(), wgpu_height());
    sgl_draw();
    sg_end_pass();
    sg_commit();
}

static void shutdown(void) {
    sgl_shutdown();
    sg_shutdown();
}

int main() {
    wgpu_start(&(wgpu_desc_t){
        .init_cb = init,
        .frame_cb = frame,
        .shutdown_cb = shutdown,
        .sample_count = SAMPLE_COUNT,
        .width = 512,
        .height = 512,
        .title = "sgl-lines-wgpu"
    });
    return 0;
}