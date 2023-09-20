//
// Created by jackn on 15/09/2023.
//

#include "rendering_framework.h"

static Framework gFramework = Framework::NONE;

namespace Rendering {
    void SetFramework(Framework renderingBackend) {
        gFramework = renderingBackend;
    }

    Framework GetFramework() {
        return gFramework;
    }

}