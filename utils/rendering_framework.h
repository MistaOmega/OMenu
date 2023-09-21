//
// Created by jackn on 15/09/2023.
//

#ifndef OMENU_RENDERING_FRAMEWORK
#define OMENU_RENDERING_FRAMEWORK

enum Framework {
    NONE = 0,
    DIRECTX9,
    DIRECTX12,
    OPENGL
};

namespace Rendering {
    void SetFramework(Framework backend);

    Framework GetFramework();
}

#endif //OMENU_RENDERING_FRAMEWORK
