local camera = {
    fov = 80,
    near = 0.1,
    position = {3.0, 2.0, -2.5},
    rotation = {10.0, -90.0, 0.0},
}

local light = {
    direction = {-2.0, -2.0, 0.0}
}

local model = {
    name = "House",
    file = "assets/models/house/house.obj",
    position = {0.0, 0.0, 0.0},
    rotation = {0.0, 90.0, 0.0},
    scale = 0.5
}

return { Camera = camera, Light = light, Model = model }