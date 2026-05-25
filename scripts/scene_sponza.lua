local camera = {
    fov = 80,
    near = 0.1,
    position = {0.0, 1.0, 0.0},
    rotation = {0.0, -90.0, 0.0},
}

local light = {
    direction = {-0.05, -0.3, 0.0},
    max_shadow_dist = 15.0
}

local model = {
    name = "Crytek Sponza",
    file = "assets/models/sponza/sponza.obj",
    position = {0.0, 0.0, 0.0},
    rotation = {0.0, 90.0, 0.0},
    scale = 0.006
}

return { Camera = camera, Light = light, Model = model }