local camera = {
    fov = 80,
    near = 0.1,
    position = {0.0, 0.9, -1.0},
    rotation = {30.0, 0.0, 0.0},
}

local light = {
    direction = {2, -1, 1}
}

local shadow_config = {
    max_shadow_distance = 5.0,
    csm_cascade_count = 2
}

local model = {
    name = "Dragon",
    file = "assets/models/teapot/teapot.obj",
    position = {0.0, 0.0, 0.0},
    rotation = {0.0, 0.0, 0.0},
    scale = 0.01
}

return {
    Camera = camera,
    Light = light,
    ShadowConfig = shadow_config,
    Model = model,
}