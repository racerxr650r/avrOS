#!/usr/bin/env bash
# Insert the avrOS file header into a new .c / .h file.
# Usage: util/scripts/stamp_license.sh <file.c|file.h> ["<description>"]
#
# Environment:
#   AVROS_AUTHOR  Override author name (defaults to `git config user.name`)
#   AVROS_EMAIL   Override author email (defaults to `git config user.email`)
set -euo pipefail

file="${1:?usage: stamp_license.sh <file> [description]}"
desc="${2:-FIXME: one-line description}"
author="${AVROS_AUTHOR:-$(git config user.name)}"
email="${AVROS_EMAIL:-$(git config user.email)}"
year="$(date +%Y)"
created="$(date +%m/%d/%Y)"
base="$(basename "$file")"

if [[ -e "$file" ]]; then
    echo "Refusing to overwrite existing $file" >&2
    exit 1
fi

case "$file" in
    *.h)
        guard="$(echo "$base" | tr '[:lower:].' '[:upper:]_')_"
        cat > "$file" <<EOF
/**
 * @file $base
 * @brief $desc
 *
 * Created: $created
 * Author: $author
 *
 * Copyright (C) $year by $author <$email>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef $guard
#define $guard

#include "avrOS.h"

#endif /* $guard */
EOF
        ;;
    *.c)
        cat > "$file" <<EOF
/*
 * $base
 *
 * $desc
 *
 * Created: $created
 * Author : $author
 *
 * Copyright (C) $year by $author <$email>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "avrOS.h"
EOF
        ;;
    *)
        echo "Unsupported extension: $file" >&2
        exit 2
        ;;
esac

echo "Stamped $file"
