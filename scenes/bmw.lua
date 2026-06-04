local camera = {
    fov = 70,
    near = 0.1,
    position = { -2.7, 1.5, -3.0 },
    rotation = { 18.0, 38.0, 0.0 },
}

local light = {
    direction = { 1.0, -1.0, -2.0 }
}

local shadow_config = {
    max_shadow_distance = 10.0,
    csm_cascade_count = 2
}

local model = {
    name = "Bmw",
    file = "assets/models/bmw/bmw.obj",
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
