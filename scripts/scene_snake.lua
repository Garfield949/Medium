local camera = {
    fov = 75,
    near = 0.01,
    position = {0.0, 2.0, -4.5},
    rotation = {0.0, 0.0, 0.0},
}

local light = {
    direction = {-1.0, -1.0, 2.0}
}

local model = {
    name = "Snake Statue",
    file = "assets/models/snake/snake.obj",
    position = {0.0, 0.0, 0.0},
    rotation = {0.0, 0.0, 0.0},
    scale = 0.5
}

return { Camera = camera, Light = light, Model = model }