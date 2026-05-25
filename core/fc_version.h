/**
 * @file fc_version.h
 * @author fool-cat (2696652257@qq.com)
 * @brief fc_embed library version definition — single source of truth.
 * @version 1.0.0
 * @date 2025-12-06
 *
 * @copyright Copyright (c) 2025
 *
 * HOW TO BUMP THE VERSION
 * -----------------------
 * 1. Change FC_EMBED_VERSION_MAJOR / MINOR / PATCH below.
 * 2. Run the version-bump skill (or follow its checklist manually) to
 *    propagate the new version into fool_cat.fc_embed.pdsc:
 *      - <release version="...">  (newest entry at top)
 *      - every <component Cversion="...">
 *      - every <file attr="config" version="...">
 * 3. Validate XML:
 *      python -c "import xml.etree.ElementTree as ET; \
 *                 ET.parse('fool_cat.fc_embed.pdsc'); print('XML OK')"
 */

#ifndef __FC_VERSION_H__
#define __FC_VERSION_H__

// clang-format off

/** Major version — increment on breaking API changes. */
#define FC_EMBED_VERSION_MAJOR  2

/** Minor version — increment when adding backward-compatible features. */
#define FC_EMBED_VERSION_MINOR  0

/** Patch version — increment for bug fixes and documentation corrections. */
#define FC_EMBED_VERSION_PATCH  0

/** Composite integer: 0xMMmmPP (e.g. 0x010000 for 1.0.0). */
#define FC_EMBED_VERSION \
    ((FC_EMBED_VERSION_MAJOR << 16) | \
     (FC_EMBED_VERSION_MINOR <<  8) | \
     (FC_EMBED_VERSION_PATCH))

/** Human-readable string, e.g. "1.0.0". */
#define FC_EMBED_VERSION_STRING \
    FC_EMBED_STRINGIFY(FC_EMBED_VERSION_MAJOR) "." \
    FC_EMBED_STRINGIFY(FC_EMBED_VERSION_MINOR) "." \
    FC_EMBED_STRINGIFY(FC_EMBED_VERSION_PATCH)

/* Internal stringify helpers — do not use directly. */
#define FC_EMBED_STRINGIFY(x)   FC_EMBED_STRINGIFY_(x)
#define FC_EMBED_STRINGIFY_(x)  #x

// clang-format on

#endif /* __FC_VERSION_H__ */
