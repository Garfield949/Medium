local camera = {
    fov = 50,
    position = { 3.0, 1.3, -3.5 },
    rotation = { 10.0, -35.0, 0.0 },
}

local light = {
    direction = { -2, -1, 1 }
}

local shadow_config = {
    max_shadow_distance = 10.0,
    csm_cascade_count = 3
}

local model = {
    name = "Porsche",
    file = "assets/models/porsche/porsche.obj",
    position = { 0.0, 0.0, 0.0 },
    rotation = { 0.0, 0.0, 0.0 },
    scale = 1.0
}

return {
    Camera = camera,
    Light = light,
    ShadowConfig = shadow_config,
    Model = model,
}
