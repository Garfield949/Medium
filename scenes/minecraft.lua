local camera = {
    fov = 80,
    position = {0.0, 12.0, 0.0},
    rotation = {0.0, 135.0, 0.0},
}

local light = {
    direction = {2, -1, 1}
}

local shadow_config = {
    max_shadow_distance = 35.0,
    csm_cascade_count = 3
}

local model = {
    name = "Empire",
    file = "assets/models/minecraft/empire.obj",
    position = {0.0, 0.0, 0.0},
    rotation = {0.0, 0.0, 0.0},
    scale = 0.3
}

return {
    Camera = camera,
    Light = light,
    ShadowConfig = shadow_config,
    Model = model,
}