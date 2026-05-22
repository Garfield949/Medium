local camera = {
    fov = 80,
    near = 0.1,
    position = {0.0, 0.0, 0.0},
    rotation = {0.0, 0.0, 0.0},
}

local light = {
    direction = {2, -1, 1}
}

local model = {
    name = "Dragon",
    file = "assets/models/dragon.obj",
    position = {0.0, 0.0, 0.0},
    rotation = {0.0, 0.0, 0.0},
    scale = 0.3
}

return { Camera = camera, Light = light, Model = model }