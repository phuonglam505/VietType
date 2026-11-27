/* Header C cho FFI viettype-engine (API theo instance) */
#ifndef VIETTYPE_ENGINE_H
#define VIETTYPE_ENGINE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Handle engine opaque */
typedef struct VietTypeEngine VietTypeEngine;

/* Tạo/hủy */
VietTypeEngine *viettype_engine_new(void);
void viettype_engine_free(VietTypeEngine *engine);

/* Xử lý phím. Trả 1 nếu xử lý, 0 nếu bỏ qua.
 * out_buf: nhận byte đầu ra UTF-8
 * out_len: dung lượng out_buf
 * actual_len: nhận số byte thực tế ghi
 * backspaces: nhận số backspace cần gửi trước đầu ra
 * backspaces_bytes: nhận số backspace tính theo byte cho bảng mã đa byte (có thể NULL) */
int viettype_engine_process(VietTypeEngine *engine, uint32_t key_code,
                        uint8_t *out_buf, size_t out_len,
                        size_t *actual_len, size_t *backspaces,
                        size_t *backspaces_bytes);

/* Xử lý backspace. Ngữ nghĩa đầu ra giống viettype_engine_process. */
int viettype_engine_backspace(VietTypeEngine *engine,
                          uint8_t *out_buf, size_t out_len,
                          size_t *actual_len, size_t *backspaces,
                          size_t *backspaces_bytes);

/* Đặt lại trạng thái engine (vd: khi đổi focus) */
void viettype_engine_reset(VietTypeEngine *engine);

/* Đặt lại mềm: lưu trạng thái để backspace có thể khôi phục */
void viettype_engine_soft_reset(VietTypeEngine *engine);

/* Nạp ngữ cảnh (surrounding text) vào engine để khôi phục trạng thái. */
int viettype_engine_feed_context(VietTypeEngine *engine, const char *text);

/* Đặt kiểu gõ: 0=Telex, 1=SimpleTelex, 2=VNI, 3=VIQR, 4=MsVi */
void viettype_engine_set_input_method(VietTypeEngine *engine, int method);

/* Bật(1) / tắt(0) chế độ tiếng Việt */
void viettype_engine_set_viet_mode(VietTypeEngine *engine, int enabled);

/* Đặt tùy chọn engine */
void viettype_engine_set_options(VietTypeEngine *engine,
                             int free_marking, int modern_style,
                             int spell_check, int auto_restore,
                             int ede_mode, int macro_enabled);

/* Trả 1 nếu ở đầu từ */
int viettype_engine_at_word_beginning(VietTypeEngine *engine);

/* ==================== Macro (gõ tắt) ==================== */
int viettype_engine_add_macro(VietTypeEngine *engine, const char *key, const char *value);
int viettype_engine_remove_macro(VietTypeEngine *engine, const char *key);
void viettype_engine_clear_macros(VietTypeEngine *engine);
void viettype_engine_load_macros(VietTypeEngine *engine, const char *text);
char *viettype_engine_save_macros(VietTypeEngine *engine);
void viettype_engine_free_string(char *s);

/* Chuyển UTF-8 sang bảng mã đích. */
int viettype_charset_from_utf8(const uint8_t *input, size_t input_len,
                           int charset_id,
                           uint8_t *out_buf, size_t out_len,
                           size_t *actual_len);

/* Chuyển byte từ bảng mã nguồn sang UTF-8. */
int viettype_charset_to_utf8(const uint8_t *input, size_t input_len,
                         int charset_id,
                         uint8_t *out_buf, size_t out_len,
                         size_t *actual_len);

#ifdef __cplusplus
}
#endif

#endif /* VIETTYPE_ENGINE_H */
