#include "../runtime/src/jni_bridge.c"
#include <assert.h>

int nfsmw_display_width(void) { return 720; }
int nfsmw_display_height(void) { return 480; }

static size_t ink_count(const struct fake_object *graphics, int *rightmost)
{
    const struct fake_object *bitmap = as_object(graphics->elements[0]);
    const unsigned char *pixels = bitmap->bytes;
    size_t count = 0U;
    *rightmost = -1;
    for (size_t i = 0U; i < bitmap->length; i += 4U) {
        if (pixels[i + 3U] != 0U) {
            int x = (int)((i / 4U) % (size_t)bitmap->width);
            if (x > *rightmost) *rightmost = x;
            ++count;
        }
    }
    return count;
}

int main(void)
{
    const float sizes[] = {7.0F, 13.0F, 14.0F, 20.0F, 56.0F, 80.0F};
    struct fake_object *text = new_string("WW");
    void *measure = register_method("measureText", "(Ljava/lang/String;)F", 0);
    void *ascent = register_field("ascent", "I", 0);
    void *descent = register_field("descent", "I", 0);
    for (size_t i = 0U; i < sizeof(sizes) / sizeof(sizes[0]); ++i) {
        struct fake_object *paint = new_paint(sizes[i]);
        union fake_jvalue arguments[1];
        arguments[0].object_value = text;
        float width = jni_call_float_method(NULL, paint, measure, text);
        assert(width == jni_call_float_method_a(NULL, paint, measure,
                                                (const uintptr_t *)arguments));
        int top = jni_get_int_field(NULL, paint, ascent);
        int bottom = jni_get_int_field(NULL, paint, descent);
        struct fake_object *tight = new_bitmap_graphics((int)width, bottom - top);
        struct fake_object *large = new_bitmap_graphics(256, 256);
        draw_text(tight, paint, "WW", 0, -top);
        draw_text(large, paint, "WW", 0, 128);
        int rightmost, unused;
        size_t count = ink_count(tight, &rightmost);
        assert(count > 0U && count == ink_count(large, &unused));
        /* Two glyphs occupy 11 columns; the final column is spacing. */
        assert((float)(rightmost + 1) == width * 11.0F / 12.0F);
    }
    return 0;
}
