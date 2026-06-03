local camera = {
    fov = 80,
    near = 0.1,
    position = {0.0, 8.0, 0.0},
    rotation = {0.0, 0.0, 0.0},
}

local light = {
    direction = {2, -1, 1}
}

local shadow_config = {
    max_shadow_distance = 38.0,
    csm_cascade_count = 4
}

local model = {
    name = "Minecraft",
    file = "assets/models/minecraft/minecraft.obj",
    position = {0.0, 0.0, 0.0},
    rotation = {0.0, 0.0, 0.0},
    scale = 15
}

return {
    Camera = camera,
    Light = light,
    ShadowConfig = shadow_config,
    Model = model,
}