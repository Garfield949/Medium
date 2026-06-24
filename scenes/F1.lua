local camera = {
    fov = 80,
    near = 0.1,
    position = { -2.1, 1.5, -1.4 },
    rotation = { 42.0, 45.0, 0.0 },
}

local light = {
    direction = { -2.0, -2.0, 2.0 }
}

local shadow_config = {
    max_shadow_distance = 5.0,
    csm_cascade_count = 3
}

local model = {
    name = "F1",
    file = "assets/models/F1/F1.obj",
    position = { 0.0, 0.0, 0.0 },
    rotation = { 0.0, 0.0, 0.0 },
    scale = 0.01
}

return {
    Camera = camera,
    Light = light,
    ShadowConfig = shadow_config,
    Model = model,
}
