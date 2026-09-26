#pragma once
// owen.h — backend redirect hook (Owen.c port).
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Install the Epic-services → backend redirect. All offsets are baked
// per-version; pass 0 for the EOS set on versions that don't need it.
// Returns 0 on success.
int owen_install(uintptr_t engine_base,
                 const char* backend_url,
                 uint32_t ue_pr, uint16_t ue_geturl, uint32_t ue_seturl,
                 uint32_t eos_pr, uint16_t eos_geturl, uint32_t eos_seturl,
                 const char* version_string);

#ifdef __cplusplus
}
#endif
