local camera = {
    fov = 80,
    near = 0.1,
    position = {0.0, 5.0, 0.0},
    rotation = {0.0, 0.0, 0.0},
}

local light = {
    direction = {2, -1, 1},
    max_shadow_dist = 15
}

local model = {
    name = "Minecraft",
    file = "assets/models/minecraft/minecraft.obj",
    position = {0.0, 0.0, 0.0},
    rotation = {0.0, 0.0, 0.0},
    scale = 10
}

return { Camera = camera, Light = light, Model = model }