local camera = {
    fov = 80,
    near = 0.1,
    position = {-1.0, 1.5, -1.0},
    rotation = {5.0, 15.0, 0.0},
}

local light = {
    direction = {2, -1, 1}
}

local model = {
    name = "Dragon",
    file = "assets/models/conference/conference.obj",
    position = {0.0, 0.0, 0.0},
    rotation = {0.0, 0.0, 0.0},
    scale = 0.005
}

return { Camera = camera, Light = light, Model = model }